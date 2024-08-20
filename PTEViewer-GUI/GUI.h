#pragma once
#include <Windows.h>
#include <functional>

#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")

class GUI
{
public:
    GUI() = default;
    ~GUI() = default;

private:
    HWND m_hWnd;
    WNDCLASSEXW m_WC;
    ID3D11Device* m_pD3DDevice;
    ID3D11DeviceContext* m_pD3DDeviceContext;
    IDXGISwapChain* m_pSwapChain;
    bool m_SwapChainOccluded;
    ID3D11RenderTargetView* m_MainRenderTargetView;
    static inline UINT m_ResizeWidth, m_ResizeHeight;

public:
    static GUI* CreateGUI();
    void DestroyGUI();

private:
    bool SetupWindow();
    bool SetupDX11();
    bool SetupImGui();

    void DestroyWindow();
    void DestroyDX11();
    void DestroyImGui();

    void CreateRenderTarget();
    void DestroyRenderTarget();

public:
    void Render(std::function<void()> DrawCallback);

private:
    bool HandleMessage();
    bool HandleOcclusion();
    void HandleResize();

private:
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
