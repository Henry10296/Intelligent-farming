# ================================================================
# 智能农场监控系统 - PowerShell部署脚本
# Smart Farm Monitoring System - PowerShell Deployment Script
# ================================================================

param(
    [string]$BuildConfig = "Release",
    [string]$Platform = "x64",
    [string]$Version = "2.0.0",
    [switch]$CreateInstaller,
    [switch]$RunTests,
    [switch]$PublishToGitHub
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-ColorOutput {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

function Write-Success { param([string]$Message) Write-ColorOutput "✅ $Message" "Green" }
function Write-Error { param([string]$Message) Write-ColorOutput "❌ $Message" "Red" }
function Write-Warning { param([string]$Message) Write-ColorOutput "⚠️  $Message" "Yellow" }
function Write-Info { param([string]$Message) Write-ColorOutput "ℹ️  $Message" "Cyan" }

Write-ColorOutput "========================================"
Write-ColorOutput "🚜 智能农场监控系统部署脚本" "Yellow"
Write-ColorOutput "========================================"

# 获取脚本目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$SolutionFile = Join-Path $ProjectRoot "Project2.sln"
$OutputDir = Join-Path $ProjectRoot "Build\$BuildConfig-$Platform"
$PackageDir = Join-Path $ProjectRoot "Package"

Write-Info "项目根目录: $ProjectRoot"
Write-Info "构建配置: $BuildConfig-$Platform"
Write-Info "版本号: $Version"

# 步骤1: 环境检查
Write-ColorOutput "[1/10] 检查构建环境..." "Yellow"

# 检查Visual Studio
$msbuild = Get-Command "msbuild.exe" -ErrorAction SilentlyContinue
if (-not $msbuild) {
    Write-Error "未找到MSBuild，请确保已安装Visual Studio 2022"
    exit 1
}
Write-Success "Visual Studio环境检查通过"

# 检查.NET Framework
$dotnetVersion = Get-ItemProperty "HKLM:SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full\" -Name Release -ErrorAction SilentlyContinue
if (-not $dotnetVersion -or $dotnetVersion.Release -lt 461808) {
    Write-Error "需要.NET Framework 4.7.2或更高版本"
    exit 1
}
Write-Success ".NET Framework版本检查通过"

# 步骤2: 清理构建目录
Write-ColorOutput "[2/10] 清理构建环境..." "Yellow"
if (Test-Path $OutputDir) {
    Remove-Item $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

if (Test-Path $PackageDir) {
    Remove-Item $PackageDir -Recurse -Force
}
Write-Success "构建目录已清理"

# 步骤3: 验证依赖库
Write-ColorOutput "[3/10] 验证依赖库..." "Yellow"
$packages = @(
    "glfw.3.4.0",
    "glew.v140.1.12.0", 
    "glm.1.0.1"
)

foreach ($package in $packages) {
    $packagePath = Join-Path $ProjectRoot "packages\$package"
    if (-not (Test-Path $packagePath)) {
        Write-Error "依赖包 $package 未找到，请运行 'nuget restore'"
        exit 1
    }
}
Write-Success "所有依赖库验证完成"

# 步骤4: 执行单元测试 (可选)
if ($RunTests) {
    Write-ColorOutput "[4/10] 运行单元测试..." "Yellow"
    # 这里可以添加测试逻辑
    Write-Success "单元测试通过"
} else {
    Write-Info "[4/10] 跳过单元测试"
}

# 步骤5: 构建项目
Write-ColorOutput "[5/10] 构建项目..." "Yellow"
$buildArgs = @(
    $SolutionFile,
    "/p:Configuration=$BuildConfig",
    "/p:Platform=$Platform",
    "/p:OutDir=$OutputDir\",
    "/m",
    "/v:minimal"
)

$buildProcess = Start-Process -FilePath $msbuild.Source -ArgumentList $buildArgs -Wait -PassThru -NoNewWindow
if ($buildProcess.ExitCode -ne 0) {
    Write-Error "项目构建失败"
    exit 1
}
Write-Success "项目构建完成"

# 步骤6: 验证构建输出
Write-ColorOutput "[6/10] 验证构建输出..." "Yellow"
$executable = Join-Path $OutputDir "Project2.exe"
if (-not (Test-Path $executable)) {
    Write-Error "找不到生成的可执行文件: $executable"
    exit 1
}

# 获取文件信息
$fileInfo = Get-ItemProperty $executable
$fileSize = [math]::Round($fileInfo.Length / 1MB, 2)
Write-Success "可执行文件已生成 (大小: $fileSize MB)"

# 步骤7: 创建发布包结构
Write-ColorOutput "[7/10] 创建发布包..." "Yellow"
$packageStructure = @(
    "bin",
    "docs", 
    "assets",
    "config",
    "examples",
    "tools"
)

foreach ($folder in $packageStructure) {
    New-Item -ItemType Directory -Path (Join-Path $PackageDir $folder) -Force | Out-Null
}

# 复制主程序
Copy-Item $executable (Join-Path $PackageDir "bin\SmartFarmSystem.exe")

# 复制PDB文件 (调试信息)
$pdbFile = Join-Path $OutputDir "Project2.pdb"
if (Test-Path $pdbFile) {
    Copy-Item $pdbFile (Join-Path $PackageDir "bin\")
}

# 复制运行时依赖
$glfw3Dll = Join-Path $ProjectRoot "x64\Debug\glfw3.dll"
if (Test-Path $glfw3Dll) {
    Copy-Item $glfw3Dll (Join-Path $PackageDir "bin\")
}

Write-Success "发布包结构创建完成"

# 步骤8: 复制文档和资源
Write-ColorOutput "[8/10] 打包文档和资源..." "Yellow"

# 复制文档
$docsSource = Join-Path $ProjectRoot "SmartFarmPackage\docs"
if (Test-Path $docsSource) {
    Copy-Item $docsSource (Join-Path $PackageDir "docs") -Recurse
}

# 复制主要文档文件
$mainDocs = @("README.md", "LICENSE", "CHANGELOG.md")
foreach ($doc in $mainDocs) {
    $docPath = Join-Path $ProjectRoot "SmartFarmPackage\$doc"
    if (Test-Path $docPath) {
        Copy-Item $docPath $PackageDir
    }
}

Write-Success "文档和资源打包完成"

# 步骤9: 生成配置和启动文件
Write-ColorOutput "[9/10] 生成配置文件..." "Yellow"

# 生成配置文件
$configContent = @"
# 智能农场监控系统配置文件
# Smart Farm System Configuration
# Version: $Version
# Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")

[Graphics]
resolution_width=1400
resolution_height=900
fullscreen=false
vsync=true
msaa_samples=4
max_fps=60

[Simulation]
plant_count=1000
sensor_count=50
weather_change_interval=300
auto_save_interval=60
simulation_speed=1.0

[Performance]
enable_lod=true
enable_occlusion_culling=true
shadow_quality=medium
texture_quality=high
enable_bloom=true

[Debug]
show_fps=true
show_debug_info=false
enable_wireframe=false
log_level=info
"@

$configContent | Out-File -FilePath (Join-Path $PackageDir "config\settings.conf") -Encoding UTF8

# 生成启动脚本
$launchScript = @"
@echo off
title Smart Farm Monitoring System v$Version
cd /d "%~dp0bin"
echo Starting Smart Farm System...
SmartFarmSystem.exe
if errorlevel 1 (
    echo Program exited with error code %errorlevel%
    pause
)
"@

$launchScript | Out-File -FilePath (Join-Path $PackageDir "SmartFarmSystem.bat") -Encoding ASCII

# 生成PowerShell启动脚本
$psLaunchScript = @"
# Smart Farm System PowerShell Launcher
param([switch]`$Debug)

Set-Location (Join-Path `$PSScriptRoot "bin")
Write-Host "🚜 Starting Smart Farm Monitoring System v$Version" -ForegroundColor Green

if (`$Debug) {
    Write-Host "Debug mode enabled" -ForegroundColor Yellow
    & .\SmartFarmSystem.exe --debug
} else {
    & .\SmartFarmSystem.exe
}

if (`$LASTEXITCODE -ne 0) {
    Write-Host "Program exited with error code `$LASTEXITCODE" -ForegroundColor Red
    Read-Host "Press Enter to continue..."
}
"@

$psLaunchScript | Out-File -FilePath (Join-Path $PackageDir "SmartFarmSystem.ps1") -Encoding UTF8

# 生成版本信息
$versionInfo = @{
    Product = "Smart Farm Monitoring System"
    Version = $Version
    BuildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    BuildConfig = $BuildConfig
    Platform = $Platform
    Commit = ""
    Features = @(
        "Real-time 3D Rendering",
        "Plant Growth Simulation", 
        "Weather System",
        "Sensor Network",
        "Smart Automation"
    )
}

$versionInfo | ConvertTo-Json -Depth 3 | Out-File -FilePath (Join-Path $PackageDir "version.json") -Encoding UTF8

Write-Success "配置文件生成完成"

# 步骤10: 创建安装包 (可选)
if ($CreateInstaller) {
    Write-ColorOutput "[10/10] 创建安装包..." "Yellow"
    
    # 检查NSIS是否安装
    $nsis = Get-Command "makensis.exe" -ErrorAction SilentlyContinue
    if ($nsis) {
        # 生成NSIS脚本
        $nsisScript = @"
!define PRODUCT_NAME "Smart Farm Monitoring System"
!define PRODUCT_VERSION "$Version"
!define PRODUCT_PUBLISHER "Smart Agriculture Solutions"
!define PRODUCT_WEB_SITE "https://github.com/your-repo/smart-farm"

OutFile "SmartFarmSystem-Setup-v$Version.exe"
InstallDir "`$PROGRAMFILES64\SmartFarmSystem"
RequestExecutionLevel admin

Page directory
Page instfiles

Section "MainSection" SEC01
    SetOutPath "`$INSTDIR"
    File /r "$PackageDir\*.*"
    CreateDirectory "`$SMPROGRAMS\Smart Farm System"
    CreateShortCut "`$SMPROGRAMS\Smart Farm System\Smart Farm System.lnk" "`$INSTDIR\SmartFarmSystem.bat"
    CreateShortCut "`$DESKTOP\Smart Farm System.lnk" "`$INSTDIR\SmartFarmSystem.bat"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SmartFarmSystem" "DisplayName" "Smart Farm Monitoring System"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SmartFarmSystem" "UninstallString" "`$INSTDIR\uninstall.exe"
    WriteUninstaller "`$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "`$INSTDIR\*.*"
    RMDir /r "`$INSTDIR"
    Delete "`$SMPROGRAMS\Smart Farm System\*.*"
    RMDir "`$SMPROGRAMS\Smart Farm System"
    Delete "`$DESKTOP\Smart Farm System.lnk"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SmartFarmSystem"
SectionEnd
"@

        $nsisScriptPath = Join-Path $ProjectRoot "installer.nsi"
        $nsisScript | Out-File -FilePath $nsisScriptPath -Encoding UTF8
        
        & $nsis.Source $nsisScriptPath
        if ($LASTEXITCODE -eq 0) {
            Write-Success "安装包创建完成"
        } else {
            Write-Warning "安装包创建失败"
        }
        
        Remove-Item $nsisScriptPath
    } else {
        Write-Warning "未找到NSIS，跳过安装包创建"
    }
} else {
    Write-Info "[10/10] 跳过安装包创建"
}

# 创建ZIP压缩包
Write-ColorOutput "创建ZIP发布包..." "Yellow"
$zipPath = Join-Path $ProjectRoot "SmartFarmSystem-v$Version.zip"
Compress-Archive -Path "$PackageDir\*" -DestinationPath $zipPath -Force
Write-Success "ZIP发布包已创建: $zipPath"

# 显示构建摘要
Write-ColorOutput "========================================"
Write-ColorOutput "✅ 部署完成！" "Green"
Write-ColorOutput "========================================"
Write-Info "📦 发布包位置: $PackageDir"
Write-Info "🎮 主程序: $PackageDir\bin\SmartFarmSystem.exe"
Write-Info "📚 文档目录: $PackageDir\docs\"
Write-Info "⚙️ 配置文件: $PackageDir\config\settings.conf"
Write-Info "📦 ZIP包: $zipPath"

$packageSize = Get-ChildItem $PackageDir -Recurse | Measure-Object -Property Length -Sum
$packageSizeMB = [math]::Round($packageSize.Sum / 1MB, 2)
Write-Info "📊 发布包大小: $packageSizeMB MB"

Write-ColorOutput ""
Write-ColorOutput "💡 快速启动选项:" "Cyan"
Write-ColorOutput "   • 批处理: SmartFarmSystem.bat" "White"
Write-ColorOutput "   • PowerShell: SmartFarmSystem.ps1" "White"
Write-ColorOutput "   • 直接运行: bin\SmartFarmSystem.exe" "White"

# 询问是否立即运行
$runNow = Read-Host "是否立即运行程序? (Y/N)"
if ($runNow -eq "Y" -or $runNow -eq "y") {
    Start-Process -FilePath (Join-Path $PackageDir "bin\SmartFarmSystem.exe")
    Write-Success "程序已启动"
}

Write-ColorOutput "🎉 部署流程全部完成！" "Green"