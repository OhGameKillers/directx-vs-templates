//
// DeviceResources.h - A wrapper for the device and swapchain
//

#pragma once

namespace DX
{
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        DeviceResources();

        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void SetWindow(HWND window);
        bool WindowSizeChanged();
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) { m_deviceNotify = deviceNotify; };
        void Present();

        // Device Accessors.
        RECT GetOutputSize() const { return m_outputSize; }

        // Direct3D Accessors.
        ID3D11Device*           GetD3DDevice() const                    { return m_d3dDevice.Get(); }
        ID3D11Device1*          GetD3DDevice1() const                   { return m_d3dDevice1.Get(); }
        ID3D11DeviceContext*    GetD3DDeviceContext() const             { return m_d3dContext.Get(); }
        ID3D11DeviceContext1*   GetD3DDeviceContext1() const            { return m_d3dContext1.Get(); }
        IDXGISwapChain*         GetSwapChain() const                    { return m_swapChain.Get(); }
        IDXGISwapChain1*		GetSwapChain1() const                   { return m_swapChain1.Get(); }
        D3D_FEATURE_LEVEL		GetDeviceFeatureLevel() const           { return m_d3dFeatureLevel; }
        ID3D11RenderTargetView*	GetBackBufferRenderTargetView() const   { return m_d3dRenderTargetView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const             { return m_d3dDepthStencilView.Get(); }
        D3D11_VIEWPORT			GetScreenViewport() const               { return m_screenViewport; }

    private:
        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice1;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext1;
        Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
        Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain1;

        // Direct3D rendering objects. Required for 3D.
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_d3dRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_d3dDepthStencilView;
        D3D11_VIEWPORT                                  m_screenViewport;

        // Cached device properties.
        HWND                                            m_window;
        D3D_FEATURE_LEVEL                               m_d3dFeatureLevel;
        RECT                                            m_outputSize;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify* m_deviceNotify;
    };
}