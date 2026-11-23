//----------------------------------------------------------------------------------
// File:        LeadBufferEngine.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Blackwell 32GB Asynchronous Lead-Buffer Ring & Zero-Stutter Playback
//----------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "../security/MemorySafety.h"
#include "../core/EngineConfig.h"

struct BufferedFrame {
    ComPtr<ID3D12Resource> pUpscaledTexture;
    ComPtr<ID3D12Resource> pOriginalTexture;
    uint32_t FrameIndex = 0;
    bool IsNeuralRendered = false; // true if NGX VSR evaluated, false if bicubic fallback
};

class LeadBufferEngine {
public:
    LeadBufferEngine();
    ~LeadBufferEngine();

    bool Initialize(
        ID3D12Device* pDevice,
        ID3D12CommandQueue* pComputeQueue,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        uint32_t ringCapacity = 90
    );
    void Shutdown();

    void Start();
    void Stop();

    // Presentation consumption
    bool AcquireNextDisplayFrame(BufferedFrame& outFrame);

    // Timeline control
    void Seek(uint32_t targetFrame);
    void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    void TogglePause() { m_isPaused = !m_isPaused; }
    bool IsPaused() const { return m_isPaused; }

    uint32_t GetCurrentFrameIndex() const { return m_currentPlayheadFrame; }
    uint32_t GetBufferedLeadFrames() const { return m_bufferedCount.load(); }
    uint32_t GetRingCapacity() const { return m_capacity; }

private:
    void InferenceWorkerThread();

    ID3D12Device* m_pDevice = nullptr;
    ID3D12CommandQueue* m_pComputeQueue = nullptr;

    uint32_t m_width = 3840;
    uint32_t m_height = 2160;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uint32_t m_capacity = 90;

    std::vector<BufferedFrame> m_ringBuffer;
    uint32_t m_readIndex = 0;
    uint32_t m_writeIndex = 0;
    std::atomic<uint32_t> m_bufferedCount = 0;

    std::atomic<uint32_t> m_currentPlayheadFrame = 0;
    std::atomic<float> m_playbackSpeed = 1.0f;
    std::atomic<bool> m_isPaused = false;
    std::atomic<bool> m_isRunning = false;

    std::thread m_workerThread;
    std::mutex m_mutex;
    std::condition_variable m_cvNotFull;
    std::condition_variable m_cvNotEmpty;
};
