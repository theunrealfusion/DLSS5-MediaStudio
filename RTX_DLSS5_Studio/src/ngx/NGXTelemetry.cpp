//----------------------------------------------------------------------------------
// File:        NGXTelemetry.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Telemetry implementation and rolling latency calculation
//----------------------------------------------------------------------------------

#include "NGXTelemetry.h"
#include <numeric>
#include <algorithm>

void NGXTelemetry::StartFrameTiming() {
    m_startTime = std::chrono::high_resolution_clock::now();
}

void NGXTelemetry::EndFrameTiming() {
    auto endTime = std::chrono::high_resolution_clock::now();
    float latencyMs = std::chrono::duration<float, std::milli>(endTime - m_startTime).count();

    m_latencyHistory.push_back(latencyMs);
    if (m_latencyHistory.size() > 120) {
        m_latencyHistory.pop_front();
    }

    m_currentSnapshot.LastInferenceTimeMs = latencyMs;

    float sum = std::accumulate(m_latencyHistory.begin(), m_latencyHistory.end(), 0.0f);
    m_currentSnapshot.AverageInferenceTimeMs = sum / static_cast<float>(m_latencyHistory.size());

    m_currentSnapshot.MinInferenceTimeMs = *std::min_element(m_latencyHistory.begin(), m_latencyHistory.end());
    m_currentSnapshot.MaxInferenceTimeMs = *std::max_element(m_latencyHistory.begin(), m_latencyHistory.end());
}

void NGXTelemetry::RecordFrameProcessed() {
    m_totalFrames++;
    m_currentSnapshot.TotalFramesProcessed = m_totalFrames;

    m_fpsFrameCounter++;
    auto now = std::chrono::high_resolution_clock::now();
    float elapsedSec = std::chrono::duration<float>(now - m_lastFpsCheck).count();
    if (elapsedSec >= 1.0f) {
        m_currentSnapshot.FPS = static_cast<float>(m_fpsFrameCounter) / elapsedSec;
        m_fpsFrameCounter = 0;
        m_lastFpsCheck = now;
    }
}

void NGXTelemetry::SetGPUInfo(const std::wstring& name, uint32_t major, uint32_t minor) {
    m_currentSnapshot.GPUName = name;
    m_currentSnapshot.DriverMajor = major;
    m_currentSnapshot.DriverMinor = minor;
}

void NGXTelemetry::SetVRAMUsage(uint64_t bytes) {
    m_currentSnapshot.AllocatedVRAMBytes = bytes;
}

void NGXTelemetry::SetVSRQuality(uint32_t quality) {
    m_currentSnapshot.ActiveVSRQuality = quality;
}

void NGXTelemetry::SetTrueHDREnabled(bool enabled) {
    m_currentSnapshot.TrueHDREnabled = enabled;
}

TelemetrySnapshot NGXTelemetry::GetSnapshot() {
    return m_currentSnapshot;
}
