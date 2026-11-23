//----------------------------------------------------------------------------------
// File:        NGXTelemetry.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Hardware telemetry, Tensor Core latency measurement, and GPU profiling
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <deque>

struct TelemetrySnapshot {
    std::wstring GPUName = L"NVIDIA GeForce RTX 5090";
    uint32_t DriverMajor = 621;
    uint32_t DriverMinor = 24;
    float LastInferenceTimeMs = 0.0f;
    float AverageInferenceTimeMs = 0.0f;
    float MinInferenceTimeMs = 0.0f;
    float MaxInferenceTimeMs = 0.0f;
    float FPS = 0.0f;
    uint64_t TotalFramesProcessed = 0;
    uint32_t ActiveVSRQuality = 4;
    bool TrueHDREnabled = false;
    uint64_t AllocatedVRAMBytes = 0;
};

class NGXTelemetry {
public:
    static NGXTelemetry& Instance() {
        static NGXTelemetry instance;
        return instance;
    }

    void StartFrameTiming();
    void EndFrameTiming();

    void RecordFrameProcessed();
    void SetGPUInfo(const std::wstring& name, uint32_t major, uint32_t minor);
    void SetVRAMUsage(uint64_t bytes);
    void SetVSRQuality(uint32_t quality);
    void SetTrueHDREnabled(bool enabled);

    TelemetrySnapshot GetSnapshot();

private:
    NGXTelemetry() = default;

    std::chrono::high_resolution_clock::time_point m_startTime;
    std::deque<float> m_latencyHistory;
    uint64_t m_totalFrames = 0;
    TelemetrySnapshot m_currentSnapshot;

    std::chrono::high_resolution_clock::time_point m_lastFpsCheck;
    uint32_t m_fpsFrameCounter = 0;
};
