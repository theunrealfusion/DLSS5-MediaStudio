//----------------------------------------------------------------------------------
// File:        EngineConfig.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Thread-safe configuration data structures, presets, and enums
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <cstdint>

enum class ComparisonMode : uint32_t {
    FullUpscaled = 0,       // Clean DLSS / VSR + TrueHDR full output
    OriginalBaseline = 1,   // Original unprocessed / bicubic frame
    SplitScreen = 2,        // Draggable vertical / horizontal wipe divider
    SideBySide = 3,         // 50/50 dual side-by-side synchronized view
    DifferenceHeatmap = 4,  // |I_AI - I_Orig| * Gain in Turbo/Jet or Grayscale
    QuadGrid = 5,           // 2x2 multi-view: Original, VSR, TrueHDR, Combined
    InstantToggle = 6       // A/B flip toggle on shortcut key
};

enum class DifferenceColorMap : uint32_t {
    Grayscale = 0,
    Turbo = 1,
    Jet = 2,
    Hot = 3
};

enum class LoupeShape : uint32_t {
    Circular = 0,
    Rectangular = 1
};

enum class VideoCodecExport : uint32_t {
    AV1 = 0,                // RTX 5090 Dual NVENC 9th-gen AV1
    HEVC = 1                // High-efficiency H.265
};

struct EngineConfig {
    // Input / Output paths
    std::wstring InputFilePath = L"";
    std::wstring OutputExportPath = L"";

    // Video Dimensions
    uint32_t SourceWidth = 0;
    uint32_t SourceHeight = 0;
    uint32_t TargetWidth = 3840;        // Default 4K target
    uint32_t TargetHeight = 2160;

    // NVIDIA NGX Super Resolution (VSR)
    bool EnableVSR = true;
    uint32_t VSRQualityLevel = 4;       // 0=Bicubic, 1=Low, 2=Medium, 3=High, 4=Ultra (5090 optimized)

    // NVIDIA NGX TrueHDR
    bool EnableTrueHDR = false;
    bool OutputFP16 = false;             // FP16 scRGB for wide color gamut (enabled when TrueHDR is active)
    uint32_t HDRContrast = 100;         // 0 to 200
    uint32_t HDRSaturation = 100;       // 0 to 200
    uint32_t HDRMiddleGray = 50;        // 10 to 100
    uint32_t HDRMaxLuminance = 1000;    // 400 to 2000 nits

    // Lead-buffer ring configuration (Optimized for 32GB RTX 5090)
    uint32_t LeadBufferCapacity = 90;   // 90 pre-rendered 4K frames (~3s at 30fps / 1.5s at 60fps)
    bool AdaptiveFallback = true;       // Seamless fallback to bicubic on seek/starvation

    // Comparison Mode settings
    ComparisonMode ActiveComparisonMode = ComparisonMode::SplitScreen;
    float SplitPosition = 0.5f;         // 0.0 to 1.0 divider
    bool SplitIsVertical = true;
    float DifferenceGain = 5.0f;        // Multiplier for difference mode
    DifferenceColorMap DiffMap = DifferenceColorMap::Turbo;

    // Interactive Loupe (Magnifier)
    bool LoupeActive = false;
    float LoupeMagnification = 4.0f;    // 2x, 4x, 8x
    float LoupeRadius = 150.0f;         // In screen pixels
    float LoupeCenterNormX = 0.5f;
    float LoupeCenterNormY = 0.5f;
    LoupeShape Shape = LoupeShape::Circular;

    // Security Policies
    bool EnforceStrictAuthenticode = true;
    bool AirGappedMode = true;          // Prevent unapproved outbound network requests
    bool AuditLogging = true;

    // Display & Presentation
    bool EnableVSync = false;           // Set false for ultra-low latency presentation
    bool NativeFullscreen = false;
};
