# NVIDIA RTX DLSS-5 Video Studio

[![Release](https://img.shields.io/github/v/release/theunrealfusion/DLSS5-MediaStudio?color=76B900&label=Release&logo=nvidia)](https://github.com/theunrealfusion/DLSS5-MediaStudio/releases)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20x64-blue.svg?logo=windows)](https://github.com/theunrealfusion/DLSS5-MediaStudio)
[![Hardware](https://img.shields.io/badge/Hardware-RTX%205090%20Blackwell%20%7C%20RTX%20Series-76B900?logo=nvidia)](https://www.nvidia.com)
[![DirectX](https://img.shields.io/badge/DirectX-12%20Agility%20SDK-black.svg?logo=microsoft)](https://github.com/theunrealfusion/DLSS5-MediaStudio)
[![License](https://img.shields.io/badge/License-Proprietary%20%2F%20NVIDIA%20EULA-orange.svg)](https://github.com/theunrealfusion/DLSS5-MediaStudio)

A studio-grade desktop video playback, super-resolution upscaling (VSR), TrueHDR conversion, and side-by-side comparison application built directly on the **NVIDIA RTX Video SDK v1.1.0** and specifically optimized for the **NVIDIA GeForce RTX 5090 (Blackwell Architecture)** with 32GB GDDR7 VRAM.

---

## 🚀 Quick Download & Installation

You can download the latest production releases directly from the **[Releases Section](https://github.com/theunrealfusion/DLSS5-MediaStudio/releases)** of this repository:

| Download Asset | Type | Description |
| :--- | :--- | :--- |
| 🟢 **[`RTX_DLSS5_Studio.exe`](https://github.com/theunrealfusion/DLSS5-MediaStudio/releases/latest)** | **Standalone Single Executable** | **Zero-setup direct download.** Download and double-click to run. Automatically self-extracts and verifies NVIDIA neural runtime DLLs. |
| 📦 **[`RTX_DLSS5_Studio-v1.1.0-Windows-x64.zip`](https://github.com/theunrealfusion/DLSS5-MediaStudio/releases/latest)** | **Portable ZIP Package** | Complete pre-extracted bundle containing `RTX_DLSS5_Studio.exe`, `nvngx_vsr.dll`, `nvngx_truehdr.dll`, `Shaders.hlsl`, `run.bat`, and `README.txt`. |
| 🛡️ **[`SHA256SUMS.txt`](https://github.com/theunrealfusion/DLSS5-MediaStudio/releases/latest)** | **Integrity Checksums** | Official SHA-256 hashes to verify binary authenticity and digital integrity. |

> [!TIP]
> **No installation required!** Simply download `RTX_DLSS5_Studio.exe` to any folder and double-click it. The application self-contains all necessary runtime dependencies.

---

## 🌟 Key Capabilities & Architecture

### 1. RTX 5090 Blackwell Hardware Optimization
- **32GB GDDR7 Lead-Buffer Ring**: Caches 60 to 120 uncompressed 4K FP16 / HDR frames ahead of the playback playhead in VRAM (~3.8 GB footprint), delivering true zero-stutter playback at 120Hz / 144Hz+.
- **Dynamic Instant Fallback**: Instantaneous zero-drop GPU bicubic transition during rapid timeline scrubbing, seamlessly re-engaging neural upscaling as buffers refill.
- **Triple-Queue Asynchronous Pipeline**: Multi-queue Direct3D 12 engine separating Video Decode, NGX Tensor Inference, and Swapchain Presentation via `ID3D12Fence` synchronization.
- **Ultra VSR Quality 4 + FP16 scRGB TrueHDR**: Neural 4K super-resolution and wide color gamut HDR conversion evaluated at <4ms GPU evaluation latency.

### 2. Hardened Enterprise Security
- **Authenticode Cryptographic Validation**: Validates digital certificates of all runtime binaries using Win32 `WinVerifyTrust` to prevent DLL hijacking.
- **Zero-Trust Input Sanitization**: Path canonicalization (`GetFullPathNameW`), magic-byte header verification, and prevention of path traversal attacks.
- **TDR & Device Loss Resilience**: Handles `DXGI_ERROR_DEVICE_REMOVED` / `DXGI_ERROR_DEVICE_RESET` and re-instantiates pipeline resources seamlessly.

### 3. Studio Evaluation & Comparison Modes
1. **Full Upscaled**: Fullscreen neural DLSS-5 / VSR + TrueHDR output.
2. **Original Baseline**: Hardware bicubic SDR source for reference.
3. **Split-Screen Wipe**: Interactive draggable vertical/horizontal divider with sub-pixel rendering.
4. **Side-by-Side (SBS 50/50)**: Dual synchronized viewports.
5. **Difference Heatmap**: Computes $|I_{AI} - I_{Orig}| \times \text{Gain}$ with Turbo false-color mapping.
6. **2x2 Quad Grid**: Simultaneous 4-quadrant inspection (Source, VSR, TrueHDR, Combined).
7. **8x Interactive Loupe Lens**: Hoverable magnifying glass (2x, 4x, 8x) for inspecting fine textures and edges.

---

## 🎮 Controls & Shortcuts

| Shortcut | Action |
| :--- | :--- |
| **`Left Mouse Click / Drag`** | Slide split comparison separator in real time |
| **`Middle Mouse / C`** | Reset / center split separator to 50% |
| **`Mouse Wheel`** | Smoothly shift comparison wipe position |
| **`[` / `]` or `,` / `.`** | Step split slider left / right by 2% |
| **`Home` / `End`** | Snap slider to 0% (all DLSS) or 100% (all Original) |
| **`Drag & Drop`** | Drag any video file (.mp4, .mkv, .mov, etc.) into the window |
| **`O` / `Ctrl+O`** | Open File Dialog to select video |
| **`D` / `Tab`** | Cycle comparison mode (Split, Side-by-Side, Heatmap, Quad) |
| **`V`** | Toggle wipe orientation (Vertical vs Horizontal) |
| **`L`** | Toggle 8x interactive magnifying loupe |
| **`1` – `4`** | Set VSR Quality level (1=Low, 2=Med, 3=High, 4=Ultra) |
| **`0`** | Baseline Bicubic mode (bypass neural upscaler) |
| **`H`** | Toggle RTX TrueHDR On/Off |
| **`Space`** | Play / Pause playback |
| **`Left` / `Right`** | Single-frame step backward / forward |
| **`+` / `-`** | Increase / decrease heatmap gain |
| **`F1`** | Toggle real-time telemetry HUD overlay |
| **`F12`** | Capture signed comparison snapshot |
| **`Esc`** | Exit application |

---

## 🔒 Automated Release Workflow (Owner-Only Access)

This repository includes an automated GitHub Actions release workflow located at [`.github/workflows/release.yml`](.github/workflows/release.yml).

### Strict Authorization Guard
To guarantee release integrity and security, the workflow is strictly restricted so that **only the authorized repository owner (`@kshvsharma960`) can execute it**.
- Both job-level preconditions and runner-level security steps authenticate `${{ github.actor }}` against `@kshvsharma960` and `@${{ github.repository_owner }}`.
- Any unauthorized attempts to trigger or dispatch a release will be immediately rejected with an exit code error before any compilation or release packaging occurs.

### How to Trigger a Release (For @kshvsharma960)
1. Go to the **Actions** tab in GitHub.
2. Under **Workflows**, click **Release RTX DLSS-5 Studio**.
3. Click the **Run workflow** dropdown button on the right.
4. Fill in:
   - **Release Tag Name**: e.g., `v1.1.0`
   - **Release Title**: e.g., `RTX DLSS-5 Video Studio v1.1.0 - Production Release`
   - **Publish as Draft**: (leave unchecked for public release, or check for draft)
   - **Mark as Pre-release**: (optional)
5. Click **Run workflow**.

The workflow will automatically:
1. Compile the Release x64 binary with MSVC and Whole-Program Optimization (`/LTCG`).
2. Embed the Windows manifest, version metadata (`1.1.0.0`), application icon, and runtime assets into `RTX_DLSS5_Studio.exe`.
3. Package the standalone executable, portable ZIP bundle (`RTX_DLSS5_Studio-v1.1.0-Windows-x64.zip`), and launchers.
4. Calculate cryptographic SHA-256 hashes (`SHA256SUMS.txt`).
5. Publish the release to the **Releases** section with all downloadable assets attached.

---

## 💻 System Requirements

- **Operating System**: Windows 10 (21H2+) or Windows 11 (64-bit)
- **GPU**: NVIDIA GeForce RTX 20, 30, 40, or 50 Series (RTX 5090 Blackwell Architecture Recommended)
- **Driver**: NVIDIA Graphics Driver R550.50 or higher (Driver R610+ / 621.24 verified)
- **API**: Direct3D 12 Feature Level 12.0+

---

## 🛠️ Building from Source

```powershell
# Clone the repository
git clone https://github.com/theunrealfusion/DLSS5-MediaStudio.git
cd DLSS5-MediaStudio

# Build Release binary
.\RTX_DLSS5_Studio\scripts\build.ps1 -Configuration Release

# Package Release deliverables locally
.\RTX_DLSS5_Studio\scripts\package_release.ps1 -Version "v1.1.0"
```
