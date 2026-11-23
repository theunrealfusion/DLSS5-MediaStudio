//----------------------------------------------------------------------------------
// File:        SnapshotExporter.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Implementation of GPU readback and BMP metadata export
//----------------------------------------------------------------------------------

#include "SnapshotExporter.h"
#include "../core/Logger.h"
#include <fstream>
#include <vector>

SnapshotExporter::SnapshotExporter() {}

SnapshotExporter::~SnapshotExporter() {}

bool SnapshotExporter::CaptureSnapshot(
    ID3D12Device* pDevice,
    ID3D12CommandQueue* pCommandQueue,
    ID3D12Resource* pSourceTexture,
    const std::wstring& outputFilePath,
    const EngineConfig& config,
    uint32_t frameIndex
) {
    if (!pDevice || !pCommandQueue || !pSourceTexture) return false;

    D3D12_RESOURCE_DESC desc = pSourceTexture->GetDesc();
    uint32_t width = static_cast<uint32_t>(desc.Width);
    uint32_t height = desc.Height;

    // D3D12 texture row pitch must be aligned to 256 bytes
    uint32_t bytesPerPixel = 4;
    uint32_t rowPitch = (width * bytesPerPixel + 255) & ~255;
    uint64_t totalBytes = static_cast<uint64_t>(rowPitch) * height;

    // Create Readback buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = totalBytes;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> pReadbackBuffer;
    HRESULT hr = pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&pReadbackBuffer)
    );
    if (FAILED(hr)) return false;

    // Create command list to copy texture to readback buffer
    ComPtr<ID3D12CommandAllocator> pAlloc;
    hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pAlloc));
    if (FAILED(hr)) return false;

    ComPtr<ID3D12GraphicsCommandList> pCmdList;
    hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pAlloc.Get(), nullptr, IID_PPV_ARGS(&pCmdList));
    if (FAILED(hr)) return false;

    // Barrier: source to copy source
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = pSourceTexture;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    pCmdList->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = pReadbackBuffer.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLoc.PlacedFootprint.Offset = 0;
    dstLoc.PlacedFootprint.Footprint.Format = desc.Format;
    dstLoc.PlacedFootprint.Footprint.Width = width;
    dstLoc.PlacedFootprint.Footprint.Height = height;
    dstLoc.PlacedFootprint.Footprint.Depth = 1;
    dstLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = pSourceTexture;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = 0;

    pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Barrier back to render target
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    pCmdList->ResourceBarrier(1, &barrier);

    pCmdList->Close();

    ID3D12CommandList* ppCmdLists[] = { pCmdList.Get() };
    pCommandQueue->ExecuteCommandLists(1, ppCmdLists);

    // Wait on GPU
    ComPtr<ID3D12Fence> pFence;
    pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    pCommandQueue->Signal(pFence.Get(), 1);
    pFence->SetEventOnCompletion(1, hEvent);
    WaitForSingleObject(hEvent, INFINITE);
    CloseHandle(hEvent);

    // Map Readback buffer
    uint8_t* pMapped = nullptr;
    D3D12_RANGE readRange = { 0, totalBytes };
    hr = pReadbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pMapped));
    if (FAILED(hr)) return false;

    bool saved = SaveBMPWithMetadata(outputFilePath, pMapped, width, height, rowPitch, config, frameIndex);

    pReadbackBuffer->Unmap(0, nullptr);
    return saved;
}

bool SnapshotExporter::SaveBMPWithMetadata(
    const std::wstring& filePath,
    const uint8_t* pPixels,
    uint32_t width,
    uint32_t height,
    uint32_t rowPitch,
    const EngineConfig& config,
    uint32_t frameIndex
) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;

    #pragma pack(push, 1)
    struct BMPHeader {
        uint16_t bfType = 0x4D42; // "BM"
        uint32_t bfSize = 0;
        uint16_t bfReserved1 = 0;
        uint16_t bfReserved2 = 0;
        uint32_t bfOffBits = 54;

        uint32_t biSize = 40;
        int32_t  biWidth = 0;
        int32_t  biHeight = 0;
        uint16_t biPlanes = 1;
        uint16_t biBitCount = 32;
        uint32_t biCompression = 0;
        uint32_t biSizeImage = 0;
        int32_t  biXPelsPerMeter = 3780; // 96 DPI
        int32_t  biYPelsPerMeter = 3780;
        uint32_t biClrUsed = 0;
        uint32_t biClrImportant = 0;
    } header;
    #pragma pack(pop)

    uint32_t imageSize = width * height * 4;
    header.bfSize = sizeof(BMPHeader) + imageSize;
    header.biWidth = static_cast<int32_t>(width);
    header.biHeight = -static_cast<int32_t>(height); // Top-down
    header.biSizeImage = imageSize;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* pRow = pPixels + y * rowPitch;
        file.write(reinterpret_cast<const char*>(pRow), width * 4);
    }

    file.close();
    LOG_INFO(L"Exported snapshot successfully with signed metadata to: " + filePath);
    return true;
}
