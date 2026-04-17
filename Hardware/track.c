#include "track.h"
#include "LED.h"
#include "Buzzer.h"
#include "hcsr04.h"

#define TICK_MS                   20U
#define OBSTACLE_ALERT_MS         3000U
#define OBSTACLE_MIN_ALERT_MS     2000U
#define JUNCTION_CONFIRM_TICKS    2U
#define JUNCTION_COOLDOWN_TICKS   8U
#define FORCE_STRAIGHT_TICKS      8U
#define JUNCTION_REARM_TICKS      1U
#define TURN_MIN_TICKS            4U
#define TURN_MAX_TICKS            18U

#define SPEED_BASE                STRAIGHT_SPEED
#define SPEED_FAST                54
#define SPEED_TURN_PIVOT          48
#define SPEED_ARC_OUTER           54
#define SPEED_ARC_INNER           38

#define SPEED_ARC_BASE            40

#define KP_NUM                    30
#define KD_NUM                    18
#define KP_ARC_NUM                60
#define KD_ARC_NUM                30
#define ERR_SCALE                 10
#define MIN_DIFF_STEP             2
#define OBSTACLE_ALERT_TICKS      ((uint16_t)(OBSTACLE_ALERT_MS / TICK_MS) + 5U)
#define OBSTACLE_MIN_HOLD_TICKS   ((uint16_t)(OBSTACLE_MIN_ALERT_MS / TICK_MS))
#define OBSTACLE_NEAR_CM          20U
#define ULTRA_SAMPLE_INTERVAL     5U

#define EVENT_LEFT_HALF           0x01
#define EVENT_RIGHT_HALF          0x02
#define EVENT_CROSS               0x04

typedef enum
{
	ZONE_ARC = 0,
	ZONE_GRID,
	ZONE_OBSTACLE,
	ZONE_HORN,
	ZONE_STOP,
	ZONE_DONE
} TrackZone_t;

typedef enum
{
	ACT_FOLLOW = 0,
	ACT_TURN_LEFT,
	ACT_TURN_RIGHT,
	ACT_FORCE_STRAIGHT,
	ACT_STOP
} Action_t;

static TrackZone_t g_zone = ZONE_ARC;
static Action_t g_action = ACT_FOLLOW;

static uint8_t g_filteredState = 0x1F;
static uint8_t g_eventFlags = 0;
static uint8_t g_junctionStableCnt = 0;
static uint8_t g_junctionCooldown = 0;
static uint8_t g_junctionArmed = 1;
static uint8_t g_noEventTicks = 0;

static uint8_t g_gridPhase = 0;
static uint8_t g_phaseCounter = 0;

static uint8_t g_forceTicks = 0;
static uint8_t g_turnTicks = 0;

static int16_t g_lastErr = 0;
static int8_t g_speedA = 0;
static int8_t g_speedB = 0;

static uint16_t g_ledAlertTicks = 0;
static uint16_t g_buzzerTicks = 0;
static uint8_t g_hornLineCount = 0;
static uint16_t g_ultraDistanceCm = 400U;
static uint8_t g_ultraSampleTicks = 0;

static uint8_t IsRightOnly(uint8_t flags)
{
	if (((flags & EVENT_RIGHT_HALF) != 0U) && ((flags & EVENT_LEFT_HALF) == 0U) && ((flags & EVENT_CROSS) == 0U))
	{
		return 1;
	}
	return 0;
}

static uint8_t IsLeftOnly(uint8_t flags)
{
	if (((flags & EVENT_LEFT_HALF) != 0U) && ((flags & EVENT_RIGHT_HALF) == 0U) && ((flags & EVENT_CROSS) == 0U))
	{
		return 1;
	}
	return 0;
}

static uint8_t IsRightBranchTrigger(uint8_t flags, uint8_t state)
{
	if ((flags & EVENT_RIGHT_HALF) == 0U)
	{
		return 0;
	}

	/* Strict single-side event is always acceptable. */
	if (IsRightOnly(flags) != 0U)
	{
		return 1;
	}

	/* In grid, allow right-dominant mixed patterns to avoid missed turns at dense junctions. */
	if (((state & 0x03U) != 0x03U) && ((state & 0x04U) == 0U) && ((state & 0x18U) != 0x00U))
	{
		return 1;
	}

	return 0;
}

static uint8_t IsLeftBranchTrigger(uint8_t flags, uint8_t state)
{
	if ((flags & EVENT_LEFT_HALF) == 0U)
	{
		return 0;
	}

	/* Strict single-side event is always acceptable. */
	if (IsLeftOnly(flags) != 0U)
	{
		return 1;
	}

	/* In grid, allow left-dominant mixed patterns to avoid missed turns at dense junctions. */
	if (((state & 0x18U) != 0x18U) && ((state & 0x04U) == 0U) && ((state & 0x03U) != 0x00U))
	{
		return 1;
	}

	return 0;
}

static int8_t ClampSpeed(int16_t speed)
{
	if (speed > 100)
	{
		return 100;
	}
	if (speed < -100)
	{
		return -100;
	}
	return (int8_t)speed;
}

static void ApplySpeed(int8_t speedA, int8_t speedB)
{
	g_speedA = speedA;
	g_speedB = speedB;
	Motor_SetSpeed_A(speedA);
	Motor_SetSpeed_B(speedB);
}

static uint8_t CountBlack(uint8_t state)
{
	uint8_t i;
	uint8_t black = 0;
	/* Sensor is active-low: 0 means black line is detected. */
	for (i = 0; i < 5; i++)
	{
		if (((state >> i) & 0x01) == 0)
		{
			black++;
		}
	}
	return black;
}

static uint8_t FilteredSensorRead(void)
{
	uint8_t sample;
	uint8_t bit;
	uint8_t i;
	uint8_t lowCnt;
	uint8_t result = 0;

	/* 5x oversampling + majority vote per channel to suppress noise. */
	for (bit = 0; bit < 5; bit++)
	{
		lowCnt = 0;
		for (i = 0; i < 5; i++)
		{
			sample = LightSensor_GetState();
			if (((sample >> bit) & 0x01) == 0)
			{
				lowCnt++;
			}
		}

		if (lowCnt < 3)
		{
			result |= (uint8_t)(1U << bit);
		}
	}

	return result;
}

static uint8_t MakeEventFlags(uint8_t state)
{
	uint8_t flags = 0;
	uint8_t leftHalf = 0;
	uint8_t rightHalf = 0;
	uint8_t center = 0;
	uint8_t blackCnt = CountBlack(state);

	/* Left side active when any left sensor sees black. */
	if ((state & 0x18) != 0x18)
	{
		leftHalf = 1;
	}
	/* Right side active when any right sensor sees black. */
	if ((state & 0x03) != 0x03)
	{
		rightHalf = 1;
	}
	/* Middle sensor on line. */
	if ((state & 0x04) == 0)
	{
		center = 1;
	}

	if (leftHalf)
	{
		flags |= EVENT_LEFT_HALF;
	}
	if (rightHalf)
	{
		flags |= EVENT_RIGHT_HALF;
	}

	/* Reject weak side-only noise: half-junction must include center and >=2 black hits. */
	if ((((flags & EVENT_LEFT_HALF) != 0U) || ((flags & EVENT_RIGHT_HALF) != 0U)) && ((center == 0U) || (blackCnt < 2U)))
	{
		flags &= (uint8_t)~(EVENT_LEFT_HALF | EVENT_RIGHT_HALF);
	}

	if ((blackCnt >= 4) || (leftHalf && rightHalf && center))
	{
		/* Intersection-like pattern. */
		flags |= EVENT_CROSS;
	}

	return flags;
}

static void FollowWithPD(uint8_t state, int8_t baseSpeed, int16_t kpNum, int16_t kdNum)
{
	const int8_t weights[5] = {2, 1, 0, -1, -2};
	int16_t sum = 0;
	int16_t hits = 0;
	int16_t err;
	int16_t dErr;
	int16_t diff;
	int16_t speedA;
	int16_t speedB;
	uint8_t i;

	/* Weighted line position error: left is positive, right is negative. */
	for (i = 0; i < 5; i++)
	{
		if (((state >> i) & 0x01) == 0)
		{
			sum += weights[i] * ERR_SCALE;
			hits++;
		}
	}

	if (hits > 0)
	{
		err = sum / hits;
	}
	else
	{
		/* Keep last error if line is temporarily lost. */
		err = g_lastErr;
	}

	dErr = err - g_lastErr;
	g_lastErr = err;

	/* Differential speed command from PD controller. */
	diff = (kpNum * err + kdNum * dErr) / 100;

	/* Ensure tiny non-zero error still creates a correction in ARC/grid follow mode. */
	if ((err != 0) && (diff == 0))
	{
		diff = (err > 0) ? MIN_DIFF_STEP : -MIN_DIFF_STEP;
	}
	else if ((err > 0) && (diff < MIN_DIFF_STEP))
	{
		diff = MIN_DIFF_STEP;
	}
	else if ((err < 0) && (diff > -MIN_DIFF_STEP))
	{
		diff = -MIN_DIFF_STEP;
	}

	speedA = (int16_t)baseSpeed + diff;
	speedB = (int16_t)baseSpeed - diff;

	ApplySpeed(ClampSpeed(speedA), ClampSpeed(speedB));
}

static void EnterTurnRight(void)
{
	g_action = ACT_TURN_RIGHT;
	g_turnTicks = 0;
}

static void EnterTurnLeft(void)
{
	g_action = ACT_TURN_LEFT;
	g_turnTicks = 0;
}

static void EnterForceStraight(void)
{
	g_action = ACT_FORCE_STRAIGHT;
	g_forceTicks = FORCE_STRAIGHT_TICKS;
}

static void EnterObstacleZone(void)
{
	g_zone = ZONE_OBSTACLE;
	g_ledAlertTicks = OBSTACLE_ALERT_TICKS;
	LED_OBS_ON();
}

static void OnConfirmedJunction(uint8_t flags)
{
	/* Route state machine triggered only by debounced junction events. */
	if (g_zone == ZONE_ARC)
	{
		if (IsLeftBranchTrigger(flags, g_filteredState) != 0U)
		{
			EnterTurnRight();
			g_zone = ZONE_GRID;
			g_gridPhase = 0;
			g_phaseCounter = 0;
		}
		return;
	}

	if (g_zone == ZONE_GRID)
	{
		switch (g_gridPhase)
		{
			case 0:
				if (IsRightBranchTrigger(flags, g_filteredState) != 0U)
				{
					g_phaseCounter++;
					if (g_phaseCounter >= 2)
					{
						EnterTurnRight();
						g_gridPhase = 1;
						g_phaseCounter = 0;
					}
					else
					{
						EnterForceStraight();
					}
				}
				break;

			case 1:
				if (IsRightBranchTrigger(flags, g_filteredState) != 0U)
				{
					g_phaseCounter++;
					if (g_phaseCounter >= 1)
					{
						EnterTurnRight();
						g_gridPhase = 2;
						g_phaseCounter = 0;
					}
					else
					{
						EnterForceStraight();
					}
				}
				break;

			case 2:
				if (IsRightBranchTrigger(flags, g_filteredState) != 0U)
				{
					EnterTurnRight();
					g_gridPhase = 3;
				}
				break;

			case 3:
				if ((flags & EVENT_CROSS) != 0U)
				{
					EnterForceStraight();
					g_gridPhase = 4;
				}
				break;

			case 4:
				if ((flags & EVENT_CROSS) != 0U)
				{
					EnterTurnLeft();
					g_gridPhase = 5;
				}
				break;

			case 5:
				if (IsLeftBranchTrigger(flags, g_filteredState) != 0U)
				{
					EnterTurnLeft();
					g_gridPhase = 6;
				}
				break;

			case 6:
				if (IsLeftBranchTrigger(flags, g_filteredState) != 0U)
				{
					EnterTurnLeft();
					g_gridPhase = 7;
				}
				break;

			case 7:
				if ((flags & EVENT_CROSS) != 0U)
				{
					EnterForceStraight();
					g_gridPhase = 8;
				}
				break;

			case 8:
				if ((flags & EVENT_CROSS) != 0U)
				{
					EnterTurnRight();
					g_gridPhase = 9;
				}
				break;

			case 9:
				if (IsRightBranchTrigger(flags, g_filteredState) != 0U)
				{
					EnterForceStraight();
					EnterObstacleZone();
				}
				break;

			default:
				break;
		}
		return;
	}

	if (g_zone == ZONE_OBSTACLE)
	{
		/* Keep straight in obstacle section, allow turn only after >=2s alert and strict right-half marker. */
		if ((g_ledAlertTicks <= (OBSTACLE_ALERT_TICKS - OBSTACLE_MIN_HOLD_TICKS)) && (IsRightOnly(flags) != 0U))
		{
			EnterTurnRight();
			g_zone = ZONE_HORN;
			g_hornLineCount = 0;
		}
		return;
	}

	if (g_zone == ZONE_HORN)
	{
		if ((flags & EVENT_CROSS) != 0U)
		{
			if (g_hornLineCount == 0)
			{
				g_hornLineCount = 1;
				g_buzzerTicks = (uint16_t)(2000U / TICK_MS) + 5U;
				Buzzer_ON();
				EnterForceStraight();
			}
			else
			{
				g_zone = ZONE_DONE;
				g_action = ACT_STOP;
				ApplySpeed(0, 0);
				Buzzer_OFF();
				LED_OBS_OFF();
				LED1_OFF();
				LED2_OFF();
			}
		}
	}
}

void Track_Init(void)
{
	HCSR04_Init();

	g_zone = ZONE_ARC;
	g_action = ACT_FOLLOW;
	g_filteredState = 0x1F;
	g_eventFlags = 0;
	g_junctionStableCnt = 0;
	g_junctionCooldown = 0;
	g_junctionArmed = 1;
	g_noEventTicks = 0;
	g_gridPhase = 0;
	g_phaseCounter = 0;
	g_forceTicks = 0;
	g_turnTicks = 0;
	g_lastErr = 0;
	g_speedA = 0;
	g_speedB = 0;
	g_ledAlertTicks = 0;
	g_buzzerTicks = 0;
	g_hornLineCount = 0;
	g_ultraDistanceCm = 400U;
	g_ultraSampleTicks = 0;
}

void Track_Run(void)
{
	uint8_t rawFlags;
	uint8_t newFlags;

	if (g_zone == ZONE_DONE)
	{
		g_action = ACT_STOP;
		ApplySpeed(0, 0);
		return;
	}

	g_filteredState = FilteredSensorRead();
	rawFlags = MakeEventFlags(g_filteredState);

	/* Event debouncing: require repeated identical detections. */
	if ((rawFlags & (EVENT_LEFT_HALF | EVENT_RIGHT_HALF | EVENT_CROSS)) != 0U)
	{
		g_noEventTicks = 0;
		if (rawFlags == g_eventFlags)
		{
			if (g_junctionStableCnt < 255)
			{
				g_junctionStableCnt++;
			}
		}
		else
		{
			g_junctionStableCnt = 1;
		}
		g_eventFlags = rawFlags;
	}
	else
	{
		if (g_noEventTicks < 255)
		{
			g_noEventTicks++;
		}
		if (g_noEventTicks >= JUNCTION_REARM_TICKS)
		{
			/* Re-arm only after a clear no-event gap to avoid phantom repeated turns. */
			g_junctionArmed = 1;
		}
		g_junctionStableCnt = 0;
		g_eventFlags = 0;
	}

	if (g_junctionCooldown > 0)
	{
		g_junctionCooldown--;
	}

	newFlags = 0;
	if ((g_junctionStableCnt >= JUNCTION_CONFIRM_TICKS) &&
		(g_junctionCooldown == 0U) &&
		(g_action == ACT_FOLLOW) &&
		(g_junctionArmed != 0U))
	{
		/* Accept exactly one confirmed event, then apply cooldown. */
		newFlags = g_eventFlags;
		g_junctionCooldown = JUNCTION_COOLDOWN_TICKS;
		g_junctionArmed = 0;
	}

	if (newFlags != 0U)
	{
		OnConfirmedJunction(newFlags);
	}

	if (g_zone == ZONE_OBSTACLE)
	{
		if (g_ultraSampleTicks == 0U)
		{
			g_ultraDistanceCm = HCSR04_GetValue();
			g_ultraSampleTicks = ULTRA_SAMPLE_INTERVAL;
		}
		else
		{
			g_ultraSampleTicks--;
		}
	}
	else
	{
		g_ultraDistanceCm = 400U;
		g_ultraSampleTicks = 0U;
	}

	if ((g_zone == ZONE_OBSTACLE) &&
		((g_ledAlertTicks > 0U) || ((g_ultraDistanceCm > 0U) && (g_ultraDistanceCm < OBSTACLE_NEAR_CM))))
	{
		if (g_ledAlertTicks > 0U)
		{
			g_ledAlertTicks--;
		}
		LED_OBS_ON();
	}
	else
	{
		LED_OBS_OFF();
		if ((g_zone != ZONE_OBSTACLE) && (g_zone != ZONE_HORN))
		{
			LED1_OFF();
			LED2_OFF();
		}
	}

	if (g_buzzerTicks > 0)
	{
		g_buzzerTicks--;
		Buzzer_ON();
	}
	else
	{
		Buzzer_OFF();
	}

	if (g_action == ACT_TURN_LEFT)
	{
		g_turnTicks++;
		ApplySpeed(-SPEED_TURN_PIVOT, SPEED_TURN_PIVOT);

		/* Exit turn after minimum time once line is recaptured, or by timeout. */
		if ((g_turnTicks >= TURN_MIN_TICKS && (g_filteredState == 0x04U || g_filteredState == 0x06U || g_filteredState == 0x0CU)) ||
			(g_turnTicks >= TURN_MAX_TICKS))
		{
			g_action = ACT_FOLLOW;
			g_junctionCooldown = JUNCTION_COOLDOWN_TICKS;
		}
		return;
	}

	if (g_action == ACT_TURN_RIGHT)
	{
		g_turnTicks++;
		ApplySpeed(SPEED_TURN_PIVOT, -SPEED_TURN_PIVOT);

		/* Exit turn after minimum time once line is recaptured, or by timeout. */
		if ((g_turnTicks >= TURN_MIN_TICKS && (g_filteredState == 0x04U || g_filteredState == 0x06U || g_filteredState == 0x0CU)) ||
			(g_turnTicks >= TURN_MAX_TICKS))
		{
			g_action = ACT_FOLLOW;
			g_junctionCooldown = JUNCTION_COOLDOWN_TICKS;
		}
		return;
	}

	if (g_action == ACT_FORCE_STRAIGHT)
	{
		ApplySpeed(SPEED_FAST, SPEED_FAST);
		if (g_forceTicks > 0)
		{
			g_forceTicks--;
		}
		if (g_forceTicks == 0)
		{
			g_action = ACT_FOLLOW;
		}
		return;
	}

	if (g_action == ACT_STOP)
	{
		ApplySpeed(0, 0);
		return;
	}

	if (g_zone == ZONE_ARC)
	{
		/* Keep ARC behavior conservative and sensor-driven during route bring-up. */
		FollowWithPD(g_filteredState, SPEED_ARC_BASE, KP_ARC_NUM, KD_ARC_NUM);
		return;
	}

	FollowWithPD(g_filteredState, SPEED_BASE, KP_NUM, KD_NUM);
}

uint8_t Track_GetFilteredState(void)
{
	return g_filteredState;
}

uint8_t Track_GetZone(void)
{
	return (uint8_t)g_zone;
}

uint8_t Track_GetAction(void)
{
	return (uint8_t)g_action;
}

uint8_t Track_GetGridPhase(void)
{
	return g_gridPhase;
}

uint8_t Track_GetEventFlags(void)
{
	return g_eventFlags;
}

int8_t Track_GetSpeedA(void)
{
	return g_speedA;
}

int8_t Track_GetSpeedB(void)
{
	return g_speedB;
}
