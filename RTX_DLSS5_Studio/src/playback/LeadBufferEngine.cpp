//----------------------------------------------------------------------------------
// File:        LeadBufferEngine.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Implementation of asynchronous lead buffering and dynamic fallback
//----------------------------------------------------------------------------------

#include "LeadBufferEngine.h"
#include "../core/Logger.h"
#include "../ngx/NGXTelemetry.h"

LeadBufferEngine::LeadBufferEngine() {}

LeadBufferEngine::~LeadBufferEngine() {
    Shutdown();
}

bool LeadBufferEngine::Initialize(
    ID3D12Device* pDevice,
    ID3D12CommandQueue* pComputeQueue,
    uint32_t width,
    uint32_t height,
    DXGI_FORMAT format,
    uint32_t ringCapacity
) {
    if (!pDevice || !pComputeQueue) return false;
    m_pDevice = pDevice;
    m_pComputeQueue = pComputeQueue;
    m_width = width;
    m_height = height;
    m_format = format;
    m_capacity = ringCapacity;

    m_ringBuffer.resize(m_capacity);

    // Allocate ring buffer textures on default GPU heap (VRAM)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = m_format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    size_t totalBytes = 0;
    for (uint32_t i = 0; i < m_capacity; ++i) {
        HRESULT hr = m_pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_ringBuffer[i].pUpscaledTexture)
        );
        if (FAILED(hr)) {
            LOG_ERROR("LeadBufferEngine: Failed to allocate VRAM texture at index " + std::to_string(i));
            return false;
        }

        hr = m_pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_ringBuffer[i].pOriginalTexture)
        );
        if (FAILED(hr)) return false;

        m_ringBuffer[i].FrameIndex = i;
        m_ringBuffer[i].IsNeuralRendered = false;

        totalBytes += (m_width * m_height * 4 * 2);
    }

    NGXTelemetry::Instance().SetVRAMUsage(totalBytes);
    LOG_INFO("LeadBufferEngine allocated " + std::to_string(m_capacity) + " frames in RTX 5090 VRAM (~" +
             std::to_string(totalBytes / (1024 * 1024)) + " MB)");

    return true;
}

void LeadBufferEngine::Start() {
    m_isRunning = true;
    m_workerThread = std::thread(&LeadBufferEngine::InferenceWorkerThread, this);
    LOG_INFO("Lead-buffer inference worker thread started.");
}

void LeadBufferEngine::Stop() {
    m_isRunning = false;
    m_cvNotFull.notify_all();
    m_cvNotEmpty.notify_all();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void LeadBufferEngine::Shutdown() {
    Stop();
    m_ringBuffer.clear();
    m_pDevice = nullptr;
    m_pComputeQueue = nullptr;
}

void LeadBufferEngine::InferenceWorkerThread() {
    while (m_isRunning) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cvNotFull.wait(lock, [this]() {
            return !m_isRunning || (m_bufferedCount.load() < m_capacity);
        });

        if (!m_isRunning) break;

        // Process frame ahead of playhead
        uint32_t frameToRender = m_currentPlayheadFrame.load() + m_bufferedCount.load();
        uint32_t slot = m_writeIndex;

        m_ringBuffer[slot].FrameIndex = frameToRender;
        m_ringBuffer[slot].IsNeuralRendered = true;

        m_writeIndex = (m_writeIndex + 1) % m_capacity;
        m_bufferedCount++;

        lock.unlock();
        m_cvNotEmpty.notify_one();

        // Simulate tiny frame yield or pacing if buffer is comfortably leaded
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

bool LeadBufferEngine::AcquireNextDisplayFrame(BufferedFrame& outFrame) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_bufferedCount.load() == 0) {
        // Dynamic Fallback: Starvation detected during rapid seek, return immediately with fallback flag
        outFrame.FrameIndex = m_currentPlayheadFrame.load();
        outFrame.IsNeuralRendered = false; // Trigger immediate hardware bicubic rendering
        return false;
    }

    outFrame = m_ringBuffer[m_readIndex];
    m_readIndex = (m_readIndex + 1) % m_capacity;
    m_bufferedCount--;

    if (!m_isPaused) {
        m_currentPlayheadFrame++;
    }

    lock.unlock();
    m_cvNotFull.notify_one();
    return true;
}

void LeadBufferEngine::Seek(uint32_t targetFrame) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_currentPlayheadFrame = targetFrame;
    m_readIndex = 0;
    m_writeIndex = 0;
    m_bufferedCount = 0;
    lock.unlock();

    m_cvNotFull.notify_all();
    LOG_INFO("LeadBufferEngine timeline seek to frame " + std::to_string(targetFrame));
}
