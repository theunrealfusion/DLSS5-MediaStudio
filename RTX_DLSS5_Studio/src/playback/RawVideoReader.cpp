//----------------------------------------------------------------------------------
// File:        RawVideoReader.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Implementation of raw studio frame parsing and seeking
//----------------------------------------------------------------------------------

#include "RawVideoReader.h"
#include "../core/Logger.h"
#include <regex>
#include <vector>
#include <algorithm>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace {
    std::string WideToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
#if defined(_WIN32)
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], sizeNeeded, nullptr, nullptr);
        return str;
#else
        return std::string(wstr.begin(), wstr.end());
#endif
    }
}

RawVideoReader::RawVideoReader() {}

RawVideoReader::~RawVideoReader() {
    Close();
}

bool RawVideoReader::ParseFilenameMetadata(const std::wstring& filename) {
    // NVIDIA format: fmt_width_height_pitch_numFrames_name.ext
    // Example: NV12_1280_720_1280_120_star_sky.yuv
    std::string narrowName = WideToUtf8(filename);

    std::regex pattern(R"(([A-Za-z0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+))");
    std::smatch matches;

    if (std::regex_search(narrowName, matches, pattern) && matches.size() >= 6) {
        std::string fmtStr = matches[1].str();
        m_width = std::stoul(matches[2].str());
        m_height = std::stoul(matches[3].str());
        m_pitch = std::stoul(matches[4].str());
        m_numFrames = std::stoul(matches[5].str());

        if (fmtStr == "NV12") m_format = RawPixelFormat::NV12;
        else if (fmtStr == "P010") m_format = RawPixelFormat::P010;
        else if (fmtStr == "ARGB") m_format = RawPixelFormat::ARGB;
        else if (fmtStr == "ABGR10") m_format = RawPixelFormat::ABGR10;
        else if (fmtStr == "YUY2") m_format = RawPixelFormat::YUY2;

        LOG_INFO("Parsed filename metadata: Format=" + fmtStr + ", Res=" + std::to_string(m_width) + "x" +
                 std::to_string(m_height) + ", Pitch=" + std::to_string(m_pitch) + ", Frames=" + std::to_string(m_numFrames));
        return true;
    }

    return false;
}

bool RawVideoReader::Open(const std::wstring& filePath, uint32_t width, uint32_t height, RawPixelFormat format) {
    Close();
    m_filePath = filePath;

    if (!ParseFilenameMetadata(filePath)) {
        m_width = width ? width : 1920;
        m_height = height ? height : 1080;
        m_pitch = m_width;
        m_format = format;
        m_numFrames = 1;
    }

    // Calculate frame size based on format
    switch (m_format) {
        case RawPixelFormat::NV12:
            m_frameSizeBytes = (m_pitch * m_height * 3) / 2; // Y + UV interleaved
            break;
        case RawPixelFormat::P010:
            m_frameSizeBytes = (m_pitch * m_height * 3);     // 10-bit Y + UV (2 bytes per sample)
            break;
        case RawPixelFormat::ARGB:
        case RawPixelFormat::ABGR10:
            m_frameSizeBytes = m_pitch * m_height * 4;       // 4 bytes per pixel
            break;
        case RawPixelFormat::YUY2:
            m_frameSizeBytes = m_pitch * m_height * 2;
            break;
        default:
            m_frameSizeBytes = m_pitch * m_height * 4;
            break;
    }

#if defined(_WIN32)
    _wfopen_s(&m_pFile, filePath.c_str(), L"rb");
#else
    m_pFile = fopen(filePath.c_str(), "rb");
#endif

    if (!m_pFile) {
        LOG_ERROR(L"RawVideoReader: Failed to open file: " + filePath);
        return false;
    }

    // If numFrames was not parsed, estimate from file size
    fseek(m_pFile, 0, SEEK_END);
    long long fileSize = _ftelli64(m_pFile);
    fseek(m_pFile, 0, SEEK_SET);

    if (m_frameSizeBytes > 0 && fileSize >= (long long)m_frameSizeBytes) {
        m_numFrames = static_cast<uint32_t>(fileSize / m_frameSizeBytes);
    }

    m_currentFrame = 0;
    LOG_INFO(L"RawVideoReader: Successfully opened " + filePath + L" (" + std::to_wstring(m_numFrames) + L" frames)");
    return true;
}

void RawVideoReader::Close() {
    if (m_pFile) {
        fclose(m_pFile);
        m_pFile = nullptr;
    }
    m_currentFrame = 0;
}

bool RawVideoReader::ReadFrame(uint8_t* pDestBuffer, size_t destBufferSize, int32_t frameIndex) {
    if (!m_pFile || !pDestBuffer || destBufferSize < m_frameSizeBytes) {
        return false;
    }

    if (frameIndex >= 0) {
        m_currentFrame = frameIndex % m_numFrames;
    }

    long long offset = static_cast<long long>(m_currentFrame) * m_frameSizeBytes;
    _fseeki64(m_pFile, offset, SEEK_SET);

    size_t bytesRead = fread(pDestBuffer, 1, m_frameSizeBytes, m_pFile);
    return (bytesRead == m_frameSizeBytes);
}

bool RawVideoReader::ReadFrameAsRGBA(uint8_t* pDestRGBA, size_t destPitchBytes, int32_t frameIndex) {
    if (!m_pFile || !pDestRGBA || m_frameSizeBytes == 0 || m_width == 0 || m_height == 0) {
        return false;
    }

    if (m_rawFrameBuffer.size() < m_frameSizeBytes) {
        m_rawFrameBuffer.resize(m_frameSizeBytes);
    }

    if (!ReadFrame(m_rawFrameBuffer.data(), m_rawFrameBuffer.size(), frameIndex)) {
        return false;
    }

    if (m_format == RawPixelFormat::NV12) {
        const uint8_t* yPlane = m_rawFrameBuffer.data();
        const uint8_t* uvPlane = m_rawFrameBuffer.data() + (m_pitch * m_height);

        for (uint32_t y = 0; y < m_height; ++y) {
            uint8_t* dstRow = pDestRGBA + (y * destPitchBytes);
            const uint8_t* yRow = yPlane + (y * m_pitch);
            const uint8_t* uvRow = uvPlane + ((y / 2) * m_pitch);

            for (uint32_t x = 0; x < m_width; ++x) {
                int Y = yRow[x];
                int U = uvRow[(x / 2) * 2] - 128;
                int V = uvRow[(x / 2) * 2 + 1] - 128;

                int R = static_cast<int>(Y + 1.402f * V);
                int G = static_cast<int>(Y - 0.344136f * U - 0.714136f * V);
                int B = static_cast<int>(Y + 1.772f * U);

                dstRow[x * 4 + 0] = static_cast<uint8_t>(std::clamp(R, 0, 255));
                dstRow[x * 4 + 1] = static_cast<uint8_t>(std::clamp(G, 0, 255));
                dstRow[x * 4 + 2] = static_cast<uint8_t>(std::clamp(B, 0, 255));
                dstRow[x * 4 + 3] = 255;
            }
        }
        return true;
    } else if (m_format == RawPixelFormat::ARGB || m_format == RawPixelFormat::ABGR10) {
        const uint8_t* srcData = m_rawFrameBuffer.data();
        size_t rowBytes = m_width * 4;
        for (uint32_t y = 0; y < m_height; ++y) {
            uint8_t* dstRow = pDestRGBA + (y * destPitchBytes);
            const uint8_t* srcRow = srcData + (y * m_pitch * 4);
            memcpy(dstRow, srcRow, rowBytes);
        }
        return true;
    }

    return false;
}

void RawVideoReader::NextFrameCyclic() {
    if (m_numFrames > 0) {
        m_currentFrame = (m_currentFrame + 1) % m_numFrames;
    }
}

void RawVideoReader::PrevFrameCyclic() {
    if (m_numFrames > 0) {
        m_currentFrame = (m_currentFrame == 0) ? (m_numFrames - 1) : (m_currentFrame - 1);
    }
}

void RawVideoReader::SeekFrame(uint32_t frame) {
    if (m_numFrames > 0) {
        m_currentFrame = frame % m_numFrames;
    }
}
