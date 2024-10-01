/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <os/log.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Model/CubismMoc.hpp>
#include "plat.hpp"
#include <mach-o/dyld.h>
#include <libgen.h>
#include <CoreGraphics/CGEvent.h>

using std::endl;
using namespace Csm;
using namespace std;

double CPlat::s_currentFrame = 0.0;
double CPlat::s_lastFrame = 0.0;
double CPlat::s_deltaTime = 0.0;

csmByte* CPlat::LoadFileAsBytes(const string filePath, csmSizeInt* outSize)
{
    //filePath;//
    const char* path = filePath.c_str();

    int size = 0;
    struct stat statBuf;
    if (stat(path, &statBuf) == 0)
    {
        size = statBuf.st_size;

        if (size == 0)
        {
            PrintLogLn("Stat succeeded but file size is zero. path:%s", path);
            return NULL;
        }
    }
    else
    {
        PrintLogLn("Stat failed. errno:%d path:%s", errno, path);
        return NULL;
    }

    std::fstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        PrintLogLn("File open failed. path:%s", path);
        return NULL;
    }

    char* buf = new char[size];
    file.read(buf, size);
    file.close();

    *outSize = size;
    return reinterpret_cast<csmByte*>(buf);
}

void CPlat::ReleaseBytes(csmByte* byteData)
{
    delete[] byteData;
}

csmFloat32  CPlat::GetDeltaTime()
{
    return static_cast<csmFloat32>(s_deltaTime);
}

void CPlat::UpdateTime()
{
    s_currentFrame = glfwGetTime();
    s_deltaTime = s_currentFrame - s_lastFrame;
    s_lastFrame = s_currentFrame;
}

void CPlat::PrintLogLn(const csmChar* format, ...)
{
    va_list args;
    csmChar buf[256];
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args); 
    os_log(OS_LOG_DEFAULT, "%s", buf);
    va_end(args);
}

void CPlat::PrintMessageLn(const csmChar* message)
{
    PrintLogLn("%s", message);
}

std::string CPlat::GetExecuteAbsolutePath()
{
    char path[1024];
    uint32_t size = sizeof(path);
    _NSGetExecutablePath(path, &size);
    return (std::string(dirname(path)) + "/");
}

void CPlat::GetCursorPos(float* x, float* y)
{
    CGPoint cursor;
    CGEventRef event = CGEventCreate(NULL);
    cursor = CGEventGetLocation(event);
    *x = cursor.x;
    *y = cursor.y;
    CFRelease(event);
}