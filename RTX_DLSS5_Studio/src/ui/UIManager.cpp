//----------------------------------------------------------------------------------
// File:        UIManager.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Event handling for mouse divider wipes, hotkeys, and loupe controls
//----------------------------------------------------------------------------------

#include "UIManager.h"
#include "../core/Logger.h"
#include <algorithm>

UIManager::UIManager() {}

UIManager::~UIManager() {}

bool UIManager::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, EngineConfig& config) {
    switch (message) {
        case WM_SETCURSOR: {
            if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                SetCursor(LoadCursor(nullptr, config.SplitIsVertical ? IDC_SIZEWE : IDC_SIZENS));
                return true;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            if (width > 0 && height > 0) {
                float videoAspect = (config.SourceHeight > 0) ? (static_cast<float>(config.SourceWidth) / static_cast<float>(config.SourceHeight)) :
                                    ((config.TargetHeight > 0) ? (static_cast<float>(config.TargetWidth) / static_cast<float>(config.TargetHeight)) : (16.0f / 9.0f));
                float vpAspect = static_cast<float>(width) / static_cast<float>(height);
                float renderW = static_cast<float>(width);
                float renderH = static_cast<float>(height);
                float offsetX = 0.0f;
                float offsetY = 0.0f;
                if (vpAspect > videoAspect) {
                    renderW = renderH * videoAspect;
                    offsetX = (static_cast<float>(width) - renderW) * 0.5f;
                } else if (vpAspect < videoAspect) {
                    renderH = renderW / videoAspect;
                    offsetY = (static_cast<float>(height) - renderH) * 0.5f;
                }

                float normX = (renderW > 0.0f) ? ((xPos - offsetX) / renderW) : 0.5f;
                float normY = (renderH > 0.0f) ? ((yPos - offsetY) / renderH) : 0.5f;

                if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                    m_isDraggingDivider = true;
                    config.SplitPosition = config.SplitIsVertical ? normX : normY;
                    config.SplitPosition = std::clamp(config.SplitPosition, 0.01f, 0.99f);
                    SetCapture(hWnd);
                }

                if (config.LoupeActive) {
                    config.LoupeCenterNormX = std::clamp(normX, 0.0f, 1.0f);
                    config.LoupeCenterNormY = std::clamp(normY, 0.0f, 1.0f);
                }
            }
            return true;
        }

        case WM_MBUTTONDOWN: {
            if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                config.SplitPosition = 0.50f; // Reset slider to center
                return true;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (m_isDraggingDivider) {
                m_isDraggingDivider = false;
                ReleaseCapture();
            }
            return true;
        }

        case WM_MOUSEMOVE: {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            if (width > 0 && height > 0) {
                float videoAspect = (config.SourceHeight > 0) ? (static_cast<float>(config.SourceWidth) / static_cast<float>(config.SourceHeight)) :
                                    ((config.TargetHeight > 0) ? (static_cast<float>(config.TargetWidth) / static_cast<float>(config.TargetHeight)) : (16.0f / 9.0f));
                float vpAspect = static_cast<float>(width) / static_cast<float>(height);
                float renderW = static_cast<float>(width);
                float renderH = static_cast<float>(height);
                float offsetX = 0.0f;
                float offsetY = 0.0f;
                if (vpAspect > videoAspect) {
                    renderW = renderH * videoAspect;
                    offsetX = (static_cast<float>(width) - renderW) * 0.5f;
                } else if (vpAspect < videoAspect) {
                    renderH = renderW / videoAspect;
                    offsetY = (static_cast<float>(height) - renderH) * 0.5f;
                }

                float normX = (renderW > 0.0f) ? ((xPos - offsetX) / renderW) : 0.5f;
                float normY = (renderH > 0.0f) ? ((yPos - offsetY) / renderH) : 0.5f;

                if (m_isDraggingDivider && config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                    config.SplitPosition = config.SplitIsVertical ? normX : normY;
                    config.SplitPosition = std::clamp(config.SplitPosition, 0.01f, 0.99f);
                }

                if (config.LoupeActive) {
                    config.LoupeCenterNormX = std::clamp(normX, 0.0f, 1.0f);
                    config.LoupeCenterNormY = std::clamp(normY, 0.0f, 1.0f);
                }
            }
            return true;
        }

        case WM_MOUSEWHEEL: {
            if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                float step = (delta > 0) ? -0.02f : 0.02f;
                config.SplitPosition = std::clamp(config.SplitPosition + step, 0.01f, 0.99f);
                return true;
            }
            break;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case 'C':
                case 'c':
                case 'R':
                case 'r': { // Center / Reset split slider to 50%
                    if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                        config.SplitPosition = 0.50f;
                        return true;
                    }
                    break;
                }

                case VK_HOME: { // Slide fully to left (0% -> all DLSS-5)
                    if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                        config.SplitPosition = 0.01f;
                        return true;
                    }
                    break;
                }

                case VK_END: { // Slide fully to right (100% -> all Original)
                    if (config.ActiveComparisonMode == ComparisonMode::SplitScreen) {
                        config.SplitPosition = 0.99f;
                        return true;
                    }
                    break;
                }
                case 'D': // Cycle Comparison Mode
                case VK_TAB: {
                    uint32_t nextMode = (static_cast<uint32_t>(config.ActiveComparisonMode) + 1) % 6;
                    config.ActiveComparisonMode = static_cast<ComparisonMode>(nextMode);
                    LOG_INFO("Switched Comparison Mode: " + std::to_string(nextMode));
                    return true;
                }

                case 'L': { // Toggle Loupe Magnifier
                    config.LoupeActive = !config.LoupeActive;
                    LOG_INFO("Loupe Magnifier: " + std::string(config.LoupeActive ? "ENABLED" : "DISABLED"));
                    return true;
                }

                case 'V': { // Toggle Split orientation (Vertical vs Horizontal)
                    config.SplitIsVertical = !config.SplitIsVertical;
                    return true;
                }

                case VK_LEFT: {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        config.SplitPosition = std::clamp(config.SplitPosition - 0.02f, 0.01f, 0.99f);
                        return true;
                    }
                    break;
                }

                case VK_RIGHT: {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        config.SplitPosition = std::clamp(config.SplitPosition + 0.02f, 0.01f, 0.99f);
                        return true;
                    }
                    break;
                }

                case VK_OEM_4: // '[' key
                case VK_OEM_COMMA: { // ',' key
                    config.SplitPosition = std::clamp(config.SplitPosition - 0.02f, 0.01f, 0.99f);
                    return true;
                }

                case VK_OEM_6: // ']' key
                case VK_OEM_PERIOD: { // '.' key
                    config.SplitPosition = std::clamp(config.SplitPosition + 0.02f, 0.01f, 0.99f);
                    return true;
                }

                case '1':
                case '2':
                case '3':
                case '4': { // VSR Quality Levels
                    config.VSRQualityLevel = static_cast<uint32_t>(wParam - '0');
                    LOG_INFO("VSR Quality set to: " + std::to_string(config.VSRQualityLevel));
                    return true;
                }

                case '0': {
                    config.VSRQualityLevel = 0; // Bicubic baseline
                    return true;
                }

                case 'H': { // Toggle TrueHDR
                    config.EnableTrueHDR = !config.EnableTrueHDR;
                    LOG_INFO("TrueHDR: " + std::string(config.EnableTrueHDR ? "ENABLED" : "DISABLED"));
                    return true;
                }

                case VK_OEM_PLUS:
                case VK_ADD: {
                    config.DifferenceGain += 1.0f;
                    return true;
                }

                case VK_OEM_MINUS:
                case VK_SUBTRACT: {
                    if (config.DifferenceGain > 1.0f) config.DifferenceGain -= 1.0f;
                    return true;
                }

                case VK_F1: { // Toggle Telemetry HUD
                    m_telemetryOverlay.ToggleVisible();
                    return true;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
    return false;
}
