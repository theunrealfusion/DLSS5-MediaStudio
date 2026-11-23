//----------------------------------------------------------------------------------
// File:        D3D12DeviceManager.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Direct3D 12 Device, Multi-Queue Asynchronous Architecture
//----------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "../security/MemorySafety.h"
#include <string>

class D3D12DeviceManager {
public:
    D3D12DeviceManager();
    ~D3D12DeviceManager();

    bool Initialize(bool enableDebugLayer = false);
    void Shutdown();

    // Device & Factory getters
    ID3D12Device* GetDevice() const { return m_pDevice.Get(); }
    IDXGIFactory4* GetFactory() const { return m_pFactory.Get(); }
    IDXGIAdapter1* GetAdapter() const { return m_pAdapter.Get(); }

    // Multi-Queue getters (Async Compute architecture for RTX 5090)
    ID3D12CommandQueue* GetDirectQueue() const { return m_pDirectQueue.Get(); }
    ID3D12CommandQueue* GetComputeQueue() const { return m_pComputeQueue.Get(); }
    ID3D12CommandQueue* GetCopyQueue() const { return m_pCopyQueue.Get(); }

    // Synchronization helpers
    void FlushGPU();
    uint64_t SignalFence(ID3D12CommandQueue* pQueue);
    void WaitForFenceCPU(uint64_t fenceValue);
    void QueueWaitForFenceGPU(ID3D12CommandQueue* pWaitingQueue, uint64_t fenceValue);

    // Diagnostics & Health
    bool CheckDeviceHealth();
    std::wstring GetAdapterDescription() const { return m_adapterDesc; }
    size_t GetDedicatedVideoMemoryMB() const { return m_dedicatedVRAMMB; }

private:
    ComPtr<IDXGIFactory4> m_pFactory;
    ComPtr<IDXGIAdapter1> m_pAdapter;
    ComPtr<ID3D12Device> m_pDevice;

    ComPtr<ID3D12CommandQueue> m_pDirectQueue;
    ComPtr<ID3D12CommandQueue> m_pComputeQueue;
    ComPtr<ID3D12CommandQueue> m_pCopyQueue;

    ComPtr<ID3D12Fence> m_pFence;
    HANDLE m_fenceEvent = nullptr;
    uint64_t m_currentFenceValue = 0;

    std::wstring m_adapterDesc = L"";
    size_t m_dedicatedVRAMMB = 0;
};
