# 🚀 智能农场监控系统安装指南

## 📋 目录
1. [系统要求](#系统要求)
2. [快速安装](#快速安装)
3. [手动安装](#手动安装)
4. [依赖库安装](#依赖库安装)
5. [配置设置](#配置设置)
6. [故障排除](#故障排除)
7. [卸载说明](#卸载说明)

---

## 💻 系统要求

### 最低配置
- **操作系统**: Windows 10 (64位) Build 1903 或更高
- **处理器**: Intel i3-6100 / AMD FX-6300 或同等性能
- **内存**: 4GB RAM
- **显卡**: 支持OpenGL 3.3的显卡 (NVIDIA GTX 750 / AMD R7 260X)
- **存储**: 500MB 可用磁盘空间
- **网络**: 可选，用于更新和在线功能

### 推荐配置
- **操作系统**: Windows 11 (64位)
- **处理器**: Intel i5-8400 / AMD Ryzen 5 2600 或更高
- **内存**: 8GB RAM 或更多
- **显卡**: 支持OpenGL 4.6的独立显卡 (NVIDIA GTX 1060 / AMD RX 580)
- **存储**: 1GB 可用磁盘空间 (SSD推荐)
- **网络**: 宽带连接

### 最佳体验配置
- **操作系统**: Windows 11 Pro (64位)
- **处理器**: Intel i7-10700K / AMD Ryzen 7 3700X 或更高
- **内存**: 16GB RAM 或更多
- **显卡**: NVIDIA RTX 3060 / AMD RX 6600 XT 或更高
- **存储**: 2GB 可用磁盘空间 (NVMe SSD)
- **网络**: 千兆网络连接

---

## ⚡ 快速安装

### 方法1: 自动安装程序 (推荐)

1. **下载安装程序**
   - 访问 [GitHub Releases](https://github.com/your-repo/smart-farm/releases)
   - 下载最新的 `SmartFarmSystem-Setup-v2.0.0.exe`

2. **运行安装程序**
   ```
   右键点击安装程序 → 以管理员身份运行
   ```

3. **按照安装向导步骤**
   - 选择安装目录 (默认: `C:\Program Files\SmartFarmSystem`)
   - 确认开始菜单快捷方式创建
   - 选择是否创建桌面快捷方式
   - 点击"安装"开始安装过程

4. **完成安装**
   - 安装完成后点击"完成"
   - 选择是否立即启动程序

### 方法2: 便携版安装

1. **下载便携版**
   - 下载 `SmartFarmSystem-v2.0.0.zip`
   - 解压到任意目录 (建议: `C:\SmartFarmSystem`)

2. **运行程序**
   - 双击 `SmartFarmSystem.bat` 启动
   - 或直接运行 `bin\SmartFarmSystem.exe`

---

## 🔧 手动安装

### 步骤1: 准备环境

1. **检查Windows版本**
   ```cmd
   winver
   ```
   确保版本为Windows 10 Build 1903或更高

2. **检查系统架构**
   ```cmd
   wmic os get osarchitecture
   ```
   必须为64位系统

3. **更新Windows**
   - 打开设置 → 更新和安全 → Windows更新
   - 安装所有可用更新并重启

### 步骤2: 安装运行时依赖

1. **Microsoft Visual C++ 运行时**
   - 下载并安装 [Visual C++ Redistributable for Visual Studio 2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)

2. **.NET Framework (如果需要)**
   - 下载并安装 [.NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/net48)

3. **DirectX运行时**
   ```cmd
   dxdiag
   ```
   确保DirectX版本为12或更高

### 步骤3: 解压程序文件

1. **创建安装目录**
   ```cmd
   mkdir "C:\Program Files\SmartFarmSystem"
   ```

2. **解压文件**
   - 将下载的ZIP文件解压到安装目录
   - 确保文件结构如下：
   ```
   SmartFarmSystem/
   ├── bin/
   │   ├── SmartFarmSystem.exe
   │   ├── Project2.pdb
   │   └── glfw3.dll
   ├── docs/
   ├── config/
   ├── assets/
   ├── README.md
   └── SmartFarmSystem.bat
   ```

### 步骤4: 验证安装

1. **检查文件完整性**
   ```cmd
   cd "C:\Program Files\SmartFarmSystem"
   dir bin\SmartFarmSystem.exe
   ```

2. **测试运行**
   ```cmd
   bin\SmartFarmSystem.exe --version
   ```

---

## 📚 依赖库安装

### OpenGL驱动更新

1. **NVIDIA显卡**
   - 访问 [NVIDIA驱动页面](https://www.nvidia.com/drivers)
   - 下载并安装最新驱动

2. **AMD显卡**
   - 访问 [AMD驱动页面](https://www.amd.com/support)
   - 下载并安装最新驱动

3. **Intel集成显卡**
   - 访问 [Intel显卡驱动页面](https://www.intel.com/content/www/us/en/support/products/80939/graphics.html)
   - 下载并安装最新驱动

### 验证OpenGL支持

运行OpenGL检查工具:
```cmd
bin\SmartFarmSystem.exe --check-opengl
```

预期输出:
```
OpenGL Vendor: NVIDIA Corporation
OpenGL Renderer: NVIDIA GeForce RTX 3060
OpenGL Version: 4.6.0 NVIDIA 456.71
GLSL Version: 4.60 NVIDIA
✅ OpenGL 4.6 支持已确认
```

---

## ⚙️ 配置设置

### 基础配置

编辑 `config\settings.conf` 文件:

```ini
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

[Performance]
enable_lod=true
enable_occlusion_culling=true
shadow_quality=medium
texture_quality=high
```

### 高级配置

1. **性能优化配置** (低端硬件)
   ```ini
   [Graphics]
   resolution_width=1280
   resolution_height=720
   msaa_samples=2
   max_fps=30
   
   [Simulation]
   plant_count=500
   sensor_count=25
   
   [Performance]
   shadow_quality=low
   texture_quality=medium
   enable_bloom=false
   ```

2. **高质量配置** (高端硬件)
   ```ini
   [Graphics]
   resolution_width=1920
   resolution_height=1080
   msaa_samples=8
   max_fps=120
   
   [Simulation]
   plant_count=2000
   sensor_count=100
   
   [Performance]
   shadow_quality=high
   texture_quality=ultra
   enable_bloom=true
   ```

### 命令行参数

```cmd
SmartFarmSystem.exe [选项]

选项:
  --fullscreen          全屏模式启动
  --windowed            窗口模式启动
  --resolution WxH      设置分辨率 (例: --resolution 1920x1080)
  --no-vsync            禁用垂直同步
  --debug               启用调试模式
  --config PATH         指定配置文件路径
  --log-level LEVEL     设置日志级别 (debug|info|warn|error)
  --check-opengl        检查OpenGL支持并退出
  --version             显示版本信息并退出
  --help                显示帮助信息
```

使用示例:
```cmd
# 以1080p全屏模式启动
SmartFarmSystem.exe --fullscreen --resolution 1920x1080

# 启用调试模式和详细日志
SmartFarmSystem.exe --debug --log-level debug

# 使用自定义配置文件
SmartFarmSystem.exe --config "D:\MyConfig\farm.conf"
```

---

## 🔧 故障排除

### 常见问题解决

#### 问题1: 程序无法启动
**症状**: 双击程序没有反应或立即退出

**解决方案**:
1. 检查系统要求是否满足
2. 更新显卡驱动
3. 安装Visual C++运行时
4. 以管理员身份运行

**详细步骤**:
```cmd
# 1. 检查错误日志
type logs\error.log

# 2. 验证依赖
bin\SmartFarmSystem.exe --check-opengl

# 3. 重新安装Visual C++运行时
# 下载并运行 vc_redist.x64.exe
```

#### 问题2: 性能问题/帧率低
**症状**: 程序运行缓慢，帧率低于20FPS

**解决方案**:
1. 降低图形设置
2. 减少模拟对象数量
3. 更新显卡驱动
4. 关闭其他占用GPU的程序

**配置调整**:
```ini
[Performance]
enable_lod=true
shadow_quality=low
texture_quality=medium
enable_bloom=false

[Simulation]
plant_count=300
sensor_count=20
```

#### 问题3: OpenGL错误
**症状**: 黑屏或显示"OpenGL错误"消息

**解决方案**:
```cmd
# 1. 检查OpenGL版本
bin\SmartFarmSystem.exe --check-opengl

# 2. 更新显卡驱动
# NVIDIA: https://www.nvidia.com/drivers
# AMD: https://www.amd.com/support

# 3. 检查是否使用集成显卡
# 在NVIDIA控制面板中设置高性能GPU
```

#### 问题4: 音频问题
**症状**: 没有声音或声音异常

**解决方案**:
1. 检查系统音量设置
2. 更新音频驱动
3. 检查配置文件音频设置

```ini
[Audio]
enable_audio=true
master_volume=0.8
effects_volume=0.6
```

### 日志文件位置

程序运行日志保存在以下位置:
- **错误日志**: `logs\error.log`
- **运行日志**: `logs\runtime.log`
- **调试日志**: `logs\debug.log` (仅调试模式)

### 获取技术支持

如果问题仍未解决，请收集以下信息并联系技术支持:

1. **系统信息**
   ```cmd
   systeminfo > system_info.txt
   dxdiag /t dxdiag_report.txt
   ```

2. **程序版本信息**
   ```cmd
   bin\SmartFarmSystem.exe --version > version_info.txt
   ```

3. **错误日志**
   ```cmd
   copy logs\*.log support_logs\
   ```

4. **配置文件**
   ```cmd
   copy config\settings.conf support_config\
   ```

发送至: support@smartfarm.com

---

## 🗑️ 卸载说明

### 使用安装程序卸载

1. **控制面板卸载**
   - 打开控制面板 → 程序 → 程序和功能
   - 找到"Smart Farm Monitoring System"
   - 点击"卸载"按钮

2. **开始菜单卸载**
   - 开始菜单 → Smart Farm System → 卸载

### 手动卸载

1. **删除程序文件**
   ```cmd
   rmdir /s "C:\Program Files\SmartFarmSystem"
   ```

2. **删除用户数据** (可选)
   ```cmd
   rmdir /s "%APPDATA%\SmartFarmSystem"
   rmdir /s "%LOCALAPPDATA%\SmartFarmSystem"
   ```

3. **删除注册表项** (仅安装程序安装)
   ```cmd
   reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\SmartFarmSystem" /f
   ```

4. **删除快捷方式**
   ```cmd
   del "%PUBLIC%\Desktop\Smart Farm System.lnk"
   del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Smart Farm System\*.*"
   rmdir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Smart Farm System"
   ```

### 清理残留文件

运行清理脚本:
```cmd
@echo off
echo 正在清理Smart Farm System残留文件...
rmdir /s /q "C:\Program Files\SmartFarmSystem" 2>nul
rmdir /s /q "%APPDATA%\SmartFarmSystem" 2>nul
rmdir /s /q "%LOCALAPPDATA%\SmartFarmSystem" 2>nul
del "%PUBLIC%\Desktop\Smart Farm System.lnk" 2>nul
echo 清理完成！
pause
```

---

## 📞 联系支持

- **技术支持**: support@smartfarm.com
- **Bug报告**: https://github.com/your-repo/smart-farm/issues
- **用户社区**: https://discord.gg/smartfarm
- **在线文档**: https://docs.smartfarm.com

---

*最后更新: 2024年7月19日*