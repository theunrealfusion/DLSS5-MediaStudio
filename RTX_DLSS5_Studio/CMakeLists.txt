cmake_minimum_required(VERSION 3.20)
project(RTX_DLSS5_Studio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NV_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${NV_SDK_ROOT}/include
)

link_directories(
    ${NV_SDK_ROOT}/lib/Windows/x64
)

set(SOURCES
    src/core/Application.cpp
    src/core/Logger.cpp
    src/security/SecurityManager.cpp
    src/ngx/NGXUpscaler.cpp
    src/ngx/NGXTelemetry.cpp
    src/render/D3D12DeviceManager.cpp
    src/render/D3D12SwapChain.cpp
    src/render/ComparisonRenderer.cpp
    src/playback/RawVideoReader.cpp
    src/playback/MediaDecoder.cpp
    src/playback/LeadBufferEngine.cpp
    src/ui/UIManager.cpp
    src/ui/TelemetryOverlay.cpp
    src/export/SnapshotExporter.cpp
    src/export/BatchVideoExporter.cpp
    src/main.cpp
    RTX_DLSS5_Studio.rc
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    nvsdk_ngx_s
    d3d12
    dxgi
    d3dcompiler
    mfplat
    mfreadwrite
    mfuuid
    wintrust
    crypt32
)

# Copy NVIDIA runtime DLLs to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${NV_SDK_ROOT}/bin/Windows/x64/rel/nvngx_vsr.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${NV_SDK_ROOT}/bin/Windows/x64/rel/nvngx_truehdr.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/src/render/Shaders.hlsl"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
