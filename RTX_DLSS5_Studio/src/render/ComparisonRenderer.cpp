//----------------------------------------------------------------------------------
// File:        ComparisonRenderer.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Direct3D 12 Comparison Shader pipeline execution
//----------------------------------------------------------------------------------

#include "ComparisonRenderer.h"
#include "../core/Logger.h"
#include <vector>
#include <filesystem>

#pragma comment(lib, "d3dcompiler.lib")

namespace {
const char g_EmbeddedShadersHLSL[] = R"(
struct VSInput {
    uint VertexID : SV_VertexID;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

cbuffer ComparisonParams : register(b0) {
    float g_SplitPosition;      // 0.0 to 1.0
    uint  g_Mode;               // 0: Full, 1: Orig, 2: Split, 3: SBS, 4: Diff, 5: Quad, 6: Loupe
    float g_DifferenceGain;     // e.g. 5.0
    uint  g_DiffColorMap;       // 0: Grayscale, 1: Turbo, 2: Jet, 3: Hot

    float2 g_LoupeCenter;       // Normalized coordinates (0 to 1)
    float  g_LoupeRadius;       // In UV space or normalized radius
    float  g_LoupeZoom;         // 2.0, 4.0, 8.0

    float4 g_DividerColor;      // RGBA color of split divider line
    float  g_DividerWidth;      // Thickness in UV
    uint   g_IsVerticalSplit;   // 1 for vertical, 0 for horizontal
    float2 g_ViewportSize;      // Rendered video width and height in screen pixels
};

Texture2D<float4> g_TexOriginal : register(t0);
Texture2D<float4> g_TexUpscaled : register(t1);
SamplerState      g_LinearSampler : register(s0);

PSInput VSMain(VSInput input) {
    PSInput output;
    output.TexCoord = float2((input.VertexID << 1) & 2, input.VertexID & 2);
    output.Position = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

float3 TurboColorMap(float x) {
    x = saturate(x);
    const float4 kRedVec4 = float4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const float4 kGreenVec4 = float4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    const float4 kBlueVec4 = float4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    const float2 kRedVec2 = float2(-152.94239396, 59.28637943);
    const float2 kGreenVec2 = float2(4.27729857, 2.82956604);
    const float2 kBlueVec2 = float2(-89.90310912, 27.34824973);

    float4 v4 = float4(1.0, x, x * x, x * x * x);
    float2 v2 = v4.zw * v4.z;

    return float3(
        dot(v4, kRedVec4) + dot(v2, kRedVec2),
        dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
        dot(v4, kBlueVec4) + dot(v2, kBlueVec2)
    );
}

float4 PSMain(PSInput input) : SV_TARGET {
    float2 uv = input.TexCoord;

    if (g_LoupeZoom > 1.0f) {
        float2 loupeDelta = (uv - g_LoupeCenter) * g_ViewportSize;
        float distPx = length(loupeDelta);
        float radiusPx = g_LoupeRadius * g_ViewportSize.y;
        if (distPx <= radiusPx) {
            if (distPx >= radiusPx - 3.0f) {
                return float4(0.0f, 0.85f, 1.0f, 1.0f);
            }
            float2 zoomedUV = g_LoupeCenter + (uv - g_LoupeCenter) / g_LoupeZoom;
            return g_TexUpscaled.Sample(g_LinearSampler, zoomedUV);
        }
    }

    if (g_Mode == 0) {
        return g_TexUpscaled.Sample(g_LinearSampler, uv);
    }

    if (g_Mode == 1) {
        return g_TexOriginal.Sample(g_LinearSampler, uv);
    }

    if (g_Mode == 2) {
        float splitCoord = g_IsVerticalSplit ? uv.x : uv.y;
        float coordPx = g_IsVerticalSplit ? (uv.x * g_ViewportSize.x) : (uv.y * g_ViewportSize.y);
        float splitPx = g_IsVerticalSplit ? (g_SplitPosition * g_ViewportSize.x) : (g_SplitPosition * g_ViewportSize.y);
        float distToLinePx = abs(coordPx - splitPx);

        float2 dividerCenter = g_IsVerticalSplit ? float2(g_SplitPosition, 0.5f) : float2(0.5f, g_SplitPosition);
        float2 screenDelta = (uv - dividerCenter) * g_ViewportSize;
        float handleDistPx = length(screenDelta);

        if (handleDistPx <= 22.0f) {
            if (handleDistPx >= 19.0f) {
                return float4(0.0f, 0.0f, 0.0f, 1.0f);
            } else if (handleDistPx >= 13.0f) {
                return float4(0.0f, 0.85f, 1.0f, 1.0f);
            } else {
                if (abs(screenDelta.x) <= 1.0f && abs(screenDelta.y) <= 6.0f) {
                    return float4(0.0f, 0.5f, 0.8f, 1.0f);
                }
                return float4(0.98f, 0.98f, 0.98f, 1.0f);
            }
        }

        if (distToLinePx <= 1.0f) {
            return g_DividerColor;
        }
        if (distToLinePx <= 2.2f) {
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        if (splitCoord < g_SplitPosition) {
            return g_TexOriginal.Sample(g_LinearSampler, uv);
        } else {
            return g_TexUpscaled.Sample(g_LinearSampler, uv);
        }
    }

    if (g_Mode == 3) {
        if (uv.x < 0.5f) {
            float2 sbsUV = float2(uv.x * 2.0f, uv.y);
            return g_TexOriginal.Sample(g_LinearSampler, sbsUV);
        } else {
            if (abs(uv.x - 0.5f) < 0.002f) return g_DividerColor;
            float2 sbsUV = float2((uv.x - 0.5f) * 2.0f, uv.y);
            return g_TexUpscaled.Sample(g_LinearSampler, sbsUV);
        }
    }

    if (g_Mode == 4) {
        float4 cOrig = g_TexOriginal.Sample(g_LinearSampler, uv);
        float4 cUp = g_TexUpscaled.Sample(g_LinearSampler, uv);
        float3 diff = abs(cUp.rgb - cOrig.rgb) * g_DifferenceGain;
        float intensity = dot(diff, float3(0.299f, 0.587f, 0.114f));

        if (g_DiffColorMap == 1) {
            return float4(TurboColorMap(intensity), 1.0f);
        } else {
            return float4(saturate(diff), 1.0f);
        }
    }

    if (g_Mode == 5) {
        float2 quadUV = frac(uv * 2.0f);
        if (abs(uv.x - 0.5f) < 0.002f || abs(uv.y - 0.5f) < 0.002f) {
            return g_DividerColor;
        }

        if (uv.x < 0.5f && uv.y < 0.5f) {
            return g_TexOriginal.Sample(g_LinearSampler, quadUV);
        } else if (uv.x >= 0.5f && uv.y < 0.5f) {
            return g_TexUpscaled.Sample(g_LinearSampler, quadUV);
        } else if (uv.x < 0.5f && uv.y >= 0.5f) {
            float4 cOrig = g_TexOriginal.Sample(g_LinearSampler, quadUV);
            float4 cUp = g_TexUpscaled.Sample(g_LinearSampler, quadUV);
            float intensity = dot(abs(cUp.rgb - cOrig.rgb) * g_DifferenceGain, float3(0.299f, 0.587f, 0.114f));
            return float4(TurboColorMap(intensity), 1.0f);
        } else {
            return g_TexUpscaled.Sample(g_LinearSampler, quadUV);
        }
    }

    return g_TexUpscaled.Sample(g_LinearSampler, uv);
}
)";
}

ComparisonRenderer::ComparisonRenderer() {}

ComparisonRenderer::~ComparisonRenderer() {
    Shutdown();
}

bool ComparisonRenderer::Initialize(ID3D12Device* pDevice, DXGI_FORMAT renderTargetFormat) {
    if (!pDevice) return false;
    m_pDevice = pDevice;

    // 1. Compile Shaders
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    std::vector<std::wstring> shaderCandidates;
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
        std::wstring exeDir = exePath;
        size_t lastSlash = exeDir.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            exeDir = exeDir.substr(0, lastSlash);
            shaderCandidates.push_back(exeDir + L"/Shaders.hlsl");
            shaderCandidates.push_back(exeDir + L"/src/render/Shaders.hlsl");
            shaderCandidates.push_back(exeDir + L"/../../src/render/Shaders.hlsl");
        }
    }
    shaderCandidates.push_back(L"RTX_DLSS5_Studio/src/render/Shaders.hlsl");
    shaderCandidates.push_back(L"RTX_DLSS5_Studio/bin/x64/Release/Shaders.hlsl");
    shaderCandidates.push_back(L"src/render/Shaders.hlsl");
    shaderCandidates.push_back(L"Shaders.hlsl");

    std::wstring resolvedShaderPath = L"";
    for (const auto& path : shaderCandidates) {
        if (std::filesystem::exists(path)) {
            resolvedShaderPath = path;
            break;
        }
    }

    HRESULT hr = E_FAIL;
    if (!resolvedShaderPath.empty()) {
        hr = D3DCompileFromFile(resolvedShaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vsBlob, &errorBlob);
        if (SUCCEEDED(hr)) {
            hr = D3DCompileFromFile(resolvedShaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &psBlob, &errorBlob);
        }
    }

    if (FAILED(hr)) {
        hr = D3DCompile(g_EmbeddedShadersHLSL, sizeof(g_EmbeddedShadersHLSL) - 1, "EmbeddedShaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) LOG_ERROR("Embedded VS Compile Error: " + std::string((char*)errorBlob->GetBufferPointer()));
            return false;
        }
        hr = D3DCompile(g_EmbeddedShadersHLSL, sizeof(g_EmbeddedShadersHLSL) - 1, "EmbeddedShaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) LOG_ERROR("Embedded PS Compile Error: " + std::string((char*)errorBlob->GetBufferPointer()));
            return false;
        }
        LOG_INFO("ComparisonRenderer: Successfully compiled embedded HLSL shader suite.");
    } else {
        LOG_INFO(L"ComparisonRenderer: Successfully compiled HLSL shaders from: " + resolvedShaderPath);
    }

    // 2. Root Signature: Constant Buffer (b0) + Descriptor Table (t0, t1) + Static Sampler (s0)
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 2; // t0: Orig, t1: Upscaled
    range.BaseShaderRegister = 0;
    range.RegisterSpace = 0;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParams[2] = {};
    // Param 0: Constant Buffer (CBV at b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Param 1: SRV Descriptor Table
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &range;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static Linear Clamp Sampler
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &samplerDesc;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serializedRootSig;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (FAILED(hr)) return false;

    hr = m_pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
    if (FAILED(hr)) return false;

    // 3. Pipeline State Object (PSO)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_pRootSignature.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState = blendDesc;

    psoDesc.SampleMask = UINT_MAX;

    D3D12_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState = rasterDesc;

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.InputLayout = { nullptr, 0 }; // Fullscreen triangle without input layout
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = renderTargetFormat;
    psoDesc.SampleDesc.Count = 1;

    hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
    if (FAILED(hr)) return false;

    // 4. Constant Buffer (aligned to 256 bytes)
    UINT cbSize = (sizeof(ShaderComparisonCB) + 255) & ~255;
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = cbSize;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pConstantBuffer)
    );
    if (FAILED(hr)) return false;

    D3D12_RANGE readRange = { 0, 0 };
    hr = m_pConstantBuffer->Map(0, &readRange, &m_pCBData);
    if (FAILED(hr)) return false;

    LOG_INFO("ComparisonRenderer initialized with full shader suite.");
    return true;
}

void ComparisonRenderer::Shutdown() {
    if (m_pConstantBuffer) {
        m_pConstantBuffer->Unmap(0, nullptr);
        m_pCBData = nullptr;
    }
    m_pConstantBuffer.Reset();
    m_pPipelineState.Reset();
    m_pRootSignature.Reset();
    m_pDevice = nullptr;
}

void ComparisonRenderer::Render(
    ID3D12GraphicsCommandList* pCmdList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE srvTableHandle,
    const EngineConfig& config,
    uint32_t viewportWidth,
    uint32_t viewportHeight
) {
    if (!pCmdList || !m_pCBData) return;

    // Compute aspect-correct viewport to prevent video distortion and stretching
    float videoAspect = (config.SourceHeight > 0) ? (static_cast<float>(config.SourceWidth) / static_cast<float>(config.SourceHeight)) :
                        ((config.TargetHeight > 0) ? (static_cast<float>(config.TargetWidth) / static_cast<float>(config.TargetHeight)) : (16.0f / 9.0f));
    float vpAspect = (viewportHeight > 0) ? (static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)) : (16.0f / 9.0f);

    float renderW = static_cast<float>(viewportWidth);
    float renderH = static_cast<float>(viewportHeight);
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    if (vpAspect > videoAspect) {
        // Window is wider than video -> pillarbox with black bars left/right
        renderW = renderH * videoAspect;
        offsetX = (static_cast<float>(viewportWidth) - renderW) * 0.5f;
    } else if (vpAspect < videoAspect) {
        // Window is taller than video -> letterbox with black bars top/bottom
        renderH = renderW / videoAspect;
        offsetY = (static_cast<float>(viewportHeight) - renderH) * 0.5f;
    }

    // Update Constant Buffer
    ShaderComparisonCB cb = {};
    cb.SplitPosition = config.SplitPosition;
    cb.Mode = static_cast<uint32_t>(config.ActiveComparisonMode);
    cb.DifferenceGain = config.DifferenceGain;
    cb.DiffColorMap = static_cast<uint32_t>(config.DiffMap);

    cb.LoupeCenterX = config.LoupeCenterNormX;
    cb.LoupeCenterY = config.LoupeCenterNormY;
    cb.LoupeRadius = (config.LoupeRadius / (renderH > 0.0f ? renderH : static_cast<float>(viewportHeight)));
    cb.LoupeZoom = config.LoupeActive ? config.LoupeMagnification : 1.0f;

    cb.DividerColor[0] = 1.0f; // White separator
    cb.DividerColor[1] = 1.0f;
    cb.DividerColor[2] = 1.0f;
    cb.DividerColor[3] = 1.0f;
    cb.DividerWidth = 0.002f;
    cb.IsVerticalSplit = config.SplitIsVertical ? 1 : 0;
    cb.ViewportWidth = renderW;
    cb.ViewportHeight = renderH;

    memcpy(m_pCBData, &cb, sizeof(cb));

    // Viewport & Scissor with aspect-correct framing
    D3D12_VIEWPORT vp = { offsetX, offsetY, renderW, renderH, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(viewportWidth), static_cast<LONG>(viewportHeight) };

    pCmdList->RSSetViewports(1, &vp);
    pCmdList->RSSetScissorRects(1, &scissor);

    pCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
    pCmdList->SetPipelineState(m_pPipelineState.Get());

    pCmdList->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());
    pCmdList->SetGraphicsRootDescriptorTable(1, srvTableHandle);

    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmdList->DrawInstanced(3, 1, 0, 0); // 3 vertices for fullscreen quad
}
