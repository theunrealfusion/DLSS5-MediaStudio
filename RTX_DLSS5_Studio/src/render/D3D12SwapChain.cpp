//----------------------------------------------------------------------------------
// File:        D3D12SwapChain.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Implementation of DXGI Flip-Discard swapchain and HDR configuration
//----------------------------------------------------------------------------------

#include "D3D12SwapChain.h"
#include "../core/Logger.h"

D3D12SwapChain::D3D12SwapChain() {}

D3D12SwapChain::~D3D12SwapChain() {
    Shutdown();
}

bool D3D12SwapChain::Initialize(
    IDXGIFactory4* pFactory,
    ID3D12Device* pDevice,
    ID3D12CommandQueue* pDirectQueue,
    HWND hWnd,
    uint32_t width,
    uint32_t height,
    bool enableHDR,
    bool outputFP16,
    uint32_t bufferCount
) {
    if (!pFactory || !pDevice || !pDirectQueue || !hWnd) {
        LOG_ERROR("D3D12SwapChain::Initialize: Invalid parameters");
        return false;
    }

    m_pDevice = pDevice;
    m_pCommandQueue = pDirectQueue;
    m_hWnd = hWnd;
    m_width = width;
    m_height = height;
    m_bufferCount = bufferCount;
    m_isHDR = enableHDR;

    if (m_isHDR) {
        if (outputFP16) {
            m_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709; // scRGB FP16
        } else {
            m_format = DXGI_FORMAT_R10G10B10A2_UNORM;
            m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020; // HDR10 PQ
        }
    } else {
        m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = m_format;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = pFactory->CreateSwapChainForHwnd(
        m_pCommandQueue,
        m_hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr)) {
        LOG_ERROR("D3D12SwapChain: Failed to create SwapChain for HWND. HR=" + std::to_string(hr));
        return false;
    }

    // Disable DXGI automatic Alt+Enter handling for custom window management
    pFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

    hr = swapChain1.As(&m_pSwapChain);
    if (FAILED(hr)) return false;

    // Set Color Space for HDR
    if (m_isHDR) {
        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(m_pSwapChain->CheckColorSpaceSupport(m_colorSpace, &colorSpaceSupport)) &&
            (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
            m_pSwapChain->SetColorSpace1(m_colorSpace);
            LOG_INFO("HDR Color Space successfully configured on swapchain.");
        }
    }

    // Descriptor heap for Render Target Views (RTV)
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = m_bufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRtvHeap));
    if (FAILED(hr)) return false;

    m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_currentBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    CreateRTVDescriptors();

    LOG_INFO("Direct3D 12 Swapchain initialized (" + std::to_string(m_width) + "x" + std::to_string(m_height) +
             ", Buffers: " + std::to_string(m_bufferCount) + ")");
    return true;
}

void D3D12SwapChain::CreateRTVDescriptors() {
    m_backBuffers.resize(m_bufferCount);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < m_bufferCount; ++i) {
        m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        m_pDevice->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
}

void D3D12SwapChain::Shutdown() {
    for (auto& buffer : m_backBuffers) {
        buffer.Reset();
    }
    m_backBuffers.clear();
    m_pRtvHeap.Reset();
    m_pSwapChain.Reset();
    m_pDevice = nullptr;
    m_pCommandQueue = nullptr;
}

bool D3D12SwapChain::Resize(uint32_t newWidth, uint32_t newHeight) {
    if (!m_pSwapChain || (newWidth == m_width && newHeight == m_height) || newWidth == 0 || newHeight == 0) {
        return true;
    }

    for (auto& buffer : m_backBuffers) {
        buffer.Reset();
    }

    m_width = newWidth;
    m_height = newHeight;

    HRESULT hr = m_pSwapChain->ResizeBuffers(
        m_bufferCount,
        m_width,
        m_height,
        m_format,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    );

    if (FAILED(hr)) {
        LOG_ERROR("D3D12SwapChain::Resize failed. HR=" + std::to_string(hr));
        return false;
    }

    m_currentBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    CreateRTVDescriptors();
    return true;
}

bool D3D12SwapChain::Present(bool vsync) {
    if (!m_pSwapChain) return false;

    UINT syncInterval = vsync ? 1 : 0;
    UINT flags = vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    HRESULT hr = m_pSwapChain->Present(syncInterval, flags);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            HRESULT reason = m_pDevice ? m_pDevice->GetDeviceRemovedReason() : hr;
            char hexBuf[32];
            sprintf_s(hexBuf, "0x%08X", (UINT)reason);
            LOG_FATAL("Present: Device Removed during swapchain present! Reason: " + std::string(hexBuf));
        }
        return false;
    }

    m_currentBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    return true;
}

ID3D12Resource* D3D12SwapChain::GetCurrentBackBuffer() const {
    if (m_currentBufferIndex < m_backBuffers.size()) {
        return m_backBuffers[m_currentBufferIndex].Get();
    }
    return nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetCurrentRTV() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_currentBufferIndex * m_rtvDescriptorSize;
    return handle;
}
