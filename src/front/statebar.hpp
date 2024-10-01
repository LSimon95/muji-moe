/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include "window.hpp"
#include "texManager.hpp"

#include <math/CubismVector2.hpp>
#include <Math/CubismMatrix44.hpp>

class CStateBar
{
public:
    CStateBar();
    ~CStateBar();

    void Initialize();
    void Render();

    float GetBarZoneHeight() { return m_barZoneHeight; }

private:
    struct Control
    {
        std::string name;
        CTexManager::TextureInfo *tex;
        Csm::CubismVector2 offset;
        bool pressed;
        bool opened;
    };
    void RenderPic(
        CStateBar::Control *control,
        int winWidth, int winHeight,
        Csm::CubismVector2 &offset,
        float mouseX, float mouseY,
        bool mousePressed);

    std::vector<CStateBar::Control> m_controls;

    int m_positionLocation;
    int m_uvLocation;
    int m_textureLocation;
    int m_colorLocation;

    float m_barZoneHeight;
    float m_barScale;

    bool m_lastMousePressed;

    // Move window function
    float m_initMouseX;
    float m_initMouseY;

    int m_initWinX;
    int m_initWinY;

    void clickCallback(Control *control);
    void pressedCallback(Control *control);

    void confirmCloseCallback();
};
