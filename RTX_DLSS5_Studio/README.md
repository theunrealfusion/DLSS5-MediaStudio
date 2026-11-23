# RTX DLSS-5 Video Studio

A studio-grade video playback, upscaling, and comparison desktop application built directly on the **NVIDIA RTX Video SDK v1.1.0** and specifically optimized for the **NVIDIA GeForce RTX 5090 (Blackwell Architecture)**.

This application elevates the concept into a hardened product featuring **Authenticode cryptographic DLL verification**, **zero-trust input sanitization**, **zero-copy Direct3D 12 asynchronous compute queues**, **deep 32GB lead buffering**, **7 interactive comparison modes**, and **high-throughput offline batch upscaling**.

---

## Key Pillars & Architecture

### 1. RTX 5090 Blackwell Hardware Optimization
- **32GB GDDR7 Lead-Buffer Ring**: Caches 60 to 120 uncompressed 4K FP16 / HDR frames ahead of the playback playhead directly in VRAM (~3.8 GB footprint), providing true zero-stutter playback at 120Hz / 144Hz+.
- **Dynamic Instant Fallback**: If seeking rapidly across the timeline or during momentary buffer underruns, the engine transitions on the fly to GPU bicubic filtering with zero dropped frames, then smoothly re-engages neural upscaling as the buffer fills.
- **Triple-Queue Asynchronous Pipeline**: Multi-queue Direct3D 12 engine separating Video Decode, NGX Tensor Inference, and Presentation via `ID3D12Fence` synchronization.
- **Ultra VSR Quality 4 + FP16 scRGB TrueHDR**: Delivers full 4K neural super-resolution with wide color gamut HDR simultaneously at <4ms GPU evaluation latency.

### 2. Enterprise-Grade Security
- **Authenticode Cryptographic Validation**: Validates the digital certificate of `nvngx_vsr.dll` and `nvngx_truehdr.dll` using `WinVerifyTrust` to prevent DLL hijacking or untrusted binary injection.
- **Zero-Trust Input Sanitization**: Path canonicalization (`GetFullPathNameW`), magic-byte header verification, and prevention of path traversal (`../`) or command injection attacks.
- **TDR & Device Loss Resilience**: Detects `DXGI_ERROR_DEVICE_REMOVED` / `DXGI_ERROR_DEVICE_RESET` and automatically re-instantiates pipeline resources without crashing.
- **Air-Gapped Policy**: Enforces local-only processing mode for secure studio/production environments.

### 3. Studio Evaluation & Comparison Modes
1. **Full Upscaled**: Fullscreen DLSS / VSR + TrueHDR output.
2. **Original Baseline**: Hardware bicubic SDR source for baseline comparison.
3. **Split-Screen Wipe**: Draggable vertical/horizontal divider with sub-pixel line rendering.
4. **Side-by-Side (SBS 50/50)**: Dual side-by-side view with synchronized timelines.
5. **Difference Heatmap**: Computes $|I_{AI} - I_{Orig}| \times \text{Gain}$ with Turbo false-color mapping, isolating noise removal and edge reconstruction.
6. **2x2 Quad Grid**: Simultaneous 4-quadrant view (Source, VSR, TrueHDR, Combined).
7. **8x Interactive Loupe Lens**: Hoverable magnifying glass (2x, 4x, 8x) for inspecting fine textures, hair, and grain without moving the viewport.

---

## Controls & Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| **`Left Mouse Click / Drag`** | Realtime slider control (slide Original on left vs DLSS-5 on right) |
| **`Middle Mouse Click / C`** | Reset / center slider to 50% |
| **`Mouse Wheel`** | Smoothly slide comparison separator in Split mode |
| **`[` / `]` or `,` / `.`** | Step split slider left / right by 2% |
| **`Shift + Left / Right`** | Step split slider left / right |
| **`Home` / `End`** | Slide split separator to 0% (all DLSS-5) or 100% (all Original) |
| **`Drag & Drop`** | Drag any video file (.mp4, .mkv, .mov, .avi, etc.) into window to play |
| **`O` / `Ctrl+O`** | Open File Dialog to browse and select any video file |
| **`D` / `Tab`** | Cycle Comparison Mode (Split, SBS, Diff, Quad, Full, Orig) |
| **`L`** | Toggle 8x Interactive Magnifier Loupe Lens |
| **`V`** | Toggle Split Wipe Orientation (Vertical vs Horizontal) |
| **`1` – `4`** | Set VSR Quality Level (`1`: Low, `2`: Med, `3`: High, `4`: Ultra) |
| **`0`** | Baseline Bicubic Mode (Disable VSR) |
| **`H`** | Toggle RTX TrueHDR On/Off |
| **`Space`** | Play / Pause Video Playback |
| **`Left` / `Right`** | Single-Frame Step Backward / Forward |
| **`+` / `-`** | Increase / Decrease Difference Heatmap Gain |
| **`F1`** | Toggle Telemetry On-Screen Display (HUD) |
| **`F12`** | Capture Snapshot with Signed Metadata Watermark |
| **`Esc`** | Exit Application |

---

## Build & Run

### Prerequisites
- Windows 10/11 x64
- NVIDIA GeForce RTX Series GPU (RTX 5090 Blackwell verified)
- NVIDIA Graphics Driver R550.50+ or R610+ (Driver 621.24 verified)
- Visual Studio 2026 / 2022 Build Tools (with C++ Desktop workload)
- Windows 11 SDK (10.0.26100.0 or 10.0.22621.0)

### Building
Run the automated build script:
```powershell
.\scripts\build.ps1 -Configuration Release
```

### Packaging Production Release
Run the automated packaging script to build, package standalone `.exe`, portable `.zip`, and generate SHA-256 checksums:
```powershell
.\scripts\package_release.ps1 -Version "v1.1.0"
```
Deliverables are output to `dist/`:
- `RTX_DLSS5_Studio.exe` (Standalone zero-dependency executable)
- `RTX_DLSS5_Studio-v1.1.0-Windows-x64.zip` (Portable bundle with DLLs and shaders)
- `SHA256SUMS.txt` (Cryptographic verification checksums)
- `release_notes.md` (Automated GitHub release notes)

### Automated GitHub Release Workflow (Owner-Only)
A GitHub Actions workflow is provided at `.github/workflows/release.yml`.
This workflow is strictly guarded so that **only `@kshvsharma960` (or the repository owner)** can trigger and publish releases.

To run:
1. Navigate to GitHub -> **Actions** -> **Release RTX DLSS-5 Studio**.
2. Click **Run workflow**, specify the version tag (e.g. `v1.1.0`), and run.
3. The standalone `.exe`, portable `.zip`, and checksums will be automatically built, verified, and uploaded to GitHub Releases.

### Launching
```powershell
# Interactive player opening an MP4 video with realtime slider
.\scripts\run.ps1 -InputFile "data/test_sample.mp4" -Mode split

# Interactive player with Ultra VSR and TrueHDR
.\scripts\run.ps1 -InputFile "my_video.mp4" -VSRQuality 4 -TrueHDR -FP16 -Mode split

# Run automated pipeline verification test (Video decode -> DLSS-5 VSR -> Slider comparison)
.\scripts\run.ps1 -Test

# Run hardware capability diagnostic on RTX 5090
.\scripts\run.ps1 -Diag
```
