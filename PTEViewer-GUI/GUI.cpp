#include "GUI.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

GUI* GUI::CreateGUI()
{
    auto gui = new GUI();
    if (!gui) // Failed to allocate.
        return nullptr;

    if (!gui->SetupWindow()
        || !gui->SetupDX11()
        || !gui->SetupImGui())
    {
        delete gui;
        return nullptr;
    }

    return gui;
}
void GUI::DestroyGUI()
{
    DestroyImGui();
    DestroyRenderTarget();
    DestroyDX11();
    DestroyWindow();
}

void GUI::Render(std::function<void()> DrawCallback)
{
    ::ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(m_hWnd);

    bool Finished = false;
    while (!Finished)
    {
        Finished = HandleMessage();
        if (Finished)
            break;
        if (HandleOcclusion())
            continue;
        HandleResize();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        DrawCallback();

        constexpr ImVec4 ClearColor = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        constexpr float ClearColorWithAlpha[4] = { ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w };

        ImGui::Render();
        m_pD3DDeviceContext->OMSetRenderTargets(1, &m_MainRenderTargetView, nullptr);
        m_pD3DDeviceContext->ClearRenderTargetView(m_MainRenderTargetView, ClearColorWithAlpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = m_pSwapChain->Present(1, 0);
        m_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
}

bool GUI::HandleMessage()
{
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            return true;
    }

    return false;
}
bool GUI::HandleOcclusion()
{
    if (m_SwapChainOccluded && m_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
    {
        ::Sleep(10);
        return true;
    }
    m_SwapChainOccluded = false;

    return false;
}
void GUI::HandleResize()
{
    if (m_ResizeWidth && m_ResizeHeight)
    {
        DestroyRenderTarget();
        m_pSwapChain->ResizeBuffers(0, m_ResizeWidth, m_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        m_ResizeWidth = m_ResizeHeight = 0;
        CreateRenderTarget();
    }
}

bool GUI::SetupWindow()
{
    m_WC = { sizeof(m_WC), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"PTEViewer-WndClss", nullptr };
    if (::RegisterClassExW(&m_WC) == INVALID_ATOM)
        return false;

    m_hWnd = ::CreateWindowW(m_WC.lpszClassName, L"PTEViewer-GUI", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, m_WC.hInstance, nullptr);
    if (!m_hWnd)
        return false;

    return true;
}
bool GUI::SetupDX11()
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT CreateDeviceFlags = 0;
#ifdef _DEBUG
    CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL FeatureLevel;
    const D3D_FEATURE_LEVEL FeatureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT Res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CreateDeviceFlags, FeatureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pD3DDevice, &FeatureLevel, &m_pD3DDeviceContext);
    if (Res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        Res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, CreateDeviceFlags, FeatureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pD3DDevice, &FeatureLevel, &m_pD3DDeviceContext);
    if (Res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}
bool GUI::SetupImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(m_hWnd))
        return false;
    if (!ImGui_ImplDX11_Init(m_pD3DDevice, m_pD3DDeviceContext))
        return false;

    return true;
}

void GUI::DestroyWindow()
{
    ::DestroyWindow(m_hWnd);
    ::UnregisterClassW(m_WC.lpszClassName, m_WC.hInstance);
}
void GUI::DestroyDX11()
{
    if (m_pSwapChain)
    {
        m_pSwapChain->Release();
        m_pSwapChain = nullptr;
    }
    if (m_pD3DDeviceContext)
    {
        m_pD3DDeviceContext->Release();
        m_pD3DDeviceContext = nullptr;
    }
    if (m_pD3DDevice)
    {
        m_pD3DDevice->Release();
        m_pD3DDevice = nullptr;
    }
}
void GUI::DestroyImGui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void GUI::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pD3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_MainRenderTargetView);
    pBackBuffer->Release();
}
void GUI::DestroyRenderTarget()
{
    if (m_MainRenderTargetView)
    {
        m_MainRenderTargetView->Release();
        m_MainRenderTargetView = nullptr;
    }
}

LRESULT WINAPI GUI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        GUI::m_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        GUI::m_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
