//----------------------------------------------------------------------------------
// File:        ComparisonRenderer.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Shader composition pipeline for the 7 comparison evaluation modes
//----------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include "../security/MemorySafety.h"
#include "../core/EngineConfig.h"

struct ShaderComparisonCB {
    float SplitPosition;
    uint32_t Mode;
    float DifferenceGain;
    uint32_t DiffColorMap;

    float LoupeCenterX;
    float LoupeCenterY;
    float LoupeRadius;
    float LoupeZoom;

    float DividerColor[4];
    float DividerWidth;
    uint32_t IsVerticalSplit;
    float ViewportWidth;
    float ViewportHeight;
};

class ComparisonRenderer {
public:
    ComparisonRenderer();
    ~ComparisonRenderer();

    bool Initialize(ID3D12Device* pDevice, DXGI_FORMAT renderTargetFormat);
    void Shutdown();

    void Render(
        ID3D12GraphicsCommandList* pCmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvTableHandle,
        const EngineConfig& config,
        uint32_t viewportWidth,
        uint32_t viewportHeight
    );

private:
    ComPtr<ID3D12RootSignature> m_pRootSignature;
    ComPtr<ID3D12PipelineState> m_pPipelineState;
    ComPtr<ID3D12Resource> m_pConstantBuffer;
    void* m_pCBData = nullptr;

    ID3D12Device* m_pDevice = nullptr;
};
