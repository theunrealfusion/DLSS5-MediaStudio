//----------------------------------------------------------------------------------
// File:        D3D12SwapChain.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Direct3D 12 Flip-Model Swapchain with native HDR10 / scRGB support
//----------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "../security/MemorySafety.h"
#include <vector>

class D3D12SwapChain {
public:
    D3D12SwapChain();
    ~D3D12SwapChain();

    bool Initialize(
        IDXGIFactory4* pFactory,
        ID3D12Device* pDevice,
        ID3D12CommandQueue* pDirectQueue,
        HWND hWnd,
        uint32_t width,
        uint32_t height,
        bool enableHDR = false,
        bool outputFP16 = true,
        uint32_t bufferCount = 3
    );
    void Shutdown();

    bool Resize(uint32_t newWidth, uint32_t newHeight);
    bool Present(bool vsync = false);

    ID3D12Resource* GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
    uint32_t GetCurrentBackBufferIndex() const { return m_currentBufferIndex; }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    DXGI_FORMAT GetFormat() const { return m_format; }
    bool IsHDREnabled() const { return m_isHDR; }

private:
    void CreateRTVDescriptors();

    ComPtr<IDXGISwapChain4> m_pSwapChain;
    ID3D12Device* m_pDevice = nullptr;
    ID3D12CommandQueue* m_pCommandQueue = nullptr;

    ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;
    UINT m_rtvDescriptorSize = 0;

    HWND m_hWnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_bufferCount = 3;
    uint32_t m_currentBufferIndex = 0;

    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_COLOR_SPACE_TYPE m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    bool m_isHDR = false;
};
