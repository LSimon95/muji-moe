/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "cubismView.hpp"
#include "window.hpp"
#include "live2DModel.hpp"
#include "live2DManager.hpp"


CCubismView::CCubismView()
{
    m_device2Screen = new Csm::CubismMatrix44();
    m_viewMatrix = new Csm::CubismViewMatrix();

    m_clearColor[0] = 1.0f;
    m_clearColor[1] = 1.0f;
    m_clearColor[2] = 1.0f;
    m_clearColor[3] = 0.0f;

    m_live2DManager = new CLive2DManager();
}

CCubismView::~CCubismView()
{
    delete m_live2DManager;

    m_renderBuffer.DestroyOffscreenSurface();
    delete m_device2Screen;
    delete m_viewMatrix;
}

void CCubismView::Initialize()
{
    int width, height;
    glfwGetWindowSize(CWindow::GetInstance()->GetGLWindow(), &width, &height);

    if (width == 0 || height <= 0) return;

    float ratio = static_cast<float>(width) / static_cast<float>(height);
    float left = -ratio;
    float right = ratio;
    float bottom = -1.0f;
    float top = 1.0f;
    
    m_viewMatrix->SetScreenRect(left, right, bottom, top);
    m_viewMatrix->Scale(1.0f, 1.0f);

    m_device2Screen->LoadIdentity();
    if (width > height)
    {
        float screenW = fabsf(right - left);
        m_device2Screen->ScaleRelative(screenW / width, -screenW / width);
    }
    else
    {
        float screenH = fabsf(top - bottom);
        m_device2Screen->ScaleRelative(screenH / height, -screenH / height);
    }

    m_device2Screen->Translate(-width * 0.5f, -height * 0.5f);

    m_viewMatrix->SetMaxScale(2.0f);
    m_viewMatrix->SetMinScale(0.8f);

    m_viewMatrix->SetMaxScreenRect(
        -2.0f,
        2.0f,
        -2.0f,
        2.0f
    );

    m_live2DManager->LoadModel();
}

void CCubismView::Render()
{
    m_live2DManager->SetViewMatrix(m_viewMatrix);
    m_live2DManager->Render();
}

void CCubismView::PreModelDraw(CLive2DModel* refModel)
{
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2* useTarget = NULL;
    if (m_renderTarget != SelectTarget_None)
    {
        useTarget = (m_renderTarget == SelectTarget_ViewFrameBuffer) ? &m_renderBuffer : &(refModel->GetRenderBuffer());

        if (!useTarget->IsValid())
        {
            int bufWidth, bufHeight;
            glfwGetFramebufferSize(CWindow::GetInstance()->GetGLWindow(), &bufWidth, &bufHeight);

            if(bufWidth!=0 && bufHeight!=0)
            {
                useTarget->CreateOffscreenSurface(static_cast<Csm::csmUint32>(bufWidth), static_cast<Csm::csmUint32>(bufHeight));
            }
        }

        useTarget->BeginDraw();
        useTarget->Clear(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
    }
}

void CCubismView::PostModelDraw(CLive2DModel* refModel)
{
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2* useTarget = NULL;

    if (m_renderTarget != SelectTarget_None)
    {
        useTarget = (m_renderTarget == SelectTarget_ViewFrameBuffer) ? &m_renderBuffer : &(refModel->GetRenderBuffer());
        useTarget->EndDraw();
    }
}