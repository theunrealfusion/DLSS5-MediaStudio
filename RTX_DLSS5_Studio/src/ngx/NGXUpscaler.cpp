//----------------------------------------------------------------------------------
// File:        NGXUpscaler.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Implementation of NGX VSR and TrueHDR feature lifecycles
//----------------------------------------------------------------------------------

#include "NGXUpscaler.h"
#include "../core/Logger.h"
#include "NGXTelemetry.h"

#define APP_ID 0x26005090
#define APP_PATH L"."

NGXUpscaler::NGXUpscaler() {}

NGXUpscaler::~NGXUpscaler() {
    Shutdown();
}

bool NGXUpscaler::Initialize(ID3D12Device* pDevice, const std::wstring& logDir) {
    if (!pDevice) {
        LOG_ERROR("NGXUpscaler::Initialize: ID3D12Device is null");
        return false;
    }
    m_pDevice = pDevice;

    // Initialize NGX SDK on Direct3D 12
    NVSDK_NGX_Result status = NVSDK_NGX_D3D12_Init(APP_ID, logDir.c_str(), m_pDevice);
    if (NVSDK_NGX_FAILED(status)) {
        LOG_ERROR("NGXUpscaler::Initialize: NVSDK_NGX_D3D12_Init failed with code: " + std::to_string(status));
        return false;
    }

    // Retrieve Capability Parameters interface
    status = NVSDK_NGX_D3D12_GetCapabilityParameters(&m_pNgxParameters);
    if (NVSDK_NGX_FAILED(status) || !m_pNgxParameters) {
        LOG_ERROR("NGXUpscaler::Initialize: NVSDK_NGX_D3D12_GetCapabilityParameters failed");
        return false;
    }

    // Query VSR availability
    int vsrAvail = 0;
    if (NVSDK_NGX_SUCCEED(m_pNgxParameters->Get(NVSDK_NGX_Parameter_VSR_Available, &vsrAvail))) {
        m_vsrAvailable = (vsrAvail != 0);
    }

    // Query TrueHDR availability
    int trueHdrAvail = 0;
    if (NVSDK_NGX_SUCCEED(m_pNgxParameters->Get(NVSDK_NGX_Parameter_TrueHDR_Available, &trueHdrAvail))) {
        m_trueHdrAvailable = (trueHdrAvail != 0);
    }

    LOG_INFO("NVIDIA NGX Initialized. VSR Available: " + std::string(m_vsrAvailable ? "YES" : "NO") +
             ", TrueHDR Available: " + std::string(m_trueHdrAvailable ? "YES" : "NO"));

    m_isInitialized = true;
    return true;
}

void NGXUpscaler::Shutdown() {
    if (m_pVsrHandle) {
        NVSDK_NGX_D3D12_ReleaseFeature(m_pVsrHandle);
        m_pVsrHandle = nullptr;
    }
    if (m_pTrueHdrHandle) {
        NVSDK_NGX_D3D12_ReleaseFeature(m_pTrueHdrHandle);
        m_pTrueHdrHandle = nullptr;
    }
    if (m_pNgxParameters) {
        NVSDK_NGX_D3D12_DestroyParameters(m_pNgxParameters);
        m_pNgxParameters = nullptr;
    }
    if (m_isInitialized) {
        if (m_pDevice) {
            NVSDK_NGX_D3D12_Shutdown1(m_pDevice);
        } else {
            NVSDK_NGX_D3D12_Shutdown1(nullptr);
        }
        m_isInitialized = false;
    }
    m_pDevice = nullptr;
    LOG_INFO("NVIDIA NGX Subsystem Shutdown.");
}

bool NGXUpscaler::CreateVSRFeature(ID3D12GraphicsCommandList* pCmdList, uint32_t nodeMask) {
    if (!m_isInitialized || !m_vsrAvailable || !pCmdList) {
        LOG_ERROR("NGXUpscaler::CreateVSRFeature: Invalid state or feature unavailable");
        return false;
    }

    if (m_pVsrHandle) {
        NVSDK_NGX_D3D12_ReleaseFeature(m_pVsrHandle);
        m_pVsrHandle = nullptr;
    }

    NVSDK_NGX_Feature_Create_Params createParams = {};
    NVSDK_NGX_Result status = NGX_D3D12_CREATE_VSR_EXT(pCmdList, nodeMask, nodeMask, &m_pVsrHandle, m_pNgxParameters, &createParams);
    if (NVSDK_NGX_FAILED(status)) {
        LOG_ERROR("NGXUpscaler::CreateVSRFeature: Failed to create VSR feature. Code: " + std::to_string(status));
        return false;
    }

    LOG_INFO("VSR Feature Instance created successfully on D3D12.");
    return true;
}

bool NGXUpscaler::CreateTrueHDRFeature(ID3D12GraphicsCommandList* pCmdList, uint32_t nodeMask) {
    if (!m_isInitialized || !m_trueHdrAvailable || !pCmdList) {
        LOG_ERROR("NGXUpscaler::CreateTrueHDRFeature: Invalid state or feature unavailable");
        return false;
    }

    if (m_pTrueHdrHandle) {
        NVSDK_NGX_D3D12_ReleaseFeature(m_pTrueHdrHandle);
        m_pTrueHdrHandle = nullptr;
    }

    NVSDK_NGX_Feature_Create_Params createParams = {};
    NVSDK_NGX_Result status = NGX_D3D12_CREATE_TRUEHDR_EXT(pCmdList, nodeMask, nodeMask, &m_pTrueHdrHandle, m_pNgxParameters, &createParams);
    if (NVSDK_NGX_FAILED(status)) {
        LOG_ERROR("NGXUpscaler::CreateTrueHDRFeature: Failed to create TrueHDR feature. Code: " + std::to_string(status));
        return false;
    }

    LOG_INFO("TrueHDR Feature Instance created successfully on D3D12.");
    return true;
}

bool NGXUpscaler::EvaluateVSR(
    ID3D12GraphicsCommandList* pCmdList,
    ID3D12Resource* pInput,
    ID3D12Resource* pOutput,
    const NVSDK_NGX_Coordinates& inBase,
    const NVSDK_NGX_Dimensions& inSize,
    const NVSDK_NGX_Coordinates& outBase,
    const NVSDK_NGX_Dimensions& outSize,
    uint32_t qualityLevel
) {
    if (!m_pVsrHandle || !pCmdList || !pInput || !pOutput) {
        return false;
    }

    NVSDK_NGX_D3D12_VSR_Eval_Params evalParams = {};
    evalParams.pInput = pInput;
    evalParams.pOutput = pOutput;
    evalParams.InputSubrectBase = inBase;
    evalParams.InputSubrectSize = inSize;
    evalParams.OutputSubrectBase = outBase;
    evalParams.OutputSubrectSize = outSize;
    evalParams.QualityLevel = static_cast<NVSDK_NGX_VSR_QualityLevel>(qualityLevel);

    NGXTelemetry::Instance().StartFrameTiming();
    NVSDK_NGX_Result status = NGX_D3D12_EVALUATE_VSR_EXT(pCmdList, m_pVsrHandle, m_pNgxParameters, &evalParams);
    NGXTelemetry::Instance().EndFrameTiming();

    if (NVSDK_NGX_FAILED(status)) {
        LOG_ERROR("NGXUpscaler::EvaluateVSR failed with status: " + std::to_string(status));
        return false;
    }

    return true;
}

bool NGXUpscaler::EvaluateTrueHDR(
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
) {
    if (!m_pTrueHdrHandle || !pCmdList || !pInput || !pOutput) {
        return false;
    }

    NVSDK_NGX_D3D12_TRUEHDR_Eval_Params evalParams = {};
    evalParams.pInput = pInput;
    evalParams.pOutput = pOutput;
    evalParams.InputSubrectTL = inBase;
    evalParams.InputSubrectBR = inSize;
    evalParams.OutputSubrectTL = outBase;
    evalParams.OutputSubrectBR = outSize;
    evalParams.Contrast = contrast;
    evalParams.Saturation = saturation;
    evalParams.MiddleGray = middleGray;
    evalParams.MaxLuminance = maxLuminance;

    NVSDK_NGX_Result status = NGX_D3D12_EVALUATE_TRUEHDR_EXT(pCmdList, m_pTrueHdrHandle, m_pNgxParameters, &evalParams);
    return NVSDK_NGX_SUCCEED(status);
}
