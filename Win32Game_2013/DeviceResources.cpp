//
// DeviceResources.cpp - A wrapper for the device and swapchain
//

#include "pch.h"
#include "DeviceResources.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
            0,
            D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
            nullptr,                    // Any feature level will do.
            0,
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
            nullptr,                    // No need to keep the D3D device reference.
            nullptr,                    // No need to know the feature level.
            nullptr                     // No need to keep the D3D device context reference.
            );

        return SUCCEEDED(hr);
    }
#endif
};

// Constructor for DeviceResources.
DX::DeviceResources::DeviceResources() : 
    m_window(0),
    m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
    m_deviceNotify(nullptr)
{
    memset(&m_screenViewport, 0, sizeof(D3D11_VIEWPORT));

    m_outputSize.top = m_outputSize.left = 0;
    m_outputSize.bottom = m_outputSize.right = 1;
}

// Configures the Direct3D device, and stores handles to it and the device context.
void DX::DeviceResources::CreateDeviceResources() 
{
    UINT creationFlags = 0;

#if defined(_DEBUG)
    if (SdkLayersAvailable())
    {
        // If the project is in a debug build, enable debugging via SDK Layers with this flag.
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif

    // This array defines the set of DirectX hardware feature levels this app will support.
    // Note the ordering should be preserved.
    // Don't forget to declare your application's minimum required feature level in its
    // description.  All applications are assumed to support 9.1 unless otherwise stated.
    D3D_FEATURE_LEVEL featureLevels[] = 
    {
        // TODO: Modify for supported Direct3D feature levels (see code below related to 11.1 fallback handling)
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Create the Direct3D 11 API device object and a corresponding context.
    HRESULT hr = D3D11CreateDevice(
        nullptr,					            // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,	            // Create a device using the hardware graphics driver.
        0,							            // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        creationFlags,				            // Set debug and Direct2D compatibility flags.
        featureLevels,				            // List of feature levels this app can support.
        _countof(featureLevels),	            // Size of the list above.
        D3D11_SDK_VERSION,			            // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        m_d3dDevice.ReleaseAndGetAddressOf(),   // Returns the Direct3D device created.
        &m_d3dFeatureLevel,			            // Returns feature level of device created.
        m_d3dContext.ReleaseAndGetAddressOf()   // Returns the device immediate context.
        );

    if ( hr == E_INVALIDARG )
    {
        assert(featureLevels[0] == D3D_FEATURE_LEVEL_11_1);

        // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
        hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                creationFlags, &featureLevels[1], _countof(featureLevels) - 1,
                                D3D11_SDK_VERSION, m_d3dDevice.ReleaseAndGetAddressOf(),
                                &m_d3dFeatureLevel, m_d3dContext.ReleaseAndGetAddressOf() );
    }

    // TODO - WARP fallback?

    DX::ThrowIfFailed(hr);

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    hr = m_d3dDevice.As(&d3dDebug);
    if (SUCCEEDED(hr))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        hr = d3dDebug.As(&d3dInfoQueue);
        if (SUCCEEDED(hr))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed 
            };
            D3D11_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    // Obtain Direct3D 11.1 interfaces (if available)
    hr = m_d3dDevice.As(&m_d3dDevice1);
    if (SUCCEEDED(hr))
    {
        (void) m_d3dContext.As(&m_d3dContext1);
    }
}

// These resources need to be recreated every time the window size is changed.
void DX::DeviceResources::CreateWindowSizeDependentResources() 
{
    if ( !m_window )
    {
        throw std::exception( "SetWindow" );
    }

    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = {nullptr};
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_d3dRenderTargetView = nullptr;
    m_d3dDepthStencilView = nullptr;
    m_d3dContext->Flush();

    // Determine the render target size in pixels.
    UINT backBufferWidth = std::max<UINT>( m_outputSize.right - m_outputSize.left, 1 );
    UINT backBufferHeight = std::max<UINT>( m_outputSize.bottom - m_outputSize.top, 1 );
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    if (m_swapChain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = m_swapChain->ResizeBuffers(
            2, // Double-buffered swap chain.
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            0
            );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // Otherwise, create a new one using the same adapter as the existing Direct3D device.

        // This sequence obtains the DXGI factory that was used to create the Direct3D device above.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        ComPtr<IDXGIFactory1> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), &dxgiFactory));

        ComPtr<IDXGIFactory2> dxgiFactory2;
        HRESULT hr = dxgiFactory.As(&dxgiFactory2);
        if (SUCCEEDED(hr))
        {
            // DirectX 11.1 or later
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
            swapChainDesc.Width = backBufferWidth; // Match the size of the window.
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
            fsSwapChainDesc.Windowed = TRUE;

            // Create a SwapChain from a Win32 window.
            DX::ThrowIfFailed( dxgiFactory2->CreateSwapChainForHwnd(
                m_d3dDevice.Get(), m_window, &swapChainDesc,
                &fsSwapChainDesc,
                nullptr, m_swapChain1.ReleaseAndGetAddressOf() ) );

            (void) m_swapChain1.As( &m_swapChain );
        }
        else
        {
            // DirectX 11.0
            DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
            swapChainDesc.BufferCount = 2;
            swapChainDesc.BufferDesc.Width = backBufferWidth;
            swapChainDesc.BufferDesc.Height = backBufferHeight;
            swapChainDesc.BufferDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.OutputWindow = m_window;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Windowed = TRUE;

            DX::ThrowIfFailed( dxgiFactory->CreateSwapChain( m_d3dDevice.Get(), &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf() ) );
        }

        // This template does not support 'full-screen' mode and prevents the ALT+ENTER shortcut from working
        dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
    }

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer)
        );

    DX::ThrowIfFailed(
        m_d3dDevice->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            m_d3dRenderTargetView.ReleaseAndGetAddressOf()
            )
        );

    // Create a depth stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        depthBufferFormat, 
        backBufferWidth,
        backBufferHeight,
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL
        );

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(
        m_d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            depthStencil.GetAddressOf()
            )
        );

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(
        m_d3dDevice->CreateDepthStencilView(
            depthStencil.Get(),
            &depthStencilViewDesc,
            m_d3dDepthStencilView.ReleaseAndGetAddressOf()
            )
        );
    
    // Set the 3D rendering viewport to target the entire window.
    m_screenViewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        static_cast<float>(backBufferWidth),
        static_cast<float>(backBufferHeight)
        );

    m_d3dContext->RSSetViewports(1, &m_screenViewport);
}

// This method is called when the Win32 window is created (or re-created).
void DX::DeviceResources::SetWindow(HWND window)
{
    m_window = window;

    GetWindowRect( m_window, &m_outputSize );
}

// This method is called when the Win32 window changes size
bool DX::DeviceResources::WindowSizeChanged()
{
    if ( !m_window )
    {
        throw std::exception( "SetWindow" );
    }

    RECT newRc;
    GetWindowRect( m_window, &newRc );
    if (newRc == m_outputSize)
    {
        return false;
    }

    m_outputSize = newRc;
    CreateWindowSizeDependentResources();
    return true;
}

// Recreate all device resources and set them back to the current state.
void DX::DeviceResources::HandleDeviceLost()
{
    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceLost();
    }

    m_d3dDepthStencilView.Reset();
    m_d3dRenderTargetView.Reset();
    m_swapChain1.Reset();
    m_swapChain.Reset();
    m_d3dContext1.Reset();
    m_d3dContext.Reset();
    m_d3dDevice1.Reset();
    m_d3dDevice.Reset();

    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceRestored();
    }
}

// Present the contents of the swap chain to the screen.
void DX::DeviceResources::Present() 
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    if (m_d3dContext1)
    {
        // Discard the contents of the render target.
        // This is a valid operation only when the existing contents will be entirely
        // overwritten. If dirty or scroll rects are used, this call should be removed.
        m_d3dContext1->DiscardView(m_d3dRenderTargetView.Get());

        // Discard the contents of the depth stencil.
        m_d3dContext1->DiscardView(m_d3dDepthStencilView.Get());
    }

    // If the device was removed either by a disconnection or a driver upgrade, we 
    // must recreate all device resources.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        HandleDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}
