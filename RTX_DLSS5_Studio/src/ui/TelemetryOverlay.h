//----------------------------------------------------------------------------------
// File:        TelemetryOverlay.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Direct3D 12 On-Screen Display HUD & Telemetry Metrics
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <cstdint>
#include "../ngx/NGXTelemetry.h"
#include "../core/EngineConfig.h"

class TelemetryOverlay {
public:
    TelemetryOverlay();
    ~TelemetryOverlay();

    std::wstring FormatHUDString(const EngineConfig& config, uint32_t leadBufferFrames, uint32_t leadCapacity);

    void ToggleVisible() { m_visible = !m_visible; }
    bool IsVisible() const { return m_visible; }

private:
    bool m_visible = true;
};
