/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include <iostream>

#include <Math/CubismMatrix44.hpp>

#include "window.hpp"
#include "texManager.hpp"
#include "cubismAllocator.hpp"
#include "cubismView.hpp"
#include "statebar.hpp"
#include "../plat.hpp"

#include "GUI/imgui.h"
#include "GUI/imgui_impl_glfw.h"
#include "GUI/imgui_impl_opengl3.h"

CWindow::CWindow() : m_window(nullptr),
                     m_isEnd(false),
                     m_mouseWinXPx(0),
                     m_mouseWinYPx(0),
                     m_mouseWinX(0.0f),
                     m_mouseWinY(0.0f),
                     m_mousePressed(false),

                     m_confirmCallback(nullptr),
                     m_cancelCallback(nullptr),
                     m_message(""),
                     m_showMessageBox(false)
{

    m_texManager = new CTexManager();
    m_cubismAllocator = new CCubismAllocator();
    m_cubismOption = new Csm::CubismFramework::Option();
    m_cubismView = new CCubismView();
    m_stateBar = new CStateBar();

    m_executeAbsolutePath = CPlat::GetExecuteAbsolutePath();
}

CWindow::~CWindow()
{
    glfwDestroyWindow(m_window);

    glfwTerminate();

    delete m_stateBar;
    delete m_texManager;
    delete m_cubismAllocator;
    delete m_cubismOption;
    delete m_cubismView;

    // clean up cubism
    Csm::CubismFramework::Dispose();
    Csm::CubismFramework::CleanUp();
}

bool CWindow::Initialize()
{
    if (glfwInit() == false)
    {
        CPlat::PrintLogLn("Failed to initialize GLFW");
        return false;
    }
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

    std::string resolution = CONFIG_IMAGE_RESOLUTIONS[CWorld::GetInstance()->m_configImage.resolution];
    int width, height;
    sscanf(resolution.c_str(), "%dx%d", &width, &height);
    m_window = glfwCreateWindow(width, height, "muji_moe", NULL, NULL);

    if (m_window == nullptr)
    {
        CPlat::PrintLogLn("Failed to create window");
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
    {
        CPlat::PrintLogLn("Failed to initialize GLEW");
        return false;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_baseShader = CreateBaseShader();
    InitializeCubism();

    // Initialize state bar
    m_stateBar->Initialize();

    // Initialize cubsim view
    m_cubismView->Initialize();

    // GUI
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    io.Fonts->AddFontFromFileTTF(
        (m_executeAbsolutePath + "Resources/fonts/azhupaopaoti-1.00-951ac929733a4f5db55cb4137f96f85e-20200320114600.ttf").c_str(), 
        16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    io.Fonts->Build();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    const char *glsl_version = "#version 120";
    ImGui_ImplOpenGL3_Init(glsl_version);

    return GL_TRUE;
}

GLuint CWindow::CreateBaseShader()
{
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    const char *vertexShader = STRINGIZE(
        \n#version 120\n
        attribute vec3 position;
        attribute vec2 uv;
        varying vec2 vuv;
        void main(void){
            gl_Position = vec4(position, 1.0);
            vuv = uv;
        }
    ) ;
    glShaderSource(vertexShaderId, 1, &vertexShader, NULL);
    glCompileShader(vertexShaderId);

    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragmentShader = STRINGIZE(
        \n#version 120\n
        varying vec2 vuv;
        uniform sampler2D texture;
        uniform vec4 baseColor;
        void main(void){ gl_FragColor = texture2D(texture, vuv) * baseColor;
        }
    );
    glShaderSource(fragmentShaderId, 1, &fragmentShader, NULL);
    glCompileShader(fragmentShaderId);

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);
    glUseProgram(programId);

    return programId;
}

void CWindow::InitializeCubism()
{
    // setup cubism
    m_cubismOption->LogFunction = CPlat::PrintMessageLn;
    m_cubismOption->LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;
    Csm::CubismFramework::StartUp(m_cubismAllocator, m_cubismOption);

    // Initialize cubism
    Csm::CubismFramework::Initialize();

    // default proj
    Csm::CubismMatrix44 projection;

    CPlat::UpdateTime();
}

bool CWindow::SetMessage(
    std::string message,
    std::function<void()> confirmCallback,
    std::function<void()> cancelCallback)
{
    m_message = message;
    m_confirmCallback = confirmCallback;
    m_cancelCallback = cancelCallback;
    m_showMessageBox = true;
    return true;
}

void CWindow::msgBoxRender()
{
    if (m_showMessageBox)
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::OpenPopup(TRAN("Message"));
        if (ImGui::BeginPopupModal(TRAN("Message")), NULL, ImGuiWindowFlags_AlwaysAutoResize)
        {
            ImGui::TextWrapped("%s", m_message.c_str());

            if (ImGui::Button(TRAN("Cancel"), ImVec2(80, 0)))
            {
                m_showMessageBox = false;
                ImGui::CloseCurrentPopup();

                m_confirmCallback = nullptr;
                m_cancelCallback = nullptr;
            }
            ImGui::SameLine();
            if (ImGui::Button(TRAN("OK"), ImVec2(80, 0)))
            {
                m_showMessageBox = false;
                if (m_confirmCallback)
                    m_confirmCallback();
                ImGui::CloseCurrentPopup();

                m_confirmCallback = nullptr;
                m_cancelCallback = nullptr;
            }
            ImGui::EndPopup();
        }
    }
}

void CWindow::Run()
{
    while (glfwWindowShouldClose(m_window) == GL_FALSE && !m_isEnd)
    {
        glfwPollEvents();

        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        auto world = CWorld::GetInstance();
        if (world->m_configChanged)
        {
            std::string resolution = CONFIG_IMAGE_RESOLUTIONS[world->m_configImage.resolution];
            sscanf(resolution.c_str(), "%dx%d", &width, &height);
            glfwSetWindowSize(m_window, width, height);
        }

        if (world->m_showPanel  && width < 720)
            glfwSetWindowSize(m_window, 720, height);

        CPlat::UpdateTime();

        glClearColor(0.0f, 0.0f, 0.0f, world->m_configImage.showBk ? 0.8f : 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearDepth(1.0);

        m_cubismView->Render();
        m_stateBar->Render();

        // ImGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Record mouse event
        ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
        m_mouseWinXPx = mousePositionAbsolute.x;
        m_mouseWinYPx = mousePositionAbsolute.y;

        if (m_mouseWinXPx < -65536)
            m_mouseWinXPx = 0; // Fix mac switch desktop issue
        if (m_mouseWinYPx < -65536)
            m_mouseWinYPx = 0;

        m_mouseWinX = (float)m_mouseWinXPx / (float)width * 2.0f - 1.0f;
        m_mouseWinY = (float)m_mouseWinYPx / (float)height * -2.0f + 1.0f;

        m_mousePressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);

        msgBoxRender();

        world->Render(width, height);

        ImGui::Render();
        int display_w, display_h;

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}