//----------------------------------------------------------------------------------
// File:        UIManager.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Mouse and keyboard interaction, divider drag handling, and UI state
//----------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include "../core/EngineConfig.h"
#include "TelemetryOverlay.h"

class UIManager {
public:
    UIManager();
    ~UIManager();

    bool HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, EngineConfig& config);

    TelemetryOverlay& GetTelemetryOverlay() { return m_telemetryOverlay; }

    bool IsDraggingDivider() const { return m_isDraggingDivider; }

private:
    TelemetryOverlay m_telemetryOverlay;
    bool m_isDraggingDivider = false;
    POINT m_lastMousePos = { 0, 0 };
};
