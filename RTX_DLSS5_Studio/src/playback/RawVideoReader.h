//----------------------------------------------------------------------------------
// File:        RawVideoReader.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Studio uncompressed video file ingestion (NV12, P010, ARGB, ABGR10)
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <cstdio>
#include <cstdint>
#include "../security/MemorySafety.h"

enum class RawPixelFormat {
    NV12,
    P010,
    ARGB,
    ABGR10,
    YUY2,
    Unknown
};

class RawVideoReader {
public:
    RawVideoReader();
    ~RawVideoReader();

    bool Open(const std::wstring& filePath, uint32_t width = 0, uint32_t height = 0, RawPixelFormat format = RawPixelFormat::NV12);
    void Close();

    bool ReadFrame(uint8_t* pDestBuffer, size_t destBufferSize, int32_t frameIndex = -1);
    bool ReadFrameAsRGBA(uint8_t* pDestRGBA, size_t destPitchBytes, int32_t frameIndex = -1);

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetPitch() const { return m_pitch; }
    uint32_t GetNumFrames() const { return m_numFrames; }
    uint32_t GetCurrentFrame() const { return m_currentFrame; }
    RawPixelFormat GetFormat() const { return m_format; }
    size_t GetFrameSizeBytes() const { return m_frameSizeBytes; }

    void NextFrameCyclic();
    void PrevFrameCyclic();
    void SeekFrame(uint32_t frame);

private:
    bool ParseFilenameMetadata(const std::wstring& filename);

    FILE* m_pFile = nullptr;
    std::wstring m_filePath = L"";

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_pitch = 0;
    uint32_t m_numFrames = 1;
    uint32_t m_currentFrame = 0;
    size_t m_frameSizeBytes = 0;
    RawPixelFormat m_format = RawPixelFormat::Unknown;
    std::vector<uint8_t> m_rawFrameBuffer;
};
