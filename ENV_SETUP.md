# STM32循迹小车开发环境配置（Windows + VS Code）

## 1. 必装软件
1. Keil MDK-ARM v5（包含 `UV4.exe`）
2. STM32F1 设备包（Keil Pack Installer 内安装）
3. ST-Link 驱动（或 STM32CubeProgrammer）
4. OpenOCD（仅当你要在 VS Code 里断点调试）
5. Git（已用于源码管理）

## 2. VS Code 插件
本工程已在 `.vscode/extensions.json` 写入推荐插件，打开工作区后会弹出推荐安装。

核心插件：
- C/C++：`ms-vscode.cpptools`
- Cortex 调试：`marus25.cortex-debug`
- 格式化：`xaver.clang-format`

## 3. 一键构建
在 VS Code 命令面板执行 `Tasks: Run Task`：
- `Keil: Build Project`
- `Keil: Rebuild Project`

如果提示找不到 `UV4.exe`：
- 方案A：把 `C:/Keil_v5/UV4` 加到系统 `PATH`
- 方案B：修改 `.vscode/tasks.json` 中 `C:/Keil_v5/UV4/UV4.exe` 为你的实际安装路径

## 4. 调试（可选）
`.vscode/launch.json` 已提供 OpenOCD + ST-Link 模板。
使用前确认：
1. 已安装 OpenOCD，并可在终端执行 `openocd --version`
2. 已连接 ST-Link
3. 已先执行一次构建，生成 `Objects/Project.axf`

## 5. 目录约定
已配置 `.gitignore`，默认只提交源码和共享工程文件，忽略构建产物和本地用户配置。
