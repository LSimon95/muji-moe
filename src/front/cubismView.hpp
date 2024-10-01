/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#pragma once

#include <Math/CubismMatrix44.hpp>
#include <Math/CubismViewMatrix.hpp>
#include "CubismFramework.hpp"
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>

class CCubismView
{
public:
    enum SelectTarget
    {
        SelectTarget_None,
        SelectTarget_ModelFrameBuffer,
        SelectTarget_ViewFrameBuffer,
    };

    CCubismView();
    ~CCubismView();

    void Initialize();
    void Render();

    void PreModelDraw(class CLive2DModel* refModel);
    void PostModelDraw(class CLive2DModel* refModel);

    void SwitchRenderingTarget(SelectTarget targetType) { m_renderTarget = targetType; }

private:
    Csm::CubismMatrix44* m_device2Screen;
    Csm::CubismViewMatrix* m_viewMatrix;

    SelectTarget m_renderTarget;
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2 m_renderBuffer;

    float m_clearColor[4];

    class CLive2DManager* m_live2DManager;
};