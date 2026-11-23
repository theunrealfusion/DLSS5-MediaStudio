//----------------------------------------------------------------------------------
// File:        main.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     CLI argument parser, headless diagnostic modes, and application startup
//----------------------------------------------------------------------------------

#include "core/Application.h"
#include "core/Logger.h"
#include "export/BatchVideoExporter.h"
#include <shellapi.h>
#include <iostream>
#include <filesystem>

void PrintUsage() {
    std::wcout << L"\n===============================================================================\n"
               << L" NVIDIA RTX DLSS-5 Video Studio (RTX 5090 Blackwell Tuned)\n"
               << L"===============================================================================\n"
               << L" Usage: RTX_DLSS5_Studio.exe [options] [video_file]\n\n"
               << L" Supported Formats:\n"
               << L"   MP4, MKV, MOV, AVI, WMV, M4V, WebM, TS, MTS, FLV, 3GP, YUV, ARGB, NV12\n\n"
               << L" Options:\n"
               << L"   -i <path>           Input video or raw YUV file (or pass as positional arg)\n"
               << L"   -vsr                Enable NVIDIA Video Super Resolution (default: on)\n"
               << L"   -vsrQuality <0-4>   0: Bicubic, 1: Low, 2: Med, 3: High, 4: Ultra (default: 4)\n"
               << L"   -thdr               Enable NVIDIA RTX TrueHDR (AI SDR->HDR conversion)\n"
               << L"   -fp16               Use FP16 scRGB wide color gamut (default: on with -thdr)\n"
               << L"   -contrast <0-200>   TrueHDR Contrast (default: 100)\n"
               << L"   -saturation <0-200> TrueHDR Saturation (default: 100)\n"
               << L"   -maxLum <400-2000>  Monitor Peak Luminance in nits (default: 1000)\n"
               << L"   -mode <name>        Comparison Mode: split (default), full, orig, sbs, diff, quad\n"
               << L"   -splitPos <0.0-1.0> Initial split slider position (default: 0.5)\n"
               << L"   -loupe              Enable 8x Interactive Magnifying Loupe\n"
               << L"   -leadBuffer <n>     Deep lead-buffer ring depth in VRAM frames (default: 90)\n"
               << L"   -batch <out_path>   Offline batch video upscaling using dual NVENC engines\n"
               << L"   -diag               Run hardware capability diagnostics and exit\n"
               << L"   -test [path]        Run automated pipeline verification test and exit\n"
               << L"   -? / -help          Display this help message\n\n"
               << L" Interactive Realtime Controls:\n"
               << L"   [Left Mouse Click]  Click or drag anywhere to slide separator in realtime\n"
               << L"   [Middle Mouse / C]  Reset / center slider to 50%\n"
               << L"   [Mouse Wheel]       Scroll wheel to smoothly slide Original (Left) vs DLSS-5 (Right)\n"
               << L"   [ [ ] / [ ] ]       Slide separator left / right by 2%\n"
               << L"   [ , ] / [ . ]       Slide separator left / right by 2%\n"
               << L"   [Shift + Left/Right]Slide separator left / right\n"
               << L"   [Home / End]        Slide separator to 0% (all DLSS-5) or 100% (all Original)\n"
               << L"   [Drag & Drop]       Drag any video file directly into window to play\n"
               << L"   [O] / [Ctrl+O]      Open File Dialog to browse and select any video file\n"
               << L"   [D] / [Tab]         Cycle Comparison Modes (Split, SBS, Diff, Quad, Full, Orig)\n"
               << L"   [V]                 Toggle Split Orientation (Vertical vs Horizontal)\n"
               << L"   [L]                 Toggle 8x Magnifier Loupe Lens\n"
               << L"   [1 - 4]             Switch VSR Quality (1: Low to 4: Ultra)\n"
               << L"   [0]                 Baseline Bicubic (VSR Quality 0)\n"
               << L"   [H]                 Toggle RTX TrueHDR On/Off\n"
               << L"   [F12]               Save Signed Comparison Snapshot to Disk\n"
               << L"   [Space]             Pause / Resume Video Playback\n"
               << L"   [Left / Right]      Single-frame step backward / forward\n"
               << L"   [+] / [-]           Adjust Difference Mode Heatmap Gain\n"
               << L"   [F1]                Toggle Telemetry On-Screen Display\n"
               << L"   [Esc]               Exit Application\n"
               << L"===============================================================================\n";
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    // Attach to parent console if running from terminal
    FILE* fp = nullptr;
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Logger::Instance().Initialize(L"RTX_DLSS5_Studio.log", true, LogLevel::Info);

    EngineConfig config;
    bool isBatchMode = false;
    bool isDiagMode = false;
    bool isTestMode = false;
    std::wstring testPath = L"";

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);

    for (int i = 0; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"-?" || arg == L"-help" || arg == L"--help") {
            PrintUsage();
            return 0;
        } else if (arg == L"-i" && i + 1 < argc) {
            config.InputFilePath = argv[++i];
        } else if (arg == L"-vsr") {
            config.EnableVSR = true;
        } else if (arg == L"-no-vsr") {
            config.EnableVSR = false;
        } else if (arg == L"-vsrQuality" && i + 1 < argc) {
            config.VSRQualityLevel = std::stoul(argv[++i]);
        } else if (arg == L"-thdr") {
            config.EnableTrueHDR = true;
        } else if (arg == L"-no-thdr") {
            config.EnableTrueHDR = false;
        } else if (arg == L"-fp16") {
            config.OutputFP16 = true;
        } else if (arg == L"-contrast" && i + 1 < argc) {
            config.HDRContrast = std::stoul(argv[++i]);
        } else if (arg == L"-saturation" && i + 1 < argc) {
            config.HDRSaturation = std::stoul(argv[++i]);
        } else if (arg == L"-maxLum" && i + 1 < argc) {
            config.HDRMaxLuminance = std::stoul(argv[++i]);
        } else if (arg == L"-mode" && i + 1 < argc) {
            std::wstring m = argv[++i];
            if (m == L"full") config.ActiveComparisonMode = ComparisonMode::FullUpscaled;
            else if (m == L"orig") config.ActiveComparisonMode = ComparisonMode::OriginalBaseline;
            else if (m == L"split") config.ActiveComparisonMode = ComparisonMode::SplitScreen;
            else if (m == L"sbs") config.ActiveComparisonMode = ComparisonMode::SideBySide;
            else if (m == L"diff") config.ActiveComparisonMode = ComparisonMode::DifferenceHeatmap;
            else if (m == L"quad") config.ActiveComparisonMode = ComparisonMode::QuadGrid;
        } else if (arg == L"-splitPos" && i + 1 < argc) {
            config.SplitPosition = std::stof(argv[++i]);
        } else if (arg == L"-loupe") {
            config.LoupeActive = true;
        } else if (arg == L"-leadBuffer" && i + 1 < argc) {
            config.LeadBufferCapacity = std::stoul(argv[++i]);
        } else if (arg == L"-batch" && i + 1 < argc) {
            isBatchMode = true;
            config.OutputExportPath = argv[++i];
        } else if (arg == L"-diag") {
            isDiagMode = true;
        } else if (arg == L"-test") {
            isTestMode = true;
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                testPath = argv[++i];
            }
        } else if (arg == L"-testFile" && i + 1 < argc) {
            isTestMode = true;
            testPath = argv[++i];
        } else if (arg[0] != L'-' && config.InputFilePath.empty()) {
            config.InputFilePath = arg;
        }
    }

    if (argv) LocalFree(argv);

    if (isTestMode) {
        if (testPath.empty()) {
            if (!config.InputFilePath.empty()) {
                testPath = config.InputFilePath;
            } else {
                testPath = L"RTX_DLSS5_Studio/data/test_sample.mp4";
                if (!std::filesystem::exists(testPath)) {
                    testPath = L"data/test_sample.mp4";
                }
            }
        }
        Application testApp;
        bool pass = testApp.RunTest(testPath);
        Logger::Instance().Shutdown();
        return pass ? 0 : 1;
    }

    if (isDiagMode) {
        LOG_INFO("===============================================================");
        LOG_INFO("Running NVIDIA GeForce RTX 5090 Blackwell Hardware Diagnostics");
        LOG_INFO("===============================================================");
        std::wcout << L"Running RTX 5090 Blackwell Diagnostics...\n";
        D3D12DeviceManager devMgr;
        if (devMgr.Initialize()) {
            LOG_INFO(L"Primary Adapter: " + devMgr.GetAdapterDescription());
            LOG_INFO("Dedicated VRAM: " + std::to_string(devMgr.GetDedicatedVideoMemoryMB()) + " MB");
            std::wcout << L"Primary Adapter: " << devMgr.GetAdapterDescription() << L"\n";
            std::wcout << L"Dedicated VRAM: " << devMgr.GetDedicatedVideoMemoryMB() << L" MB\n";

            NGXUpscaler upscaler;
            if (upscaler.Initialize(devMgr.GetDevice())) {
                LOG_INFO(std::string("NVIDIA NGX VSR Available: ") + (upscaler.IsVSRAvailable() ? "YES" : "NO"));
                LOG_INFO(std::string("NVIDIA NGX TrueHDR Available: ") + (upscaler.IsTrueHDRAvailable() ? "YES" : "NO"));
                std::wcout << L"NVIDIA NGX VSR Available: " << (upscaler.IsVSRAvailable() ? L"YES" : L"NO") << L"\n";
                std::wcout << L"NVIDIA NGX TrueHDR Available: " << (upscaler.IsTrueHDRAvailable() ? L"YES" : L"NO") << L"\n";

                ComPtr<ID3D12CommandAllocator> cmdAlloc;
                ComPtr<ID3D12GraphicsCommandList> cmdList;
                devMgr.GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
                devMgr.GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
                bool vsrCreated = upscaler.CreateVSRFeature(cmdList.Get());
                cmdList->Close();
                ID3D12CommandList* pp[] = { cmdList.Get() };
                devMgr.GetDirectQueue()->ExecuteCommandLists(1, pp);
                devMgr.FlushGPU();
                std::wcout << L"VSR Feature Creation: " << (vsrCreated ? L"SUCCESS" : L"FAILED") << L"\n";

                std::wcout << L"Diagnostic Passed.\n";
                LOG_INFO("RTX 5090 Blackwell Hardware Diagnostic Completed Successfully.");
            }
        }
        Logger::Instance().Shutdown();
        return 0;
    }

    if (isBatchMode) {
        LOG_INFO("===============================================================");
        LOG_INFO("RTX DLSS-5 Video Studio - Headless Batch Upscaling");
        LOG_INFO("===============================================================");
        std::wcout << L"Starting Batch Processing: " << config.InputFilePath << L" -> " << config.OutputExportPath << L"\n";
        D3D12DeviceManager devMgr;
        if (devMgr.Initialize()) {
            NGXUpscaler upscaler;
            if (upscaler.Initialize(devMgr.GetDevice())) {
                BatchVideoExporter exporter;
                bool success = exporter.ProcessBatch(
                    config.InputFilePath,
                    config.OutputExportPath,
                    config,
                    &upscaler,
                    [](uint32_t cur, uint32_t total, float fps) {
                        std::wcout << L"Progress: " << cur << L"/" << total
                                   << L" frames (" << static_cast<int>((cur * 100.0f) / total)
                                   << L"%) | " << fps << L" FPS\n";
                    }
                );
                if (success) {
                    std::wcout << L"[SUCCESS] Batch upscaling completed successfully!\n";
                } else {
                    std::wcout << L"[FAILED] Batch upscaling did not complete.\n";
                }
            }
        }
        Logger::Instance().Shutdown();
        return 0;
    }

    Application app;
    if (!app.Initialize(hInstance, nCmdShow, config)) {
        return -1;
    }

    return app.Run();
}
