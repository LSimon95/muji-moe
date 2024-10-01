/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <ICubismModelSetting.hpp>
#include <Type/csmRectF.hpp>
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>

#include <string>


class CLive2DModel : public Csm::CubismUserModel
{
public:
    CLive2DModel();
    virtual ~CLive2DModel();

    void LoadAssets(const Csm::csmChar* dir, const  Csm::csmChar* fileName);
    void ReloadRenderer();
    void Update();
    void Draw(Csm::CubismMatrix44& matrix);

    Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority, Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler = NULL);
    Csm::CubismMotionQueueEntryHandle StartRandomMotion(const Csm::csmChar* group, Csm::csmInt32 priority, Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler = NULL);
    void SetExpression(const Csm::csmChar* expressionID);
    void SetRandomExpression();
    virtual void MotionEventFired(const Live2D::Cubism::Framework::csmString& eventValue);
    virtual Csm::csmBool HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y);
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2& GetRenderBuffer();

protected:
    void DoDraw();

private:
    void SetupModel(Csm::ICubismModelSetting* setting);
    void SetupTextures();
    void PreloadMotionGroup(const Csm::csmChar* group);
    void ReleaseMotionGroup(const Csm::csmChar* group) const;
    void ReleaseMotions();
    void ReleaseExpressions();

    Csm::ICubismModelSetting* m_modelSetting;
    Csm::csmString m_modelHomeDir;
    Csm::csmFloat32 m_userTimeSeconds;
    Csm::csmVector<Csm::CubismIdHandle> m_eyeBlinkIds;
    Csm::csmVector<Csm::CubismIdHandle> m_lipSyncIds;
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   m_motions;
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   m_expressions;

    const Csm::CubismId* m_idParamAngleX;
    const Csm::CubismId* m_idParamAngleY;
    const Csm::CubismId* m_idParamAngleZ;
    const Csm::CubismId* m_idParamBodyAngleX;
    const Csm::CubismId* m_idParamEyeBallX;
    const Csm::CubismId* m_idParamEyeBallY;

    // LAppWavFileHandler _wavFileHandler; ///< wavファイルハンドラ

    Csm::Rendering::CubismOffscreenSurface_OpenGLES2 m_renderBuffer;

    std::string m_lastChatEmotion;
};
