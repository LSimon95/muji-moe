/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include "window.hpp"
#include "live2DModel.hpp"
#include "live2DManager.hpp"
#include "../plat.hpp"
#include "cubismView.hpp"
#include "statebar.hpp"

#include <GLFW/glfw3.h>
#include <iostream>


CLive2DManager::CLive2DManager()
{
    m_model = nullptr;

    m_viewMatrix = new Csm::CubismMatrix44();
}

CLive2DManager::~CLive2DManager()
{
    delete m_viewMatrix;
    ReleaseModel();
}

void CLive2DManager::LoadModel()
{
    m_model = new CLive2DModel();
    m_model->LoadAssets("Resources/muji_moe_auto/", "MujiMoe_2.model3.json");
}

void CLive2DManager::ReleaseModel()
{
    if (m_model)
    {
        delete m_model;
        m_model = nullptr;
    }
}

void CLive2DManager::Render()
{
    if (m_model == nullptr) return;

    auto world = CWorld::GetInstance();
    auto window = world->GetWindow();

    int width, height;
    glfwGetWindowSize(window->GetGLWindow(), &width, &height);
    int barZoneHeight = window->GetStateBar()->GetBarZoneHeight();
    float heightScale = static_cast<float>(height - barZoneHeight) / static_cast<float>(height);
    float heightTranslate = static_cast<float>(barZoneHeight) / static_cast<float>(height);
    height = height - barZoneHeight;
    if (width == 0 || height <= 0) return;

    Csm::CubismMatrix44 projection;
    if (m_model->GetModel() == NULL)
    {
        CPlat::PrintLogLn("Failed to model->GetModel().");
        return;
    }

    auto matrix = m_model->GetModelMatrix();
    if (m_model->GetModel()->GetCanvasWidth() > 1.0f && width < height)
    {
        matrix->SetWidth(2.0f);
        projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height) * heightScale);
    }
    else projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f * heightScale);
    projection.TranslateX(world->m_showPanel ? -0.5f : 0.0f);
    projection.TranslateY(heightTranslate);


    if (m_viewMatrix != NULL) projection.MultiplyByMatrix(m_viewMatrix);

    window->GetView()->PreModelDraw(m_model);

    // Draging
    int wx, wy;
    glfwGetWindowPos(window->GetGLWindow(), &wx, &wy);
    float mouseX, mouseY;
    CPlat::GetCursorPos(&mouseX, &mouseY);
    mouseX = (mouseX - wx);
    mouseY = -(mouseY - wy);
    float headx, heady;
    headx = world->m_showPanel ? (width / 4) : (width / 2);
    heady = -height / 6;
    m_model->SetDragging((mouseX - headx) / height, (mouseY - heady) / height);

    m_model->Update();
    m_model->Draw(projection);

    window->GetView()->PostModelDraw(m_model);
}

