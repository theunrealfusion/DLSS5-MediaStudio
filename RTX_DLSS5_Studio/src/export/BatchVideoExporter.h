//----------------------------------------------------------------------------------
// File:        BatchVideoExporter.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Offline batch video upscaling pipeline using RTX 5090 NVENC acceleration
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include "../core/EngineConfig.h"
#include "../ngx/NGXUpscaler.h"

class BatchVideoExporter {
public:
    BatchVideoExporter();
    ~BatchVideoExporter();

    using ProgressCallback = std::function<void(uint32_t currentFrame, uint32_t totalFrames, float fps)>;

    bool ProcessBatch(
        const std::wstring& inputVideoPath,
        const std::wstring& outputVideoPath,
        const EngineConfig& config,
        NGXUpscaler* pUpscaler,
        ProgressCallback onProgress = nullptr
    );

    void Cancel() { m_cancelRequested = true; }
    bool IsCancelled() const { return m_cancelRequested; }

private:
    bool m_cancelRequested = false;
};
