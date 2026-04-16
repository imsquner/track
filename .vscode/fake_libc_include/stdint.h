#ifndef _FAKE_STDINT_H_
#define _FAKE_STDINT_H_

/* Minimal C99 integer types for IntelliSense when no toolchain is configured. */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

typedef unsigned int uintptr_t;
typedef signed int intptr_t;

#endif
