/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include "live2DModel.hpp"

#include <fstream>
#include <vector>
#include <iostream>
#include <CubismModelSettingJson.hpp>
#include <Motion/CubismMotion.hpp>
#include <Physics/CubismPhysics.hpp>
#include <CubismDefaultParameterId.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <Utils/CubismString.hpp>
#include <Id/CubismIdManager.hpp>
#include <Motion/CubismMotionQueueEntry.hpp>
#include <sys/stat.h>

#include "texManager.hpp"
#include "window.hpp"
#include "../plat.hpp"
#include "../server/world.hpp"
#include "../server/chat.hpp"

using namespace Live2D::Cubism::Framework;

namespace
{
    csmByte *CreateBuffer(const csmChar *path, csmSizeInt *size)
    {
        CPlat::PrintLogLn("create buffer: %s", path);
        return CPlat::LoadFileAsBytes(path, size);
    }

    void DeleteBuffer(csmByte *buffer, const csmChar *path = "")
    {
        CPlat::PrintLogLn("delete buffer: %s", path);
        CPlat::ReleaseBytes(buffer);
    }
}

CLive2DModel::CLive2DModel()
    : CubismUserModel(), m_modelSetting(NULL), m_userTimeSeconds(0.0f)
{
    using namespace Live2D::Cubism::Framework::DefaultParameterId;
    m_idParamAngleX = CubismFramework::GetIdManager()->GetId(ParamAngleX);
    m_idParamAngleY = CubismFramework::GetIdManager()->GetId(ParamAngleY);
    m_idParamAngleZ = CubismFramework::GetIdManager()->GetId(ParamAngleZ);
    m_idParamBodyAngleX = CubismFramework::GetIdManager()->GetId(ParamBodyAngleX);
    m_idParamEyeBallX = CubismFramework::GetIdManager()->GetId(ParamEyeBallX);
    m_idParamEyeBallY = CubismFramework::GetIdManager()->GetId(ParamEyeBallY);
}

CLive2DModel::~CLive2DModel()
{
    m_renderBuffer.DestroyOffscreenSurface();

    ReleaseMotions();
    ReleaseExpressions();

    for (csmInt32 i = 0; i < m_modelSetting->GetMotionGroupCount(); i++)
    {
        const csmChar *group = m_modelSetting->GetMotionGroupName(i);
        ReleaseMotionGroup(group);
    }
    delete (m_modelSetting);
}

void CLive2DModel::LoadAssets(const csmChar *dir, const csmChar *fileName)
{
    m_modelHomeDir = dir;
    CPlat::PrintLogLn("load model setting: %s", fileName);

    csmSizeInt size;
    const csmString path = csmString(dir) + fileName;

    csmByte *buffer = CreateBuffer(path.GetRawString(), &size);
    ICubismModelSetting *setting = new CubismModelSettingJson(buffer, size);
    DeleteBuffer(buffer, path.GetRawString());

    SetupModel(setting);

    if (_model == NULL)
    {
        CPlat::PrintLogLn("Failed to LoadAssets().");
        return;
    }

    CreateRenderer();
    SetupTextures();
}

void CLive2DModel::SetupModel(ICubismModelSetting *setting)
{
    _updating = true;
    _initialized = false;

    m_modelSetting = setting;

    csmByte *buffer;
    csmSizeInt size;

    // Cubism Model
    if (strcmp(m_modelSetting->GetModelFileName(), "") != 0)
    {
        csmString path = m_modelSetting->GetModelFileName();
        path = m_modelHomeDir + path;

        CPlat::PrintLogLn("create model: %s", setting->GetModelFileName());

        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadModel(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // Expression
    if (m_modelSetting->GetExpressionCount() > 0)
    {
        const csmInt32 count = m_modelSetting->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++)
        {
            csmString name = m_modelSetting->GetExpressionName(i);
            csmString path = m_modelSetting->GetExpressionFileName(i);
            path = m_modelHomeDir + path;

            buffer = CreateBuffer(path.GetRawString(), &size);
            ACubismMotion *motion = LoadExpression(buffer, size, name.GetRawString());

            if (motion)
            {
                if (m_expressions[name] != NULL)
                {
                    ACubismMotion::Delete(m_expressions[name]);
                    m_expressions[name] = NULL;
                }
                m_expressions[name] = motion;
            }

            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    // Physics
    if (strcmp(m_modelSetting->GetPhysicsFileName(), "") != 0)
    {
        csmString path = m_modelSetting->GetPhysicsFileName();
        path = m_modelHomeDir + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadPhysics(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // Pose
    if (strcmp(m_modelSetting->GetPoseFileName(), "") != 0)
    {
        csmString path = m_modelSetting->GetPoseFileName();
        path = m_modelHomeDir + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadPose(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // EyeBlink
    if (m_modelSetting->GetEyeBlinkParameterCount() > 0)
    {
        _eyeBlink = CubismEyeBlink::Create(m_modelSetting);
    }

    // Breath
    {
        _breath = CubismBreath::Create();

        csmVector<CubismBreath::BreathParameterData> breathParameters;

        breathParameters.PushBack(CubismBreath::BreathParameterData(m_idParamAngleX, 0.0f, 15.0f, 6.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(m_idParamAngleY, 0.0f, 8.0f, 3.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(m_idParamAngleZ, 0.0f, 10.0f, 5.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(m_idParamBodyAngleX, 0.0f, 4.0f, 15.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamBreath), 0.5f, 0.5f, 3.2345f, 0.5f));

        _breath->SetParameters(breathParameters);
    }

    // UserData
    if (strcmp(m_modelSetting->GetUserDataFile(), "") != 0)
    {
        csmString path = m_modelSetting->GetUserDataFile();
        path = m_modelHomeDir + path;
        buffer = CreateBuffer(path.GetRawString(), &size);
        LoadUserData(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    // EyeBlinkIds
    {
        csmInt32 eyeBlinkIdCount = m_modelSetting->GetEyeBlinkParameterCount();
        for (csmInt32 i = 0; i < eyeBlinkIdCount; ++i)
        {
            m_eyeBlinkIds.PushBack(m_modelSetting->GetEyeBlinkParameterId(i));
        }
    }

    // LipSyncIds
    {
        csmInt32 lipSyncIdCount = m_modelSetting->GetLipSyncParameterCount();
        for (csmInt32 i = 0; i < lipSyncIdCount; ++i)
        {
            m_lipSyncIds.PushBack(m_modelSetting->GetLipSyncParameterId(i));
        }
    }

    if (m_modelSetting == NULL || _modelMatrix == NULL)
    {
        CPlat::PrintLogLn("Failed to SetupModel().");
        return;
    }

    // Layout
    csmMap<csmString, csmFloat32> layout;
    m_modelSetting->GetLayoutMap(layout);
    _modelMatrix->SetupFromLayout(layout);

    _model->SaveParameters();

    for (csmInt32 i = 0; i < m_modelSetting->GetMotionGroupCount(); i++)
    {
        const csmChar *group = m_modelSetting->GetMotionGroupName(i);
        PreloadMotionGroup(group);
    }

    _motionManager->StopAllMotions();

    _updating = false;
    _initialized = true;
}

void CLive2DModel::PreloadMotionGroup(const csmChar *group)
{
    const csmInt32 count = m_modelSetting->GetMotionCount(group);

    for (csmInt32 i = 0; i < count; i++)
    {
        // ex) idle_0
        csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
        csmString path = m_modelSetting->GetMotionFileName(group, i);
        path = m_modelHomeDir + path;

        CPlat::PrintLogLn("load motion: %s => [%s_%d]", path.GetRawString(), group, i);

        csmByte *buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        CubismMotion *tmpMotion = static_cast<CubismMotion *>(LoadMotion(buffer, size, name.GetRawString()));

        if (tmpMotion)
        {
            csmFloat32 fadeTime = m_modelSetting->GetMotionFadeInTimeValue(group, i);
            if (fadeTime >= 0.0f)
            {
                tmpMotion->SetFadeInTime(fadeTime);
            }

            fadeTime = m_modelSetting->GetMotionFadeOutTimeValue(group, i);
            if (fadeTime >= 0.0f)
            {
                tmpMotion->SetFadeOutTime(fadeTime);
            }
            tmpMotion->SetEffectIds(m_eyeBlinkIds, m_lipSyncIds);

            if (m_motions[name] != NULL)
            {
                ACubismMotion::Delete(m_motions[name]);
            }
            m_motions[name] = tmpMotion;
        }

        DeleteBuffer(buffer, path.GetRawString());
    }
}

void CLive2DModel::ReleaseMotionGroup(const csmChar *group) const
{
    const csmInt32 count = m_modelSetting->GetMotionCount(group);
    for (csmInt32 i = 0; i < count; i++)
    {
        csmString voice = m_modelSetting->GetMotionSoundFileName(group, i);
        if (strcmp(voice.GetRawString(), "") != 0)
        {
            csmString path = voice;
            path = m_modelHomeDir + path;
        }
    }
}

void CLive2DModel::ReleaseMotions()
{
    for (csmMap<csmString, ACubismMotion *>::const_iterator iter = m_motions.Begin(); iter != m_motions.End(); ++iter)
    {
        ACubismMotion::Delete(iter->Second);
    }

    m_motions.Clear();
}

void CLive2DModel::ReleaseExpressions()
{
    for (csmMap<csmString, ACubismMotion *>::const_iterator iter = m_expressions.Begin(); iter != m_expressions.End(); ++iter)
    {
        ACubismMotion::Delete(iter->Second);
    }

    m_expressions.Clear();
}

void CLive2DModel::Update()
{
    const csmFloat32 deltaTimeSeconds = CPlat::GetDeltaTime();
    m_userTimeSeconds += deltaTimeSeconds;

    _dragManager->Update(deltaTimeSeconds);
    _dragX = _dragManager->GetX();
    _dragY = _dragManager->GetY();

    csmBool motionUpdated = false;

    _model->LoadParameters();
    auto world = CWorld::GetInstance();
    std::string chatEmotion = world->GetChatEmotion();
    if (m_lastChatEmotion != chatEmotion) // _motionManager->IsFinished()
    {
        if (world->GetChatServerIsRunning()){
            
            for (int i = 0; i < m_modelSetting->GetExpressionCount(); i++)
            {
                std::string name = std::string(m_modelSetting->GetExpressionName(i));
                if (chatEmotion.find(name) != std::string::npos) {
                    SetExpression(name.c_str());
                    break;
                }
            }

            for (int i = 0; i < m_modelSetting->GetMotionGroupCount(); i++)
            {
                std::string name = std::string(m_modelSetting->GetMotionGroupName(i));
                if (chatEmotion.find(name) != std::string::npos) {
                    StartRandomMotion(name.c_str(), 1);
                    break;
                }
            }
        }
        m_lastChatEmotion = chatEmotion;
    }
    else
    {
        motionUpdated = _motionManager->UpdateMotion(_model, deltaTimeSeconds);
    }
    _model->SaveParameters();

    _opacity = _model->GetModelOpacity();

    if (!motionUpdated)
    {
        if (_eyeBlink != NULL)
        {
            _eyeBlink->UpdateParameters(_model, deltaTimeSeconds);
        }
    }

    if (_expressionManager != NULL)
    {
        _expressionManager->UpdateMotion(_model, deltaTimeSeconds);
    }

    _model->AddParameterValue(m_idParamAngleX, _dragX * 30);
    _model->AddParameterValue(m_idParamAngleY, _dragY * 30);
    _model->AddParameterValue(m_idParamAngleZ, _dragX * _dragY * -30);

    _model->AddParameterValue(m_idParamBodyAngleX, _dragX * 10);

    _model->AddParameterValue(m_idParamEyeBallX, _dragX);
    _model->AddParameterValue(m_idParamEyeBallY, _dragY);

    float lipEnergy = (float)(world->GetChat()->m_lipEnergyShow) / 16384.0f / 2.0f;
    // std::cout << "lipEnergy: " << lipEnergy << std::endl;
    for (csmUint32 i = 0; i < m_lipSyncIds.GetSize(); ++i)
    {
        _model->AddParameterValue(m_lipSyncIds[i], lipEnergy, 0.8f);
    }

    if (_breath != NULL)
    {
        _breath->UpdateParameters(_model, deltaTimeSeconds);
    }

    if (_physics != NULL)
    {
        _physics->Evaluate(_model, deltaTimeSeconds);
    }

    if (_lipSync)
    {
        // csmFloat32 value = 0.0f;
        // _wavFileHandler.Update(deltaTimeSeconds);
        // value = _wavFileHandler.GetRms();

        // for (csmUint32 i = 0; i < m_lipSyncIds.GetSize(); ++i)
        // {
        //     _model->AddParameterValue(m_lipSyncIds[i], value, 0.8f);
        // }
    }

    if (_pose != NULL)
    {
        _pose->UpdateParameters(_model, deltaTimeSeconds);
    }

    _model->Update();
}

CubismMotionQueueEntryHandle CLive2DModel::StartMotion(const csmChar *group, csmInt32 no, csmInt32 priority, ACubismMotion::FinishedMotionCallback onFinishedMotionHandler)
{
    if (priority == 3)
    {
        _motionManager->SetReservePriority(priority);
    }
    else if (!_motionManager->ReserveMotion(priority))
    {
        CPlat::PrintLogLn("can't start motion.");
        return InvalidMotionQueueEntryHandleValue;
    }

    const csmString fileName = m_modelSetting->GetMotionFileName(group, no);

    // ex) idle_0
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
    CubismMotion *motion = static_cast<CubismMotion *>(m_motions[name.GetRawString()]);
    csmBool autoDelete = false;

    if (motion == NULL)
    {
        csmString path = fileName;
        path = m_modelHomeDir + path;

        csmByte *buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        motion = static_cast<CubismMotion *>(LoadMotion(buffer, size, NULL, onFinishedMotionHandler));

        if (motion)
        {
            csmFloat32 fadeTime = m_modelSetting->GetMotionFadeInTimeValue(group, no);
            if (fadeTime >= 0.0f)
            {
                motion->SetFadeInTime(fadeTime);
            }

            fadeTime = m_modelSetting->GetMotionFadeOutTimeValue(group, no);
            if (fadeTime >= 0.0f)
            {
                motion->SetFadeOutTime(fadeTime);
            }
            motion->SetEffectIds(m_eyeBlinkIds, m_lipSyncIds);
            autoDelete = true;
        }

        DeleteBuffer(buffer, path.GetRawString());
    }
    else
    {
        motion->SetFinishedMotionHandler(onFinishedMotionHandler);
    }

    // voice
    // csmString voice = m_modelSetting->GetMotionSoundFileName(group, no);
    // if (strcmp(voice.GetRawString(), "") != 0)
    // {
    //     csmString path = voice;
    //     path = m_modelHomeDir + path;
    //     _wavFileHandler.Start(path);
    // }

    CPlat::PrintLogLn("start motion: [%s_%d]", group, no);
    return _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

CubismMotionQueueEntryHandle CLive2DModel::StartRandomMotion(const csmChar *group, csmInt32 priority, ACubismMotion::FinishedMotionCallback onFinishedMotionHandler)
{
    if (m_modelSetting->GetMotionCount(group) == 0)
    {
        return InvalidMotionQueueEntryHandleValue;
    }

    csmInt32 no = rand() % m_modelSetting->GetMotionCount(group);

    return StartMotion(group, no, priority, onFinishedMotionHandler);
}

void CLive2DModel::DoDraw()
{
    if (_model == NULL)
    {
        return;
    }

    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->DrawModel();
}

void CLive2DModel::Draw(CubismMatrix44 &matrix)
{
    if (_model == NULL)
    {
        return;
    }

    matrix.MultiplyByMatrix(_modelMatrix);

    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->SetMvpMatrix(&matrix);

    DoDraw();
}

csmBool CLive2DModel::HitTest(const csmChar *hitAreaName, csmFloat32 x, csmFloat32 y)
{
    // 透明時は当たり判定なし。
    if (_opacity < 1)
    {
        return false;
    }
    const csmInt32 count = m_modelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++)
    {
        if (strcmp(m_modelSetting->GetHitAreaName(i), hitAreaName) == 0)
        {
            const CubismIdHandle drawID = m_modelSetting->GetHitAreaId(i);
            return IsHit(drawID, x, y);
        }
    }
    return false; // 存在しない場合はfalse
}

void CLive2DModel::SetExpression(const csmChar *expressionID)
{
    ACubismMotion *motion = m_expressions[expressionID];
    CPlat::PrintLogLn("expression: %s", expressionID);

    if (motion != NULL) _expressionManager->StartMotionPriority(motion, false, 3);
    else CPlat::PrintLogLn("expression[%s] is null ", expressionID);
}

void CLive2DModel::SetRandomExpression()
{
    if (m_expressions.GetSize() == 0)
        return;

    csmInt32 no = rand() % m_expressions.GetSize();
    csmMap<csmString, ACubismMotion *>::const_iterator map_ite;
    csmInt32 i = 0;
    for (map_ite = m_expressions.Begin(); map_ite != m_expressions.End(); map_ite++)
    {
        if (i == no)
        {
            csmString name = (*map_ite).First;
            SetExpression(name.GetRawString());
            return;
        }
        i++;
    }
}

void CLive2DModel::ReloadRenderer()
{
    DeleteRenderer();

    CreateRenderer();

    SetupTextures();
}

void CLive2DModel::SetupTextures()
{
    for (csmInt32 modelTextureNumber = 0; modelTextureNumber < m_modelSetting->GetTextureCount(); modelTextureNumber++)
    {
        if (strcmp(m_modelSetting->GetTextureFileName(modelTextureNumber), "") == 0)
            continue;

        csmString texturePath = m_modelSetting->GetTextureFileName(modelTextureNumber);
        texturePath = m_modelHomeDir + texturePath;

        CTexManager::TextureInfo *texture = CWindow::GetInstance()->GetTexManager()->CreateTextureFromPngFile(texturePath.GetRawString());
        const csmInt32 glTextueNumber = texture->id;

        GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->BindTexture(modelTextureNumber, glTextueNumber);
    }

#ifdef PREMULTIPLIED_ALPHA_ENABLE
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(true);
#else
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(false);
#endif
}

void CLive2DModel::MotionEventFired(const csmString &eventValue)
{
    CubismLogInfo("%s is fired on CLive2DModel!!", eventValue.GetRawString());
}

Csm::Rendering::CubismOffscreenSurface_OpenGLES2 &CLive2DModel::GetRenderBuffer()
{
    return m_renderBuffer;
}
