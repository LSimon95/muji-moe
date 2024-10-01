/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#pragma once

#include <CubismFramework.hpp>
#include <string>

class CPlat
{
public:
    static Csm::csmByte* LoadFileAsBytes(const std::string filePath, Csm::csmSizeInt* outSize);
    static void ReleaseBytes(Csm::csmByte* byteData);

    static Csm::csmFloat32 GetDeltaTime();

    static void UpdateTime();
    static void PrintLogLn(const Csm::csmChar* format, ...);
    static void PrintMessageLn(const Csm::csmChar* message);
    static std::string GetExecuteAbsolutePath();

    static void GetCursorPos(float* x, float* y);

private:
    static double s_currentFrame;
    static double s_lastFrame;
    static double s_deltaTime;
};

