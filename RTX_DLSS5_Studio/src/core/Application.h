//----------------------------------------------------------------------------------
// File:        Application.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Main Application Coordinator, Event Loop, and Pipeline Execution
//----------------------------------------------------------------------------------

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include "EngineConfig.h"
#include "../security/SecurityManager.h"
#include "../render/D3D12DeviceManager.h"
#include "../render/D3D12SwapChain.h"
#include "../render/ComparisonRenderer.h"
#include "../ngx/NGXUpscaler.h"
#include "../playback/LeadBufferEngine.h"
#include "../playback/RawVideoReader.h"
#include "../playback/MediaDecoder.h"
#include "../ui/UIManager.h"
#include "../export/SnapshotExporter.h"
#include "../export/BatchVideoExporter.h"

class Application {
public:
    Application();
    ~Application();

    bool Initialize(HINSTANCE hInstance, int nCmdShow, const EngineConfig& config);
    int Run();
    void Shutdown();

    bool LoadMediaFile(const std::wstring& filePath);
    void OpenFileDialog();
    void HandleDropFiles(HDROP hDrop);

    void TogglePause() { m_isPaused = !m_isPaused; }
    void StepFrame(int32_t step);

    bool RunTest(const std::wstring& testFilePath);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void EnsureRuntimeFilesExtracted();

private:
    void RenderFrame();
    void OnResize(uint32_t width, uint32_t height);
    bool CreateVideoTexturesAndBuffers(uint32_t width, uint32_t height);
    void GenerateProceduralFrame(uint8_t* pDest, size_t rowPitch, uint32_t width, uint32_t height, uint32_t frameIdx);

    EngineConfig m_config;
    HWND m_hWnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_isRunning = false;
    bool m_isPaused = false;
    bool m_forceFrameRefresh = true;

    // Media playback state
    bool m_isMediaLoaded = false;
    bool m_isRawYuv = false;
    bool m_isMediaFoundation = false;
    bool m_isProcedural = false;

    uint32_t m_sourceWidth = 1280;
    uint32_t m_sourceHeight = 720;
    uint32_t m_currentFrameIndex = 0;
    uint32_t m_totalFrames = 30;
    float m_videoFps = 30.0f;
    float m_frameTimeAccumulator = 0.0f;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    std::wstring m_activeMediaTitle = L"Procedural Test Target";

    std::vector<uint8_t> m_mediaDecoderBuffer;

    // Subsystems
    D3D12DeviceManager m_deviceManager;
    D3D12SwapChain m_swapChain;
    NGXUpscaler m_ngxUpscaler;
    ComparisonRenderer m_comparisonRenderer;
    LeadBufferEngine m_leadBufferEngine;
    RawVideoReader m_rawReader;
    MediaDecoder m_mediaDecoder;
    UIManager m_uiManager;
    SnapshotExporter m_snapshotExporter;
    BatchVideoExporter m_batchExporter;

    // Direct3D 12 Per-Frame Resources
    ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

    ComPtr<ID3D12DescriptorHeap> m_pSrvHeap;
    ComPtr<ID3D12Resource> m_pOriginalTexture;
    ComPtr<ID3D12Resource> m_pUpscaledTexture;

    // Staging and Upload
    ComPtr<ID3D12Resource> m_pUploadBuffer;
    uint8_t* m_pUploadMappedData = nullptr;
    size_t m_uploadBufferSize = 0;
    size_t m_uploadRowPitch = 0;

    static Application* s_pInstance;
};
