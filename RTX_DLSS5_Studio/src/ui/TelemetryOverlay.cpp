//----------------------------------------------------------------------------------
// File:        TelemetryOverlay.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Format HUD string with GPU metrics, inference times, and comparison mode
//----------------------------------------------------------------------------------

#include "TelemetryOverlay.h"
#include <sstream>
#include <iomanip>

TelemetryOverlay::TelemetryOverlay() {}

TelemetryOverlay::~TelemetryOverlay() {}

std::wstring TelemetryOverlay::FormatHUDString(const EngineConfig& config, uint32_t leadBufferFrames, uint32_t leadCapacity) {
    if (!m_visible) return L"";

    TelemetrySnapshot snap = NGXTelemetry::Instance().GetSnapshot();

    std::wostringstream ss;
    ss << L" [NVIDIA RTX DLSS-5 Studio] | " << snap.GPUName << L"\n";
    ss << L" Driver: " << snap.DriverMajor << L"." << snap.DriverMinor;
    ss << L" | FPS: " << std::fixed << std::setprecision(1) << snap.FPS;
    ss << L" | Tensor Eval: " << std::fixed << std::setprecision(2) << snap.LastInferenceTimeMs << L" ms (Avg: "
       << snap.AverageInferenceTimeMs << L" ms)\n";

    ss << L" VSR Quality: ";
    switch (config.VSRQualityLevel) {
        case 0: ss << L"0 (Bicubic Baseline)"; break;
        case 1: ss << L"1 (Low)"; break;
        case 2: ss << L"2 (Medium)"; break;
        case 3: ss << L"3 (High)"; break;
        case 4: ss << L"4 (Ultra - RTX 5090 Blackwell Tuned)"; break;
        default: ss << config.VSRQualityLevel; break;
    }

    ss << L" | TrueHDR: " << (config.EnableTrueHDR ? (config.OutputFP16 ? L"Active (scRGB FP16)" : L"Active (HDR10)") : L"Off");
    ss << L"\n Mode: ";
    switch (config.ActiveComparisonMode) {
        case ComparisonMode::FullUpscaled: ss << L"Full DLSS-5 Upscaled"; break;
        case ComparisonMode::OriginalBaseline: ss << L"Original Source Baseline"; break;
        case ComparisonMode::SplitScreen:
            ss << L"Split-Screen Wipe [" << std::fixed << std::setprecision(0) << (config.SplitPosition * 100.0f) << L"%]";
            break;
        case ComparisonMode::SideBySide: ss << L"Side-by-Side (SBS 50/50)"; break;
        case ComparisonMode::DifferenceHeatmap:
            ss << L"Difference Heatmap (Gain: " << std::fixed << std::setprecision(1) << config.DifferenceGain << L"x)";
            break;
        case ComparisonMode::QuadGrid: ss << L"2x2 Quad Grid Evaluation"; break;
        case ComparisonMode::InstantToggle: ss << L"Instant A/B Flip Mode"; break;
    }

    if (config.LoupeActive) {
        ss << L" | Loupe: " << std::fixed << std::setprecision(0) << config.LoupeMagnification << L"x Zoom Active";
    }

    ss << L"\n 32GB Lead Buffer: [" << leadBufferFrames << L" / " << leadCapacity << L" Frames Ahead]";
    ss << L" | VRAM: " << (snap.AllocatedVRAMBytes / (1024 * 1024)) << L" MB";
    ss << L" | [D] Mode  [L] Loupe  [1-4] VSR  [H] HDR  [Space] Pause  [O] Open  [< >] Step  [F12] Snapshot";

    return ss.str();
}
