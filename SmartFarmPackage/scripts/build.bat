@echo off
REM ================================================================
REM 智能农场监控系统 - 自动化构建脚本
REM Smart Farm Monitoring System - Automated Build Script
REM ================================================================

setlocal enabledelayedexpansion

echo ========================================
echo 🚜 智能农场监控系统构建脚本
echo ========================================

REM 设置构建参数
set PROJECT_NAME=SmartFarmSystem
set BUILD_CONFIG=Release
set BUILD_PLATFORM=x64
set SOLUTION_FILE=%~dp0..\Project2.sln
set OUTPUT_DIR=%~dp0..\Build\%BUILD_CONFIG%-%BUILD_PLATFORM%
set PACKAGE_DIR=%~dp0..\Package

REM 检查Visual Studio环境
echo [1/8] 检查构建环境...
where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 错误: 未找到MSBuild，请确保已安装Visual Studio 2022
    echo 提示: 运行 "Developer Command Prompt for VS 2022"
    pause
    exit /b 1
)
echo ✅ Visual Studio环境就绪

REM 清理之前的构建
echo [2/8] 清理构建目录...
if exist "%OUTPUT_DIR%" (
    rmdir /s /q "%OUTPUT_DIR%"
)
mkdir "%OUTPUT_DIR%" 2>nul
echo ✅ 构建目录已清理

REM 检查依赖库
echo [3/8] 检查依赖库...
set GLFW_PATH=%~dp0..\packages\glfw.3.4.0
set GLEW_PATH=%~dp0..\packages\glew.v140.1.12.0
set GLM_PATH=%~dp0..\packages\glm.1.0.1

if not exist "%GLFW_PATH%" (
    echo ❌ 错误: GLFW库未找到
    echo 请确保NuGet包已正确安装
    pause
    exit /b 1
)

if not exist "%GLEW_PATH%" (
    echo ❌ 错误: GLEW库未找到
    echo 请确保NuGet包已正确安装
    pause
    exit /b 1
)

if not exist "%GLM_PATH%" (
    echo ❌ 错误: GLM库未找到
    echo 请确确保NuGet包已正确安装
    pause
    exit /b 1
)
echo ✅ 所有依赖库检查完成

REM 构建项目
echo [4/8] 开始构建项目...
echo 配置: %BUILD_CONFIG%
echo 平台: %BUILD_PLATFORM%
echo 解决方案: %SOLUTION_FILE%

msbuild "%SOLUTION_FILE%" /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /p:OutDir="%OUTPUT_DIR%\\" /m /v:minimal

if %errorlevel% neq 0 (
    echo ❌ 构建失败
    pause
    exit /b 1
)
echo ✅ 项目构建成功

REM 复制可执行文件
echo [5/8] 准备发布文件...
if not exist "%OUTPUT_DIR%\Project2.exe" (
    echo ❌ 错误: 找不到生成的可执行文件
    pause
    exit /b 1
)

REM 创建发布目录
if exist "%PACKAGE_DIR%" (
    rmdir /s /q "%PACKAGE_DIR%"
)
mkdir "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%\bin"
mkdir "%PACKAGE_DIR%\docs"
mkdir "%PACKAGE_DIR%\assets"
mkdir "%PACKAGE_DIR%\config"

REM 复制主程序
copy "%OUTPUT_DIR%\Project2.exe" "%PACKAGE_DIR%\bin\SmartFarmSystem.exe"
copy "%OUTPUT_DIR%\Project2.pdb" "%PACKAGE_DIR%\bin\" 2>nul

REM 复制必要的DLL
echo [6/8] 复制运行时依赖...
copy "%~dp0..\x64\Debug\glfw3.dll" "%PACKAGE_DIR%\bin\" 2>nul

REM 复制文档
echo [7/8] 打包文档和资源...
xcopy "%~dp0..\SmartFarmPackage\docs" "%PACKAGE_DIR%\docs\" /E /I /Y
copy "%~dp0..\SmartFarmPackage\README.md" "%PACKAGE_DIR%\"
copy "%~dp0..\SmartFarmPackage\LICENSE" "%PACKAGE_DIR%\" 2>nul
copy "%~dp0..\SmartFarmPackage\CHANGELOG.md" "%PACKAGE_DIR%\" 2>nul

REM 创建配置文件
echo [8/8] 生成配置文件...
echo # 智能农场监控系统配置文件 > "%PACKAGE_DIR%\config\settings.conf"
echo # Smart Farm System Configuration >> "%PACKAGE_DIR%\config\settings.conf"
echo. >> "%PACKAGE_DIR%\config\settings.conf"
echo [Graphics] >> "%PACKAGE_DIR%\config\settings.conf"
echo resolution_width=1400 >> "%PACKAGE_DIR%\config\settings.conf"
echo resolution_height=900 >> "%PACKAGE_DIR%\config\settings.conf"
echo fullscreen=false >> "%PACKAGE_DIR%\config\settings.conf"
echo vsync=true >> "%PACKAGE_DIR%\config\settings.conf"
echo msaa_samples=4 >> "%PACKAGE_DIR%\config\settings.conf"
echo. >> "%PACKAGE_DIR%\config\settings.conf"
echo [Simulation] >> "%PACKAGE_DIR%\config\settings.conf"
echo plant_count=1000 >> "%PACKAGE_DIR%\config\settings.conf"
echo sensor_count=50 >> "%PACKAGE_DIR%\config\settings.conf"
echo weather_change_interval=300 >> "%PACKAGE_DIR%\config\settings.conf"
echo auto_save_interval=60 >> "%PACKAGE_DIR%\config\settings.conf"

REM 创建启动脚本
echo @echo off > "%PACKAGE_DIR%\SmartFarmSystem.bat"
echo cd /d "%%~dp0bin" >> "%PACKAGE_DIR%\SmartFarmSystem.bat"
echo SmartFarmSystem.exe >> "%PACKAGE_DIR%\SmartFarmSystem.bat"
echo pause >> "%PACKAGE_DIR%\SmartFarmSystem.bat"

REM 生成版本信息
echo [Version] > "%PACKAGE_DIR%\version.txt"
echo Product=Smart Farm Monitoring System >> "%PACKAGE_DIR%\version.txt"
echo Version=2.0.0 >> "%PACKAGE_DIR%\version.txt"
echo Build=%date% %time% >> "%PACKAGE_DIR%\version.txt"
echo Configuration=%BUILD_CONFIG% >> "%PACKAGE_DIR%\version.txt"
echo Platform=%BUILD_PLATFORM% >> "%PACKAGE_DIR%\version.txt"

REM 显示构建结果
echo.
echo ========================================
echo ✅ 构建完成！
echo ========================================
echo 📦 发布包位置: %PACKAGE_DIR%
echo 🎮 主程序: %PACKAGE_DIR%\bin\SmartFarmSystem.exe
echo 📚 文档: %PACKAGE_DIR%\docs\
echo ⚙️ 配置: %PACKAGE_DIR%\config\settings.conf
echo.
echo 💡 快速启动: 双击 SmartFarmSystem.bat
echo.

REM 询问是否立即运行
set /p run_now="是否立即运行程序? (Y/N): "
if /i "%run_now%"=="Y" (
    start "" "%PACKAGE_DIR%\bin\SmartFarmSystem.exe"
)

REM 询问是否创建ZIP压缩包
set /p create_zip="是否创建ZIP发布包? (Y/N): "
if /i "%create_zip%"=="Y" (
    echo 正在创建ZIP压缩包...
    powershell -command "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath '%~dp0..\SmartFarmSystem-v2.0.0.zip' -Force"
    echo ✅ ZIP包已创建: SmartFarmSystem-v2.0.0.zip
)

echo.
echo 🎉 构建流程全部完成！
pause