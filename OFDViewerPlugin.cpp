// OFDViewerPlugin.cpp
// Directory Opus 13 Viewer Plugin V2 - OFD File Preview Plugin
// Implementation file for the OFD Viewer Plugin

#include "OFDViewerPlugin.h"
#include <comdef.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <cstring>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uuid.lib")

// Global variables
HINSTANCE g_hInstance = NULL;
LONG g_cDllRef = 0;

// ============================================================================
// COFDViewerPlugin Implementation
// ============================================================================

COFDViewerPlugin::COFDViewerPlugin() : m_cRef(1)
{
    // Initialize GDI+
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);
}

COFDViewerPlugin::~COFDViewerPlugin()
{
    Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

STDMETHODIMP COFDViewerPlugin::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IUnknown || riid == IID_IViewerPlugin2)
    {
        *ppvObject = static_cast<IViewerPlugin2*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) COFDViewerPlugin::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COFDViewerPlugin::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP COFDViewerPlugin::GetName(BSTR* pbstrName)
{
    if (!pbstrName)
        return E_POINTER;
    
    *pbstrName = SysAllocString(L"OFD Viewer Plugin");
    return (*pbstrName != NULL) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP COFDViewerPlugin::GetVersion(DWORD* pdwVersion)
{
    if (!pdwVersion)
        return E_POINTER;
    
    // Version format: Major.Minor.Build.Revision (packed into DWORD)
    // Example: 1.0.0.0 = 0x00010000
    *pdwVersion = 0x00010000;  // Version 1.0
    return S_OK;
}

STDMETHODIMP COFDViewerPlugin::CanView(LPCWSTR pszPath, DWORD* pdwCanView)
{
    if (!pszPath || !pdwCanView)
        return E_POINTER;
    
    *pdwCanView = FALSE;
    
    // Check file extension
    LPCWSTR pExt = PathFindExtensionW(pszPath);
    if (pExt && _wcsicmp(pExt, L".ofd") == 0)
    {
        // Verify file exists
        if (PathFileExistsW(pszPath))
        {
            *pdwCanView = TRUE;
        }
    }
    
    return S_OK;
}

STDMETHODIMP COFDViewerPlugin::CreateViewerWindow(HWND hwndParent, LPCWSTR pszPath, 
                                                   IViewerWindow2** ppViewerWindow)
{
    if (!hwndParent || !pszPath || !ppViewerWindow)
        return E_POINTER;
    
    *ppViewerWindow = NULL;
    
    try
    {
        COFDViewerWindow* pViewerWindow = new COFDViewerWindow(hwndParent);
        if (!pViewerWindow)
            return E_OUTOFMEMORY;
        
        HRESULT hr = pViewerWindow->QueryInterface(IID_IViewerWindow2, (void**)ppViewerWindow);
        if (FAILED(hr))
        {
            delete pViewerWindow;
            return hr;
        }
        
        // Load the file if path is provided
        if (pszPath && wcslen(pszPath) > 0)
        {
            pViewerWindow->LoadFile(pszPath);
        }
        
        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

STDMETHODIMP COFDViewerPlugin::GetExtensions(BSTR* pbstrExtensions)
{
    if (!pbstrExtensions)
        return E_POINTER;
    
    // Return supported extensions (semicolon-separated)
    *pbstrExtensions = SysAllocString(L"*.ofd");
    return (*pbstrExtensions != NULL) ? S_OK : E_OUTOFMEMORY;
}

// ============================================================================
// COFDViewerWindow Implementation
// ============================================================================

COFDViewerWindow::COFDViewerWindow(HWND hwndParent) 
    : m_cRef(1)
    , m_hwnd(NULL)
    , m_hwndParent(hwndParent)
    , m_dZoom(1.0)
    , m_nCurrentPage(0)
    , m_nPageCount(0)
    , m_bLoaded(false)
    , m_pOFDDocument(NULL)
{
    wcscpy_s(m_wszFilePath, MAX_PATH, L"");
    
    // Create a child window for rendering
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = COFDViewerWindow::WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"OFDViewerWindowClass";
    
    RegisterClassExW(&wc);
    
    m_hwnd = CreateWindowExW(
        0,
        L"OFDViewerWindowClass",
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 100, 100,
        m_hwndParent,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (m_hwnd)
    {
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}

COFDViewerWindow::~COFDViewerWindow()
{
    UnloadFile();
    
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

STDMETHODIMP COFDViewerWindow::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IUnknown || riid == IID_IViewerWindow2)
    {
        *ppvObject = static_cast<IViewerWindow2*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) COFDViewerWindow::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COFDViewerWindow::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP COFDViewerWindow::GetWindow(HWND* phwnd)
{
    if (!phwnd)
        return E_POINTER;
    
    *phwnd = m_hwnd;
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::LoadFile(LPCWSTR pszPath)
{
    if (!pszPath)
        return E_POINTER;
    
    // Unload any previously loaded file
    UnloadFile();
    
    // Store the file path
    wcscpy_s(m_wszFilePath, MAX_PATH, pszPath);
    
    // TODO: Implement actual OFD file loading
    // This requires an OFD SDK/library such as:
    // - Suwell OFD SDK
    // - Tongxin OFD Library
    // - Or use libofd (open source)
    
    // Placeholder implementation:
    // In a real implementation, you would:
    // 1. Open the OFD file (which is a ZIP archive containing XML and resources)
    // 2. Parse the OFD document structure
    // 3. Render pages to bitmaps or GDI+ objects
    
    m_bLoaded = true;
    m_nPageCount = 1;  // Placeholder - actual page count from OFD
    m_nCurrentPage = 0;
    
    // Trigger repaint
    if (m_hwnd)
    {
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::UnloadFile()
{
    m_bLoaded = false;
    m_nPageCount = 0;
    m_nCurrentPage = 0;
    wcscpy_s(m_wszFilePath, MAX_PATH, L"");
    
    // TODO: Free OFD document resources
    if (m_pOFDDocument)
    {
        // Close OFD document
        m_pOFDDocument = NULL;
    }
    
    if (m_hwnd)
    {
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::ZoomIn()
{
    if (m_dZoom < 4.0)
    {
        m_dZoom *= 1.25;
        if (m_hwnd)
        {
            InvalidateRect(m_hwnd, NULL, TRUE);
        }
    }
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::ZoomOut()
{
    if (m_dZoom > 0.1)
    {
        m_dZoom /= 1.25;
        if (m_hwnd)
        {
            InvalidateRect(m_hwnd, NULL, TRUE);
        }
    }
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::ZoomToFit()
{
    if (m_hwnd)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Calculate zoom to fit window
        // TODO: Use actual page dimensions from OFD
        int pageWidth = 595;   // A4 width in points (placeholder)
        int pageHeight = 842;  // A4 height in points (placeholder)
        
        double zoomX = (double)(rc.right - rc.left) / pageWidth;
        double zoomY = (double)(rc.bottom - rc.top) / pageHeight;
        
        m_dZoom = min(zoomX, zoomY);
        
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::SetZoom(double dZoom)
{
    if (dZoom < 0.1 || dZoom > 4.0)
        return E_INVALIDARG;
    
    m_dZoom = dZoom;
    
    if (m_hwnd)
    {
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::GetZoom(double* pdZoom)
{
    if (!pdZoom)
        return E_POINTER;
    
    *pdZoom = m_dZoom;
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::GoToPage(int nPage)
{
    if (!m_bLoaded)
        return E_FAIL;
    
    if (nPage < 0 || nPage >= m_nPageCount)
        return E_INVALIDARG;
    
    m_nCurrentPage = nPage;
    
    if (m_hwnd)
    {
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::GetCurrentPage(int* pnPage)
{
    if (!pnPage)
        return E_POINTER;
    
    *pnPage = m_nCurrentPage;
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::GetPageCount(int* pnCount)
{
    if (!pnCount)
        return E_POINTER;
    
    *pnCount = m_nPageCount;
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::Render(HDC hdc, const RECT* prcBounds)
{
    if (!hdc)
        return E_POINTER;
    
    if (!m_bLoaded)
        return S_OK;
    
    // TODO: Implement actual OFD rendering
    // This should render the current page to the specified DC
    
    // Placeholder: Draw a simple rectangle with text
    RECT rc = *prcBounds;
    
    // Fill background
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);
    
    // Draw border
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Draw placeholder text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    WCHAR szText[256];
    swprintf_s(szText, L"OFD Document\r\nPage %d of %d\r\n%s", 
               m_nCurrentPage + 1, m_nPageCount, m_wszFilePath);
    
    DrawTextW(hdc, szText, -1, &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    
    return S_OK;
}

STDMETHODIMP COFDViewerWindow::Print(HWND hwndOwner, LPCWSTR pszDocName)
{
    if (!pszDocName)
        return E_POINTER;
    
    // TODO: Implement printing support
    // This should send the OFD document to the printer
    
    // Placeholder: Show message
    MessageBoxW(hwndOwner, L"Print functionality not yet implemented.\r\n\r\n"
                L"To implement printing, you need to:\r\n"
                L"1. Access the Windows Print API\r\n"
                L"2. Render each page to a printable format\r\n"
                L"3. Send pages to the selected printer",
                L"OFD Viewer - Print", MB_ICONINFORMATION | MB_OK);
    
    return S_OK;
}

LRESULT COFDViewerWindow::OnPaint(HDC hdc)
{
    if (!hdc)
        return 0;
    
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    
    Render(hdc, &rc);
    
    return 0;
}

LRESULT COFDViewerWindow::OnSize(int width, int height)
{
    // Handle window resize
    if (m_bLoaded && m_hwnd)
    {
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
    return 0;
}

LRESULT CALLBACK COFDViewerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    COFDViewerWindow* pThis = reinterpret_cast<COFDViewerWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    
    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (pThis)
            {
                pThis->OnPaint(hdc);
            }
            EndPaint(hwnd, &ps);
        }
        return 0;
        
    case WM_SIZE:
        if (pThis)
        {
            pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
        
    case WM_ERASEBKGND:
        return 1;  // Prevent flicker
        
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEWHEEL:
        // Handle mouse input for navigation/zooming
        return 0;
        
    case WM_KEYDOWN:
        // Handle keyboard shortcuts
        if (pThis && wParam == VK_SPACE)
        {
            // Space bar: next page
            int nCurrentPage, nPageCount;
            pThis->GetCurrentPage(&nCurrentPage);
            pThis->GetPageCount(&nPageCount);
            if (nCurrentPage < nPageCount - 1)
            {
                pThis->GoToPage(nCurrentPage + 1);
            }
        }
        return 0;
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================================
// COFDViewerPluginFactory Implementation
// ============================================================================

COFDViewerPluginFactory::COFDViewerPluginFactory() : m_cRef(1)
{
}

COFDViewerPluginFactory::~COFDViewerPluginFactory()
{
}

STDMETHODIMP COFDViewerPluginFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppvObject = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) COFDViewerPluginFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COFDViewerPluginFactory::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

STDMETHODIMP COFDViewerPluginFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;
    
    COFDViewerPlugin* pPlugin = new COFDViewerPlugin();
    if (!pPlugin)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pPlugin->QueryInterface(riid, ppvObject);
    if (FAILED(hr))
    {
        delete pPlugin;
    }
    
    return hr;
}

STDMETHODIMP COFDViewerPluginFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        InterlockedIncrement(&g_cDllRef);
    }
    else
    {
        InterlockedDecrement(&g_cDllRef);
    }
    return S_OK;
}

// ============================================================================
// DLL Export Functions
// ============================================================================

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (rclsid != CLSID_OFDViewerPlugin)
        return CLASS_E_CLASSNOTAVAILABLE;
    
    COFDViewerPluginFactory* pFactory = new COFDViewerPluginFactory();
    if (!pFactory)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    if (FAILED(hr))
    {
        delete pFactory;
    }
    
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return (g_cDllRef == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer()
{
    // Register the COM server
    HKEY hKey = NULL;
    HKEY hKeyClass = NULL;
    HKEY hKeyInproc = NULL;
    
    WCHAR szModule[MAX_PATH];
    GetModuleFileNameW(g_hInstance, szModule, MAX_PATH);
    
    // Convert CLSID to string
    OLECHAR szCLSID[64];
    StringFromGUID2(CLSID_OFDViewerPlugin, szCLSID, 64);
    
    // Create HKCR\CLSID\{CLSID}
    WCHAR szKey[128];
    swprintf_s(szKey, L"CLSID\\%s", szCLSID);
    
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szKey, 0, NULL, 0, 
                        KEY_WRITE, NULL, &hKeyClass, NULL) != ERROR_SUCCESS)
    {
        return SELFREG_E_CLASS;
    }
    
    // Set default value (plugin name)
    RegSetValueExW(hKeyClass, NULL, 0, REG_SZ, 
                   (BYTE*)L"OFD Viewer Plugin for Directory Opus 13", 
                   sizeof(L"OFD Viewer Plugin for Directory Opus 13"));
    
    // Create HKCR\CLSID\{CLSID}\InprocServer32
    if (RegCreateKeyExW(hKeyClass, L"InprocServer32", 0, NULL, 0,
                        KEY_WRITE, NULL, &hKeyInproc, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyClass);
        return SELFREG_E_CLASS;
    }
    
    // Set DLL path
    RegSetValueExW(hKeyInproc, NULL, 0, REG_SZ, 
                   (BYTE*)szModule, (wcslen(szModule) + 1) * sizeof(WCHAR));
    
    // Set ThreadingModel
    RegSetValueExW(hKeyInproc, L"ThreadingModel", 0, REG_SZ, 
                   (BYTE*)L"Apartment", sizeof(L"Apartment"));
    
    RegCloseKey(hKeyInproc);
    RegCloseKey(hKeyClass);
    
    return S_OK;
}

STDAPI DllUnregisterServer()
{
    // Unregister the COM server
    OLECHAR szCLSID[64];
    StringFromGUID2(CLSID_OFDViewerPlugin, szCLSID, 64);
    
    WCHAR szKey[128];
    swprintf_s(szKey, L"CLSID\\%s", szCLSID);
    
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKey);
    
    return S_OK;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        break;
        
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    
    return TRUE;
}
