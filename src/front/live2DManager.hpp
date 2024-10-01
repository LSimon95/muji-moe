/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#pragma once
#include <CubismFramework.hpp>
#include <Math/CubismMatrix44.hpp>

class CLive2DManager
{
public:
    CLive2DManager();
    ~CLive2DManager();

    void LoadModel();
    void ReleaseModel();
    void Render();

    void SetViewMatrix(Csm::CubismMatrix44 *m)
    {
        for (int i = 0; i < 16; i++)
        {
            m_viewMatrix->GetArray()[i] = m->GetArray()[i];
        }
    }

private:
    class CLive2DModel *m_model;
    class Csm::CubismMatrix44 *m_viewMatrix;
};