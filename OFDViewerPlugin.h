// OFDViewerPlugin.h
// Directory Opus 13 Viewer Plugin V2 - OFD File Preview Plugin
// Header file for the OFD Viewer Plugin

#pragma once

#include <windows.h>
#include <unknwn.h>
#include <gdiplus.h>

// Forward declarations
class COFDViewerPlugin;
class COFDViewerWindow;

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890} - Replace with your own GUID
DEFINE_GUID(IID_IViewerPlugin2, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// IViewerPlugin2 Interface (Directory Opus 13 Viewer Plugin V2)
interface IViewerPlugin2 : public IUnknown
{
    // Get plugin name
    STDMETHOD(GetName)(BSTR* pbstrName) = 0;
    
    // Get plugin version
    STDMETHOD(GetVersion)(DWORD* pdwVersion) = 0;
    
    // Check if plugin can handle the file
    STDMETHOD(CanView)(LPCWSTR pszPath, DWORD* pdwCanView) = 0;
    
    // Create viewer window
    STDMETHOD(CreateViewerWindow)(HWND hwndParent, LPCWSTR pszPath, 
                                   IViewerWindow2** ppViewerWindow) = 0;
    
    // Get supported file extensions
    STDMETHOD(GetExtensions)(BSTR* pbstrExtensions) = 0;
};

// IViewerWindow2 Interface
interface IViewerWindow2 : public IUnknown
{
    // Get the window handle
    STDMETHOD(GetWindow)(HWND* phwnd) = 0;
    
    // Load the file
    STDMETHOD(LoadFile)(LPCWSTR pszPath) = 0;
    
    // Unload the file
    STDMETHOD(UnloadFile)() = 0;
    
    // Zoom operations
    STDMETHOD(ZoomIn)() = 0;
    STDMETHOD(ZoomOut)() = 0;
    STDMETHOD(ZoomToFit)() = 0;
    STDMETHOD(SetZoom)(double dZoom) = 0;
    STDMETHOD(GetZoom)(double* pdZoom) = 0;
    
    // Navigation
    STDMETHOD(GoToPage)(int nPage) = 0;
    STDMETHOD(GetCurrentPage)(int* pnPage) = 0;
    STDMETHOD(GetPageCount)(int* pnCount) = 0;
    
    // Rendering
    STDMETHOD(Render)(HDC hdc, const RECT* prcBounds) = 0;
    
    // Print support
    STDMETHOD(Print)(HWND hwndOwner, LPCWSTR pszDocName) = 0;
};

// Main Plugin Class
class COFDViewerPlugin : public IViewerPlugin2
{
public:
    COFDViewerPlugin();
    virtual ~COFDViewerPlugin();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IViewerPlugin2 methods
    STDMETHOD(GetName)(BSTR* pbstrName);
    STDMETHOD(GetVersion)(DWORD* pdwVersion);
    STDMETHOD(CanView)(LPCWSTR pszPath, DWORD* pdwCanView);
    STDMETHOD(CreateViewerWindow)(HWND hwndParent, LPCWSTR pszPath, 
                                   IViewerWindow2** ppViewerWindow);
    STDMETHOD(GetExtensions)(BSTR* pbstrExtensions);

private:
    LONG m_cRef;
    Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;
    ULONG_PTR m_gdiplusToken;
};

// Viewer Window Class
class COFDViewerWindow : public IViewerWindow2
{
public:
    COFDViewerWindow(HWND hwndParent);
    virtual ~COFDViewerWindow();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IViewerWindow2 methods
    STDMETHOD(GetWindow)(HWND* phwnd);
    STDMETHOD(LoadFile)(LPCWSTR pszPath);
    STDMETHOD(UnloadFile)();
    STDMETHOD(ZoomIn)();
    STDMETHOD(ZoomOut)();
    STDMETHOD(ZoomToFit)();
    STDMETHOD(SetZoom)(double dZoom);
    STDMETHOD(GetZoom)(double* pdZoom);
    STDMETHOD(GoToPage)(int nPage);
    STDMETHOD(GetCurrentPage)(int* pnPage);
    STDMETHOD(GetPageCount)(int* pnCount);
    STDMETHOD(Render)(HDC hdc, const RECT* prcBounds);
    STDMETHOD(Print)(HWND hwndOwner, LPCWSTR pszDocName);

private:
    LONG m_cRef;
    HWND m_hwnd;
    HWND m_hwndParent;
    WCHAR m_wszFilePath[MAX_PATH];
    double m_dZoom;
    int m_nCurrentPage;
    int m_nPageCount;
    bool m_bLoaded;
    
    // OFD specific members (to be implemented with OFD library)
    void* m_pOFDDocument;  // Placeholder for OFD document handle
    
    // Helper methods
    LRESULT OnPaint(HDC hdc);
    LRESULT OnSize(int width, int height);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Factory class for COM
class COFDViewerPluginFactory : public IClassFactory
{
public:
    COFDViewerPluginFactory();
    virtual ~COFDViewerPluginFactory();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

private:
    LONG m_cRef;
};

// COM GUIDs - Replace with your own GUIDs
// {B1C2D3E4-F5A6-7890-BCDE-F12345678901} - Class ID
DEFINE_GUID(CLSID_OFDViewerPlugin, 0xB1C2D3E4, 0xF5A6, 0x7890, 0xBC, 0xDE, 0xF1, 0x23, 0x45, 0x67, 0x89, 0x01);

// Export functions for DLL
extern "C" {
    STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
    STDAPI DllCanUnloadNow();
    STDAPI DllRegisterServer();
    STDAPI DllUnregisterServer();
}
