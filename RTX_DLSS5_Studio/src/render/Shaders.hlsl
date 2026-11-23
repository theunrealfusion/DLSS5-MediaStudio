//----------------------------------------------------------------------------------
// File:        Shaders.hlsl
// Project:     RTX DLSS-5 Video Studio
// Purpose:     HLSL Comparison Shaders (Split Wipe, Difference Heatmap, Quad Grid, Loupe)
//----------------------------------------------------------------------------------

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

// Fullscreen Quad Vertex Shader (Single Triangle Trick)
PSInput VSMain(VSInput input) {
    PSInput output;
    output.TexCoord = float2((input.VertexID << 1) & 2, input.VertexID & 2);
    output.Position = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

// Approximate Turbo Colormap Polynomial
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

    // Check Loupe Magnifier first if active (aspect-correct in pixel space)
    if (g_LoupeZoom > 1.0f) {
        float2 loupeDelta = (uv - g_LoupeCenter) * g_ViewportSize;
        float distPx = length(loupeDelta);
        float radiusPx = g_LoupeRadius * g_ViewportSize.y;
        if (distPx <= radiusPx) {
            // Draw Loupe boundary ring
            if (distPx >= radiusPx - 3.0f) {
                return float4(0.0f, 0.85f, 1.0f, 1.0f); // Cyan highlight ring
            }
            // Magnify region inside loupe
            float2 zoomedUV = g_LoupeCenter + (uv - g_LoupeCenter) / g_LoupeZoom;
            return g_TexUpscaled.Sample(g_LinearSampler, zoomedUV);
        }
    }

    // 0: Full Upscaled
    if (g_Mode == 0) {
        return g_TexUpscaled.Sample(g_LinearSampler, uv);
    }

    // 1: Original Baseline
    if (g_Mode == 1) {
        return g_TexOriginal.Sample(g_LinearSampler, uv);
    }

    // 2: Split-Screen Wipe (Original Baseline on Left, DLSS-5 Upscaled on Right)
    if (g_Mode == 2) {
        float splitCoord = g_IsVerticalSplit ? uv.x : uv.y;
        float coordPx = g_IsVerticalSplit ? (uv.x * g_ViewportSize.x) : (uv.y * g_ViewportSize.y);
        float splitPx = g_IsVerticalSplit ? (g_SplitPosition * g_ViewportSize.x) : (g_SplitPosition * g_ViewportSize.y);
        float distToLinePx = abs(coordPx - splitPx);

        // Circular slider handle grip in center of divider (exact pixel-space circular design)
        float2 dividerCenter = g_IsVerticalSplit ? float2(g_SplitPosition, 0.5f) : float2(0.5f, g_SplitPosition);
        float2 screenDelta = (uv - dividerCenter) * g_ViewportSize;
        float handleDistPx = length(screenDelta);

        if (handleDistPx <= 22.0f) {
            if (handleDistPx >= 19.0f) {
                return float4(0.0f, 0.0f, 0.0f, 1.0f); // Handle black outer shadow
            } else if (handleDistPx >= 13.0f) {
                return float4(0.0f, 0.85f, 1.0f, 1.0f); // Neon cyan accent ring
            } else {
                // Inner button: bright white with vertical grip lines
                if (abs(screenDelta.x) <= 1.0f && abs(screenDelta.y) <= 6.0f) {
                    return float4(0.0f, 0.5f, 0.8f, 1.0f); // Center grip bar
                }
                return float4(0.98f, 0.98f, 0.98f, 1.0f); // Bright white inner grip
            }
        }

        // Dividing separator line: 2px bright white line with 1px black outline borders
        if (distToLinePx <= 1.0f) {
            return g_DividerColor; // Crisp white line
        }
        if (distToLinePx <= 2.2f) {
            return float4(0.0f, 0.0f, 0.0f, 1.0f); // High-contrast black outline
        }

        if (splitCoord < g_SplitPosition) {
            // Original Baseline on LEFT
            return g_TexOriginal.Sample(g_LinearSampler, uv);
        } else {
            // DLSS 5 Upscaled on RIGHT
            return g_TexUpscaled.Sample(g_LinearSampler, uv);
        }
    }

    // 3: Side-by-Side Dual View (SBS)
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

    // 4: Difference Heatmap Mode
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

    // 5: 2x2 Quad Grid Mode
    if (g_Mode == 5) {
        float2 quadUV = frac(uv * 2.0f);
        if (abs(uv.x - 0.5f) < 0.002f || abs(uv.y - 0.5f) < 0.002f) {
            return g_DividerColor;
        }

        if (uv.x < 0.5f && uv.y < 0.5f) {
            // Quadrant 1 (Top-Left): Original
            return g_TexOriginal.Sample(g_LinearSampler, quadUV);
        } else if (uv.x >= 0.5f && uv.y < 0.5f) {
            // Quadrant 2 (Top-Right): Neural VSR Upscaled
            return g_TexUpscaled.Sample(g_LinearSampler, quadUV);
        } else if (uv.x < 0.5f && uv.y >= 0.5f) {
            // Quadrant 3 (Bottom-Left): Difference Heatmap
            float4 cOrig = g_TexOriginal.Sample(g_LinearSampler, quadUV);
            float4 cUp = g_TexUpscaled.Sample(g_LinearSampler, quadUV);
            float intensity = dot(abs(cUp.rgb - cOrig.rgb) * g_DifferenceGain, float3(0.299f, 0.587f, 0.114f));
            return float4(TurboColorMap(intensity), 1.0f);
        } else {
            // Quadrant 4 (Bottom-Right): VSR + TrueHDR Combined
            return g_TexUpscaled.Sample(g_LinearSampler, quadUV);
        }
    }

    return g_TexUpscaled.Sample(g_LinearSampler, uv);
}
