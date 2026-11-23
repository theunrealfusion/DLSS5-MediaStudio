//----------------------------------------------------------------------------------
// File:        NGXUpscaler.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     NVIDIA RTX Video SDK v1.1.0 NGX VSR & TrueHDR Integration
//----------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "../security/MemorySafety.h"
#include "../core/EngineConfig.h"

// NVIDIA NGX SDK Headers
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_vsr.h>
#include <nvsdk_ngx_helpers_truehdr.h>

class NGXUpscaler {
public:
    NGXUpscaler();
    ~NGXUpscaler();

    // Initialize NGX Subsystem on given D3D12 Device
    bool Initialize(ID3D12Device* pDevice, const std::wstring& logDir = L".");
    void Shutdown();

    // Feature setup
    bool CreateVSRFeature(ID3D12GraphicsCommandList* pCmdList, uint32_t nodeMask = 0);
    bool CreateTrueHDRFeature(ID3D12GraphicsCommandList* pCmdList, uint32_t nodeMask = 0);

    // Evaluation
    bool EvaluateVSR(
        ID3D12GraphicsCommandList* pCmdList,
        ID3D12Resource* pInput,
        ID3D12Resource* pOutput,
        const NVSDK_NGX_Coordinates& inBase,
        const NVSDK_NGX_Dimensions& inSize,
        const NVSDK_NGX_Coordinates& outBase,
        const NVSDK_NGX_Dimensions& outSize,
        uint32_t qualityLevel
    );

    bool EvaluateTrueHDR(
        ID3D12GraphicsCommandList* pCmdList,
        ID3D12Resource* pInput,
        ID3D12Resource* pOutput,
        const NVSDK_NGX_Coordinates& inBase,
        const NVSDK_NGX_Dimensions& inSize,
        const NVSDK_NGX_Coordinates& outBase,
        const NVSDK_NGX_Dimensions& outSize,
        uint32_t contrast,
        uint32_t saturation,
        uint32_t middleGray,
        uint32_t maxLuminance
    );

    // Feature availability checks
    bool IsVSRAvailable() const { return m_vsrAvailable; }
    bool IsTrueHDRAvailable() const { return m_trueHdrAvailable; }
    bool IsInitialized() const { return m_isInitialized; }

private:
    ID3D12Device* m_pDevice = nullptr;
    NVSDK_NGX_Parameter* m_pNgxParameters = nullptr;
    NVSDK_NGX_Handle* m_pVsrHandle = nullptr;
    NVSDK_NGX_Handle* m_pTrueHdrHandle = nullptr;

    bool m_isInitialized = false;
    bool m_vsrAvailable = false;
    bool m_trueHdrAvailable = false;
};
