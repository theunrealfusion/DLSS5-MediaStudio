//----------------------------------------------------------------------------------
// File:        SecurityManager.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Authenticode verification, path sanitization, and input integrity
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

class SecurityManager {
public:
    static SecurityManager& Instance() {
        static SecurityManager instance;
        return instance;
    }

    // Cryptographic Authenticode Signature verification for DLLs
    bool VerifyAuthenticodeSignature(const std::wstring& filePath, const std::wstring& expectedSubject = L"NVIDIA Corporation");

    // Path sanitization and directory traversal prevention
    bool SanitizePath(const std::wstring& rawPath, std::wstring& sanitizedPath);

    // Header validation (magic bytes) to prevent polyglot file attacks
    bool ValidateMediaHeader(const std::wstring& filePath);

    // Validates URL and checks air-gapped policy enforcement
    bool ValidateStreamURL(const std::wstring& url, bool airGappedEnforced);

private:
    SecurityManager() = default;
};
