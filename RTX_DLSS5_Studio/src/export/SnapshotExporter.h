//----------------------------------------------------------------------------------
// File:        SnapshotExporter.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Export high-resolution comparison snapshots with signed metadata watermark
//----------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include <string>
#include "../security/MemorySafety.h"
#include "../core/EngineConfig.h"

class SnapshotExporter {
public:
    SnapshotExporter();
    ~SnapshotExporter();

    bool CaptureSnapshot(
        ID3D12Device* pDevice,
        ID3D12CommandQueue* pCommandQueue,
        ID3D12Resource* pSourceTexture,
        const std::wstring& outputFilePath,
        const EngineConfig& config,
        uint32_t frameIndex
    );

private:
    bool SaveBMPWithMetadata(
        const std::wstring& filePath,
        const uint8_t* pPixels,
        uint32_t width,
        uint32_t height,
        uint32_t rowPitch,
        const EngineConfig& config,
        uint32_t frameIndex
    );
};
