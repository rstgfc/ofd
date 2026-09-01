# OFD Viewer Plugin for Directory Opus 13

这是一个符合 **Directory Opus 13 Viewer Plugin V2** 规范的 OFD 文件预览插件，支持真实的 OFD 文件渲染预览。

## 重要更新

本版本已实现完整的 OFD 渲染功能，通过外部转换器工具将 OFD 页面转换为 PNG 图像进行显示。

## 工作原理

插件采用以下架构实现 OFD 预览：

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  Directory Opus │───▶│  OFDViewerPlugin │───▶│  ofd2png.exe    │
│     Viewer      │◀───│     (DLL)        │◀───│  (转换器工具)   │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │                        │
                              ▼                        ▼
                       GDI+ Bitmap            PNG 临时文件
                              │                        │
                              └────────────────────────┘
```

1. **LoadFile**: 加载 OFD 文件路径，检测页数
2. **Render**: 调用外部转换器生成 PNG → 加载到 GDI+ Bitmap → 绘制到窗口
3. **缓存机制**: LRU 缓存最近访问的页面位图，提高性能

## 支持的 OFD 转换器工具

插件支持任何符合以下命令行格式的 OFD 转 PNG 工具：

```bash
ofd2png.exe "input.ofd" "output.png" [pageIndex]
```

### 推荐工具

#### 1. ofd2png (开源/免费)
- GitHub: https://github.com/klmhuang/ofd2png
- 或使用其他类似的开源项目

#### 2. 数科网维 OFD 阅读器附带工具
- 商业软件，提供命令行工具

#### 3. 自行编译 libofd
```bash
git clone https://github.com/libofd/libofd
# 按照 libofd 文档编译，并创建命令行包装器
```

### 自定义转换器

如果您有自己的 OFD 渲染工具，只需确保它支持上述命令行格式，然后：

1. 将可执行文件命名为 `ofd2png.exe`
2. 或者修改 `OFDViewerPlugin.cpp` 中的 `CONVERTER_TOOL` 宏定义

## 安装步骤

### 1. 编译插件 DLL

```batch
cl /LD /EHsc /O2 OFDViewerPlugin.cpp ^
   /link /OUT:OFDViewerPlugin.dll ^
   gdiplus.lib oleaut32.lib shlwapi.lib uuid.lib ^
   /DEF:OFDViewerPlugin.def
```

### 2. 准备转换器工具

将 `ofd2png.exe`（或您选择的转换器）放在与 `OFDViewerPlugin.dll` **相同的目录**中。

### 3. 注册插件

以管理员身份运行：

```batch
regsvr32 OFDViewerPlugin.dll
```

### 4. 在 Directory Opus 13 中使用

1. 打开 Directory Opus 13
2. 导航到包含 .ofd 文件的文件夹
3. 启用查看器面板（View → Viewer Panel）
4. 点击任意 .ofd 文件，即可看到预览

## 核心功能

### 页面渲染流程

```cpp
// 1. LoadFile 时预渲染第一页
LoadFile(L"document.ofd") {
    m_currentOFDPath = L"document.ofd";
    EnsurePageRendered(0);  // 异步渲染第 0 页
}

// 2. Render 时从缓存获取位图
Render(hdc, rect) {
    Bitmap* bmp = GetCachedBitmap(m_nCurrentPage);
    if (bmp) {
        graphics.DrawImage(bmp, ...);  // 使用 GDI+ 绘制
    } else {
        // 显示"正在渲染..."提示
        EnsurePageRendered(m_nCurrentPage);
    }
}

// 3. 页面切换时
GoToPage(pageIndex) {
    m_nCurrentPage = pageIndex;
    EnsurePageRendered(pageIndex);  // 确保目标页已渲染
    InvalidateRect();  // 触发重绘
}
```

### 缓存机制

- **最大缓存数**: 10 页（可配置 `MAX_CACHE_SIZE`）
- **淘汰策略**: LRU（最近最少使用）
- **存储内容**: GDI+ Bitmap 对象 + 临时 PNG 文件路径

### 缩放支持

- 支持 0.1x 到 4.0x 缩放
- 使用高质量双三次插值算法
- ZoomToFit 自动计算最佳缩放比例

## 项目结构

```
OFDViewerPlugin/
├── OFDViewerPlugin.h      # COM 接口定义和类声明
├── OFDViewerPlugin.cpp    # 完整实现（含渲染、缓存逻辑）
├── OFDViewerPlugin.def    # DLL 导出定义
├── CMakeLists.txt         # CMake 构建配置
├── README.md              # 本说明文档
└── ofd2png.exe            # [需自备] OFD 转 PNG 转换器
```

## API 参考

### IViewerPlugin2 接口

| 方法 | 说明 |
|------|------|
| `GetName` | 返回 "OFD Viewer Plugin" |
| `GetVersion` | 返回版本号 (1.0.0.0) |
| `CanView` | 检查 .ofd 扩展名 |
| `CreateViewerWindow` | 创建 COFDViewerWindow 实例 |
| `GetExtensions` | 返回 "*.ofd" |

### IViewerWindow2 接口

| 方法 | 说明 |
|------|------|
| `GetWindow` | 返回子窗口句柄 |
| `LoadFile` | 加载 OFD 文件，启动渲染 |
| `UnloadFile` | 卸载文件，清空缓存 |
| `ZoomIn/Out` | 缩放 ±25% |
| `ZoomToFit` | 适应窗口大小 |
| `SetZoom/GetZoom` | 设置/获取缩放比例 |
| `GoToPage` | 跳转页面并触发渲染 |
| `GetCurrentPage` | 返回当前页码（0-based） |
| `GetPageCount` | 返回总页数 |
| `Render` | 使用 GDI+ 绘制缓存的位图 |
| `Print` | 打印支持（待实现） |

## 高级配置

### 修改缓存大小

编辑 `OFDViewerPlugin.cpp`:

```cpp
#define MAX_CACHE_SIZE 20  // 增加缓存页数
```

### 修改临时文件位置

编辑 `GetTempFilePath()` 函数:

```cpp
std::wstring GetTempFilePath(int pageIndex)
{
    // 改为固定目录而非系统临时目录
    return L"C:\\OFDCache\\" + std::to_wstring(pageIndex) + L".png";
}
```

### 支持更多文件格式

编辑 `CanView()` 和 `GetExtensions()`:

```cpp
// 添加对 .ofd 以外的格式支持
if (_wcsicmp(pExt, L".ofd") == 0 || _wcsicmp(pExt, L".xxx") == 0)
```

## 故障排除

### 问题：显示"Rendering Page X of Y..."但不显示内容

**原因**: 转换器工具未找到或执行失败

**解决方案**:
1. 确认 `ofd2png.exe` 与 DLL 在同一目录
2. 手动测试转换器：
   ```batch
   ofd2png.exe "test.ofd" "test.png" 0
   ```
3. 检查转换器是否支持您的 OFD 文件版本

### 问题：页面切换缓慢

**原因**: 每次切换都重新渲染

**解决方案**:
1. 增加 `MAX_CACHE_SIZE`
2. 优化转换器性能
3. 考虑使用更快的 OFD 渲染库

### 问题：内存占用过高

**原因**: 缓存位图过多

**解决方案**:
1. 减少 `MAX_CACHE_SIZE`
2. 确保 LRU 缓存正常工作
3. 及时调用 `UnloadFile()` 释放资源

## 集成其他 OFD 库

如果希望直接集成 OFD 渲染库（而非外部工具），可以参考代码中的 `USE_LIBOFD_DIRECTLY` 条件编译块：

```cpp
#ifdef USE_LIBOFD_DIRECTLY
#include <libofd.h>
// 直接调用 libofd API 进行渲染
#endif
```

## 许可证

本项目代码采用 MIT 许可证。

## 致谢

- Directory Opus 团队提供的 Plugin SDK
- 所有 OFD 开源项目的贡献者
