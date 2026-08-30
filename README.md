# OFD Viewer Plugin for Directory Opus 13

这是一个符合 **Directory Opus 13 Viewer Plugin V2** 规范的 OFD 文件预览插件。

## 项目结构

```
OFDViewerPlugin/
├── OFDViewerPlugin.h      # 头文件，包含接口定义和类声明
├── OFDViewerPlugin.cpp    # 实现文件，包含所有功能实现
├── OFDViewerPlugin.def    # 模块定义文件，用于 DLL 导出
├── CMakeLists.txt         # CMake 构建配置（可选）
└── README.md              # 本说明文档
```

## 功能特性

- ✅ 支持 .ofd 文件扩展名检测
- ✅ COM 组件架构，符合 Directory Opus 13 插件规范
- ✅ IViewerPlugin2 接口实现
- ✅ IViewerWindow2 接口实现
- ✅ 缩放功能（放大、缩小、适应窗口）
- ✅ 页面导航（上一页、下一页）
- ✅ 渲染支持（GDI+）
- ✅ 打印接口预留
- ✅ 自动注册/注销 COM 组件

## 编译要求

### 系统要求
- Windows 10/11
- Visual Studio 2019 或更高版本
- Windows SDK 10.0.17763.0 或更高版本

### 依赖库
- GDI+ (gdiplus.lib)
- OLE Automation (oleaut32.lib)
- Shell Light Weight Utility (shlwapi.lib)
- UUID Library (uuid.lib)

## 编译方法

### 方法一：使用 Visual Studio

1. 创建一个新的 Win32 DLL 项目
2. 将 `OFDViewerPlugin.h` 和 `OFDViewerPlugin.cpp` 添加到项目中
3. 在项目属性中配置：
   - 平台工具集：Visual Studio 2019 (v142) 或更高
   - 字符集：使用 Unicode
   - 运行时库：多线程 DLL (/MD)
4. 添加链接器依赖：
   ```
   gdiplus.lib
   oleaut32.lib
   shlwapi.lib
   uuid.lib
   ```
5. 编译生成 `OFDViewerPlugin.dll`

### 方法二：使用命令行（Developer Command Prompt）

```batch
cl /LD /EHsc /O2 /I"%WindowsSdkDir%Include\%WindowsSDKVersion%\um" ^
   /I"%WindowsSdkDir%Include\%WindowsSDKVersion%\shared" ^
   OFDViewerPlugin.cpp ^
   /link /OUT:OFDViewerPlugin.dll ^
   gdiplus.lib oleaut32.lib shlwapi.lib uuid.lib ^
   /DEF:OFDViewerPlugin.def
```

## 安装方法

### 自动注册（推荐）

以管理员身份运行命令提示符：

```batch
regsvr32 OFDViewerPlugin.dll
```

### 手动注册

1. 将 `OFDViewerPlugin.dll` 复制到系统目录（如 `C:\Windows\System32`）
2. 运行 `regsvr32 OFDViewerPlugin.dll`

### 卸载

```batch
regsvr32 /u OFDViewerPlugin.dll
```

## 在 Directory Opus 13 中使用

1. 确保插件已正确注册
2. 打开 Directory Opus 13
3. 导航到包含 .ofd 文件的文件夹
4. 在查看器面板中选择 .ofd 文件
5. 插件将自动加载并显示 OFD 文档预览

## 接口说明

### IViewerPlugin2 接口

| 方法 | 说明 |
|------|------|
| `GetName` | 获取插件名称 |
| `GetVersion` | 获取插件版本 |
| `CanView` | 检查是否可以预览指定文件 |
| `CreateViewerWindow` | 创建查看器窗口 |
| `GetExtensions` | 获取支持的扩展名列表 |

### IViewerWindow2 接口

| 方法 | 说明 |
|------|------|
| `GetWindow` | 获取窗口句柄 |
| `LoadFile` | 加载 OFD 文件 |
| `UnloadFile` | 卸载当前文件 |
| `ZoomIn` | 放大 |
| `ZoomOut` | 缩小 |
| `ZoomToFit` | 适应窗口大小 |
| `SetZoom` | 设置缩放比例 |
| `GetZoom` | 获取当前缩放比例 |
| `GoToPage` | 跳转到指定页 |
| `GetCurrentPage` | 获取当前页码 |
| `GetPageCount` | 获取总页数 |
| `Render` | 渲染到指定 DC |
| `Print` | 打印文档 |

## 集成真实 OFD 渲染引擎

当前实现使用占位符渲染。**要支持真实的 OFD 文件渲染**，您需要集成以下任一 OFD 库：

### 选项 1：数科 OFD SDK（商业）
```cpp
#include "SuwellOFD.h"

// 在 LoadFile 中：
m_pOFDDocument = OFD_Open(pszPath);
m_nPageCount = OFD_GetPageCount(m_pOFDDocument);

// 在 Render 中：
HBITMAP hBmp = OFD_RenderPageToBitmap(m_pOFDDocument, m_nCurrentPage);
// 将位图绘制到 hdc
```

### 选项 2：同心 OFD 库（商业）
```cpp
#include "TongxinOFD.h"
```

### 选项 3：libofd（开源）
```cpp
#include "libofd.h"
```

### 选项 4：自行解析 OFD（OFD 是基于 ZIP/XML 的开放格式）
```cpp
// OFD 文件本质是 ZIP 压缩包
// 1. 解压 OFD 文件
// 2. 解析 DocRoot.xml 获取文档结构
// 3. 解析页面 XML 文件
// 4. 渲染矢量图形和文本
```

## 自定义 GUID

**重要：** 在发布前，请生成您自己的 GUID 替换代码中的示例 GUID：

1. 运行 `guidgen.exe` 或使用 PowerShell：
   ```powershell
   [guid]::NewGuid()
   ```

2. 替换以下位置：
   - `IID_IViewerPlugin2`（头文件中）
   - `CLSID_OFDViewerPlugin`（头文件和 cpp 文件中）

## 故障排除

### 问题：插件未加载
- 确认 DLL 已正确注册（`regsvr32`）
- 检查 Directory Opus 13 的插件管理器
- 查看 Windows 事件日志获取错误信息

### 问题：无法预览 OFD 文件
- 确认文件扩展名为 `.ofd`
- 检查文件是否存在且可读
- 确认已集成 OFD 渲染库

### 问题：显示空白或占位符
- 当前版本需要集成真实的 OFD 渲染库
- 请参考"集成真实 OFD 渲染引擎"部分

## 许可证

本项目代码采用 MIT 许可证。请注意，OFD 渲染可能需要第三方库，这些库可能有自己的许可条款。

## 参考资料

- [Directory Opus Plugin SDK](https://www.gpsoft.com.au/)
- [OFD 国家标准 (GB/T 33190-2016)](http://www.ofd.cn/)
- [COM 编程指南](https://docs.microsoft.com/en-us/windows/win32/com)

## 联系方式

如有问题或建议，请联系开发者。
