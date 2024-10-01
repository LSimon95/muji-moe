/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <CubismFramework.hpp>
#include <string>
#include "../server/world.hpp"

#include <functional>

#define _STRINGIZE(...) #__VA_ARGS__
#define STRINGIZE(...) _STRINGIZE(__VA_ARGS__)

class CWindow
{
public:
    CWindow();
    ~CWindow();

    static CWindow* GetInstance() {return CWorld::GetInstance()->GetWindow();};

    void Run();



    bool Initialize();
    void InitializeCubism();
    GLuint CreateBaseShader();

    GLFWwindow* GetGLWindow() { return m_window; }
    class CTexManager* GetTexManager() { return m_texManager; }
    class CCubismView* GetView() { return m_cubismView; }
    class CStateBar* GetStateBar() { return m_stateBar; }

    GLuint GetBaseShader() { return m_baseShader; }

    std::string GetExecuteAbsolutePath() { return m_executeAbsolutePath; }

    int GetMouseWinXPx() { return m_mouseWinXPx; }
    int GetMouseWinYPx() { return m_mouseWinYPx; }

    float GetMouseWinX() { return m_mouseWinX; }
    float GetMouseWinY() { return m_mouseWinY; }

    bool GetMousePressed() { return m_mousePressed; }

    bool SetMessage(
        std::string message,
        std::function<void()> confirmCallback = nullptr,
        std::function<void()> cancelCallback = nullptr);
private:
    GLFWwindow* m_window;
    class CTexManager* m_texManager;
    bool m_isEnd; 

    GLuint m_baseShader;

    class CCubismAllocator* m_cubismAllocator;
    class Csm::CubismFramework::Option* m_cubismOption;
    class CCubismView* m_cubismView;
    class CStateBar* m_stateBar;

    std::string m_executeAbsolutePath;

    int m_mouseWinXPx;
    int m_mouseWinYPx;

    float m_mouseWinX; // OpenGL window coordinates
    float m_mouseWinY;

    bool m_mousePressed;

    // Message box
    std::function<void()> m_confirmCallback;
    std::function<void()> m_cancelCallback;
    std::string m_message;
    bool m_showMessageBox;
    void msgBoxRender();

// private:
//     Csm::CubismOption _cubismOption;
//     Csm::CubismAllocator<_cubismAllocator> _cubismAllocator;
};