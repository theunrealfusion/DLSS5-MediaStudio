//----------------------------------------------------------------------------------
// File:        MediaDecoder.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Hardware-accelerated media decoding via Windows Media Foundation
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <cstdint>
#include "../security/MemorySafety.h"

#if defined(_WIN32)
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#endif

class MediaDecoder {
public:
    MediaDecoder();
    ~MediaDecoder();

    bool Open(const std::wstring& filePath);
    void Close();

    bool ReadNextFrame(uint8_t* pDestBuffer, size_t bufferSize, int64_t& outTimestampUs);
    bool Seek(int64_t timestampUs);

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    float GetFrameRate() const { return m_frameRate; }
    int64_t GetDurationUs() const { return m_durationUs; }
    bool IsOpen() const { return m_isOpen; }
    LONG GetStride() const { return m_stride; }

private:
#if defined(_WIN32)
    ComPtr<IMFSourceReader> m_pSourceReader;
    GUID m_outputSubtype = GUID_NULL;
#endif
    std::wstring m_filePath = L"";
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    LONG m_stride = 0;
    float m_frameRate = 30.0f;
    int64_t m_durationUs = 0;
    bool m_isOpen = false;
    bool m_mfStarted = false;
    bool m_comInitialized = false;
};
