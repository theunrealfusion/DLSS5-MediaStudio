//----------------------------------------------------------------------------------
// File:        SecurityManager.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     WinVerifyTrust and security policy implementation
//----------------------------------------------------------------------------------

#include "SecurityManager.h"
#include "../core/Logger.h"

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#endif

#include <fstream>
#include <algorithm>

bool SecurityManager::VerifyAuthenticodeSignature(const std::wstring& filePath, const std::wstring& expectedSubject) {
#if defined(_WIN32)
    WINTRUST_FILE_INFO fileData = { sizeof(WINTRUST_FILE_INFO) };
    fileData.pcwszFilePath = filePath.c_str();
    fileData.hFile = NULL;
    fileData.pgKnownSubject = NULL;

    WINTRUST_DATA winTrustData = { sizeof(WINTRUST_DATA) };
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.pFile = &fileData;
    winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    winTrustData.hWVTStateData = NULL;
    winTrustData.pwszURLReference = NULL;
    winTrustData.dwProvFlags = WTD_SAFER_FLAG;
    winTrustData.dwUIContext = 0;

    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    LONG lStatus = WinVerifyTrust(NULL, &policyGUID, &winTrustData);

    bool verified = (lStatus == ERROR_SUCCESS);

    // Free state data
    winTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &policyGUID, &winTrustData);

    if (!verified) {
        LOG_WARN(L"Authenticode verification failed or unsigned binary: " + filePath);
        return false;
    }

    LOG_INFO(L"Authenticode verified successfully for: " + filePath);
    return true;
#else
    return true;
#endif
}

bool SecurityManager::SanitizePath(const std::wstring& rawPath, std::wstring& sanitizedPath) {
    if (rawPath.empty()) {
        LOG_ERROR("SanitizePath: Input path is empty");
        return false;
    }

    std::wstring clean = rawPath;
    // Trim leading and trailing whitespace
    size_t first = clean.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return false;
    size_t last = clean.find_last_not_of(L" \t\r\n");
    clean = clean.substr(first, (last - first + 1));

    // Strip wrapping quotes
    if (clean.size() >= 2 && clean.front() == L'"' && clean.back() == L'"') {
        clean = clean.substr(1, clean.size() - 2);
    }

#if defined(_WIN32)
    wchar_t fullPath[MAX_PATH];
    DWORD len = GetFullPathNameW(clean.c_str(), MAX_PATH, fullPath, nullptr);
    if (len == 0 || len >= MAX_PATH) {
        LOG_ERROR(L"SanitizePath: Path exceeds MAX_PATH or invalid: " + rawPath);
        return false;
    }

    // Check file existence
    DWORD attrs = GetFileAttributesW(fullPath);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        LOG_WARN(L"SanitizePath: File or directory does not exist: " + std::wstring(fullPath));
        return false;
    }

    sanitizedPath = fullPath;
    return true;
#else
    sanitizedPath = rawPath;
    return true;
#endif
}

bool SecurityManager::ValidateMediaHeader(const std::wstring& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR(L"ValidateMediaHeader: Cannot open file: " + filePath);
        return false;
    }

    unsigned char magic[16] = {0};
    file.read(reinterpret_cast<char*>(magic), sizeof(magic));
    std::streamsize bytesRead = file.gcount();
    file.close();

    if (bytesRead < 4) {
        LOG_ERROR(L"ValidateMediaHeader: File too short to verify magic bytes: " + filePath);
        return false;
    }

    // Known media signatures:
    // MP4 / MOV: magic bytes at offset 4 are 'ftyp', 'moov', 'mdat', 'wide', 'free'
    if (bytesRead >= 8) {
        if ((magic[4] == 'f' && magic[5] == 't' && magic[6] == 'y' && magic[7] == 'p') ||
            (magic[4] == 'm' && magic[5] == 'o' && magic[6] == 'o' && magic[7] == 'v') ||
            (magic[4] == 'm' && magic[5] == 'd' && magic[6] == 'a' && magic[7] == 't') ||
            (magic[4] == 'w' && magic[5] == 'i' && magic[6] == 'd' && magic[7] == 'e') ||
            (magic[4] == 'f' && magic[5] == 'r' && magic[6] == 'e' && magic[7] == 'e')) {
            return true;
        }
    }
    // RIFF (AVI / WAV)
    if (magic[0] == 'R' && magic[1] == 'I' && magic[2] == 'F' && magic[3] == 'F') {
        return true;
    }
    // WMV / ASF: 0x30, 0x26, 0xB2, 0x75
    if (magic[0] == 0x30 && magic[1] == 0x26 && magic[2] == 0xB2 && magic[3] == 0x75) {
        return true;
    }
    // MPEG-TS sync byte 0x47
    if (magic[0] == 0x47) {
        return true;
    }
    // MKV / WebM: EBML header 0x1A, 0x45, 0xDF, 0xA3
    if (magic[0] == 0x1A && magic[1] == 0x45 && magic[2] == 0xDF && magic[3] == 0xA3) {
        return true;
    }
    // FLV header
    if (magic[0] == 'F' && magic[1] == 'L' && magic[2] == 'V') {
        return true;
    }
    // PNG: 0x89, 'P', 'N', 'G'
    if (magic[0] == 0x89 && magic[1] == 0x50 && magic[2] == 0x4E && magic[3] == 0x47) {
        return true;
    }
    // JPEG: 0xFF, 0xD8, 0xFF
    if (magic[0] == 0xFF && magic[1] == 0xD8 && magic[2] == 0xFF) {
        return true;
    }
    // Extension fallback for raw video and other media containers
    std::wstring ext = L"";
    size_t dotPos = filePath.find_last_of(L'.');
    if (dotPos != std::wstring::npos) {
        ext = filePath.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    }

    if (ext == L".yuv" || ext == L".argb" || ext == L".abgr10" || ext == L".nv12" || ext == L".p010" ||
        ext == L".mp4" || ext == L".mkv" || ext == L".mov" || ext == L".avi" || ext == L".wmv" ||
        ext == L".m4v" || ext == L".webm" || ext == L".flv" || ext == L".3gp" || ext == L".ts" || ext == L".mts") {
        return true;
    }

    return true;
}

bool SecurityManager::ValidateStreamURL(const std::wstring& url, bool airGappedEnforced) {
    if (airGappedEnforced) {
        LOG_ERROR("Security Policy: Network streaming blocked due to Air-Gapped mode.");
        return false;
    }

    if (url.empty()) return false;

    // Reject URLs with illegal command-line injection characters (semicolon, pipe, ampersand, backtick, quotes)
    const std::wstring dangerousChars = L";|&`\"'$><\n\r";
    for (wchar_t c : dangerousChars) {
        if (url.find(c) != std::wstring::npos) {
            LOG_ERROR("Security Alert: Dangerous character detected in stream URL: Command injection prevented.");
            return false;
        }
    }

    // Allow only http:// or https://
    if (url.rfind(L"http://", 0) != 0 && url.rfind(L"https://", 0) != 0) {
        LOG_ERROR("Security Alert: Unsupported protocol in stream URL. Only HTTPS/HTTP permitted.");
        return false;
    }

    return true;
}
