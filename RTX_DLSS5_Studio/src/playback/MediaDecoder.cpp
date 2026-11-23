//----------------------------------------------------------------------------------
// File:        MediaDecoder.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Hardware-accelerated Media Foundation decoding pipeline
//----------------------------------------------------------------------------------

#include "MediaDecoder.h"
#include "../core/Logger.h"
#include <algorithm>
#include <cmath>

#if defined(_WIN32)
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#endif

MediaDecoder::MediaDecoder() {
#if defined(_WIN32)
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    m_comInitialized = SUCCEEDED(hrCo) || (hrCo == RPC_E_CHANGED_MODE);

    HRESULT hr = MFStartup(MF_VERSION);
    m_mfStarted = SUCCEEDED(hr);
    if (!m_mfStarted) {
        LOG_ERROR("MediaDecoder: MFStartup failed with code: " + std::to_string(hr));
    }
#endif
}

MediaDecoder::~MediaDecoder() {
    Close();
#if defined(_WIN32)
    if (m_mfStarted) {
        MFShutdown();
        m_mfStarted = false;
    }
    if (m_comInitialized) {
        CoUninitialize();
        m_comInitialized = false;
    }
#endif
}

bool MediaDecoder::Open(const std::wstring& filePath) {
    Close();

    // Canonicalize to full absolute path required by MFCreateSourceReaderFromURL
    wchar_t fullPath[MAX_PATH] = { 0 };
    if (GetFullPathNameW(filePath.c_str(), MAX_PATH, fullPath, nullptr) > 0) {
        m_filePath = fullPath;
    } else {
        m_filePath = filePath;
    }

#if defined(_WIN32)
    if (!m_mfStarted) {
        HRESULT hr = MFStartup(MF_VERSION);
        m_mfStarted = SUCCEEDED(hr);
        if (!m_mfStarted) return false;
    }

    ComPtr<IMFAttributes> pAttributes;
    HRESULT hr = MFCreateAttributes(&pAttributes, 4);
    if (SUCCEEDED(hr)) {
        pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
        pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    }

    hr = MFCreateSourceReaderFromURL(m_filePath.c_str(), pAttributes.Get(), &m_pSourceReader);
    if (FAILED(hr)) {
        // Fallback: try with default attributes
        hr = MFCreateSourceReaderFromURL(m_filePath.c_str(), nullptr, &m_pSourceReader);
    }

    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, L"0x%08X", static_cast<unsigned int>(hr));
        LOG_ERROR(L"MediaDecoder: Failed to open media source from " + m_filePath + L" (HRESULT: " + buf + L")");
        return false;
    }

    // Deselect all streams then select only the first video stream
    m_pSourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    m_pSourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);

    // Configure video stream output format: try RGB32 first
    ComPtr<IMFMediaType> pType;
    hr = MFCreateMediaType(&pType);
    if (FAILED(hr)) return false;

    pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

    hr = m_pSourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, pType.Get());
    if (FAILED(hr)) {
        // Fallback 1: ARGB32
        pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32);
        hr = m_pSourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, pType.Get());
    }
    if (FAILED(hr)) {
        // Fallback 2: NV12
        pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        hr = m_pSourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, pType.Get());
    }
    if (FAILED(hr)) {
        // Fallback 3: YUY2
        pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
        hr = m_pSourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, pType.Get());
    }
    if (FAILED(hr)) {
        LOG_ERROR("MediaDecoder: Failed to set supported output media type (RGB32/ARGB32/NV12/YUY2).");
        return false;
    }

    // Query dimensions, actual subtype, framerate, and default stride
    ComPtr<IMFMediaType> pActualType;
    hr = m_pSourceReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &pActualType);
    if (SUCCEEDED(hr)) {
        pActualType->GetGUID(MF_MT_SUBTYPE, &m_outputSubtype);
        MFGetAttributeSize(pActualType.Get(), MF_MT_FRAME_SIZE, &m_width, &m_height);

        UINT32 num = 0, den = 0;
        if (SUCCEEDED(MFGetAttributeRatio(pActualType.Get(), MF_MT_FRAME_RATE, &num, &den)) && den > 0) {
            m_frameRate = static_cast<float>(num) / static_cast<float>(den);
        }

        LONG defaultStride = 0;
        if (SUCCEEDED(pActualType->GetUINT32(MF_MT_DEFAULT_STRIDE, reinterpret_cast<UINT32*>(&defaultStride)))) {
            m_stride = defaultStride;
        } else {
            m_stride = static_cast<LONG>(m_width * 4);
        }
    }

    PROPVARIANT var;
    PropVariantInit(&var);
    hr = m_pSourceReader->GetPresentationAttribute(static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var);
    if (SUCCEEDED(hr) && var.vt == VT_UI8) {
        m_durationUs = var.uhVal.QuadPart / 10; // Convert 100ns units to us
    }
    PropVariantClear(&var);

    m_isOpen = true;
    LOG_INFO(L"MediaDecoder: Opened " + filePath + L" (" + std::to_wstring(m_width) + L"x" +
             std::to_wstring(m_height) + L" @ " + std::to_wstring(static_cast<int>(m_frameRate)) + L" fps, duration=" +
             std::to_wstring(m_durationUs / 1000000) + L"s)");
    return true;
#else
    return false;
#endif
}

void MediaDecoder::Close() {
#if defined(_WIN32)
    m_pSourceReader.Reset();
    m_outputSubtype = GUID_NULL;
#endif
    m_isOpen = false;
    m_width = 0;
    m_height = 0;
    m_stride = 0;
    m_durationUs = 0;
}

bool MediaDecoder::ReadNextFrame(uint8_t* pDestRGBA, size_t bufferSize, int64_t& outTimestampUs) {
#if defined(_WIN32)
    if (!m_pSourceReader || !pDestRGBA || m_width == 0 || m_height == 0) return false;
    if (bufferSize < static_cast<size_t>(m_width * m_height * 4)) return false;

    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG llTimestamp = 0;
    ComPtr<IMFSample> pSample;

    HRESULT hr = m_pSourceReader->ReadSample(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
        0,
        &streamIndex,
        &flags,
        &llTimestamp,
        &pSample
    );

    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) || !pSample) {
        return false;
    }

    outTimestampUs = llTimestamp / 10;

    ComPtr<IMFMediaBuffer> pBuffer;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr)) return false;

    ComPtr<IMF2DBuffer> p2DBuffer;
    BYTE* pSrcData = nullptr;
    LONG srcPitch = 0;
    bool is2DLocked = false;

    if (SUCCEEDED(pBuffer.As(&p2DBuffer))) {
        if (SUCCEEDED(p2DBuffer->Lock2D(&pSrcData, &srcPitch))) {
            is2DLocked = true;
        }
    }

    DWORD currentLength = 0;
    if (!is2DLocked) {
        if (FAILED(pBuffer->Lock(&pSrcData, nullptr, &currentLength))) {
            return false;
        }
        srcPitch = (m_stride != 0) ? m_stride : static_cast<LONG>(m_width * 4);
    }

    if (m_outputSubtype == MFVideoFormat_RGB32 || m_outputSubtype == MFVideoFormat_ARGB32) {
        LONG absPitch = std::abs(srcPitch);
        for (uint32_t y = 0; y < m_height; ++y) {
            // Safe bottom-up DIB indexing without buffer underrun
            const BYTE* pSrcRow = (srcPitch < 0)
                ? (pSrcData + (static_cast<size_t>(m_height - 1 - y) * absPitch))
                : (pSrcData + (static_cast<size_t>(y) * absPitch));
            BYTE* pDstRow = pDestRGBA + (static_cast<size_t>(y) * m_width * 4);

            for (uint32_t x = 0; x < m_width; ++x) {
                // Media Foundation RGB32 is stored as B, G, R, X -> convert to standard RGBA
                pDstRow[x * 4 + 0] = pSrcRow[x * 4 + 2]; // Red
                pDstRow[x * 4 + 1] = pSrcRow[x * 4 + 1]; // Green
                pDstRow[x * 4 + 2] = pSrcRow[x * 4 + 0]; // Blue
                pDstRow[x * 4 + 3] = 255;                // Alpha
            }
        }
    } else if (m_outputSubtype == MFVideoFormat_NV12) {
        LONG absPitch = std::abs(srcPitch);
        const BYTE* pYPlane = pSrcData;
        const BYTE* pUVPlane = pSrcData + (static_cast<size_t>(absPitch) * m_height);

        for (uint32_t y = 0; y < m_height; ++y) {
            const BYTE* yRow = pYPlane + (static_cast<size_t>(y) * absPitch);
            const BYTE* uvRow = pUVPlane + (static_cast<size_t>(y / 2) * absPitch);
            BYTE* pDstRow = pDestRGBA + (static_cast<size_t>(y) * m_width * 4);

            for (uint32_t x = 0; x < m_width; ++x) {
                int Y = yRow[x];
                int U = uvRow[(x / 2) * 2] - 128;
                int V = uvRow[(x / 2) * 2 + 1] - 128;

                int R = static_cast<int>(Y + 1.402f * V);
                int G = static_cast<int>(Y - 0.344136f * U - 0.714136f * V);
                int B = static_cast<int>(Y + 1.772f * U);

                pDstRow[x * 4 + 0] = static_cast<uint8_t>(std::clamp(R, 0, 255));
                pDstRow[x * 4 + 1] = static_cast<uint8_t>(std::clamp(G, 0, 255));
                pDstRow[x * 4 + 2] = static_cast<uint8_t>(std::clamp(B, 0, 255));
                pDstRow[x * 4 + 3] = 255;
            }
        }
    } else if (m_outputSubtype == MFVideoFormat_YUY2) {
        LONG absPitch = std::abs(srcPitch);
        for (uint32_t y = 0; y < m_height; ++y) {
            const BYTE* pSrcRow = pSrcData + (static_cast<size_t>(y) * absPitch);
            BYTE* pDstRow = pDestRGBA + (static_cast<size_t>(y) * m_width * 4);
            for (uint32_t x = 0; x < m_width; ++x) {
                uint32_t mb = (x / 2) * 4;
                int Y = (x % 2 == 0) ? pSrcRow[mb + 0] : pSrcRow[mb + 2];
                int U = pSrcRow[mb + 1] - 128;
                int V = pSrcRow[mb + 3] - 128;
                int R = static_cast<int>(Y + 1.402f * V);
                int G = static_cast<int>(Y - 0.344136f * U - 0.714136f * V);
                int B = static_cast<int>(Y + 1.772f * U);
                pDstRow[x * 4 + 0] = static_cast<uint8_t>(std::clamp(R, 0, 255));
                pDstRow[x * 4 + 1] = static_cast<uint8_t>(std::clamp(G, 0, 255));
                pDstRow[x * 4 + 2] = static_cast<uint8_t>(std::clamp(B, 0, 255));
                pDstRow[x * 4 + 3] = 255;
            }
        }
    } else {
        size_t copyBytes = (bufferSize < static_cast<size_t>(m_width * m_height * 4)) ? bufferSize : static_cast<size_t>(m_width * m_height * 4);
        memcpy(pDestRGBA, pSrcData, copyBytes);
    }

    if (is2DLocked) {
        p2DBuffer->Unlock2D();
    } else {
        pBuffer->Unlock();
    }

    return true;
#else
    return false;
#endif
}

bool MediaDecoder::Seek(int64_t timestampUs) {
#if defined(_WIN32)
    if (!m_pSourceReader) return false;

    PROPVARIANT var;
    PropVariantInit(&var);
    var.vt = VT_I8;
    var.hVal.QuadPart = timestampUs * 10; // Convert to 100ns units

    HRESULT hr = m_pSourceReader->SetCurrentPosition(GUID_NULL, var);
    PropVariantClear(&var);

    return SUCCEEDED(hr);
#else
    return false;
#endif
}
