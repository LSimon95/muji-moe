/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#pragma once

#include <string>
#include <stdio.h>
#include <vector>
#include <cpr/cpr.h>
#include <json/json.h>
#include <thread>

#include "../version.h"

// if debug mode is enabled, we will use the local server
// #define REECHO_API_URL "http://127.0.0.1:8000/api"
#define REECHO_API_URL "https://v1.reecho.cn/api"
#define REECHO_URL "https://reecho.ai"

static const char *CONFIG_IMAGE_RESOLUTIONS[] = {
    "240x480", // 1 : 2
    "320x480", // 2 : 3

    "300x600", // 1 : 2
    "400x600", // 2 : 3

    "360x720", // 1 : 2
    "480x720", // 2 : 3

    "480x960", // 1 : 2
    "640x960", // 2 : 3

    "600x1200", // 1 : 2
    "800x1200", // 2 : 3
};

// Translation
#define LANGUAGES_COUNT 2
static const char *CONFIG_LANGUAGES[LANGUAGES_COUNT] = {
    "English",
    "Chinese",
    // "Japanese",
};

struct en2lang
{
    std::string lang[LANGUAGES_COUNT - 1];
};

#define TRAN(x) CWorld::GetInstance()->T(x)

class CWorld
{
public:
    CWorld();
    ~CWorld();

    static CWorld *GetInstance();
    class CWindow *GetWindow() { return m_window; };
    class CChat *GetChat() { return m_chat; };

    void ResetWorld();
    void SaveWorld();

    void Initialize();

    void FrontRender(int width, int height);
    void ServerRender();

    void Render(int width, int height);

    struct
    {
        int language;
        char version[16];
        char reechoKey[256];
        char openAIAPIKey[256];
    } m_configGeneral;

    struct 
    {
        char LLMApiUrl[512];
        char proxyUrl[512];
        char model[256];
        int chatMaxTokens;
    } m_configLLM;

    struct
    {
        char live2DModelPath[256];
        char vcID[64];
    } m_configChat;

    struct
    {
        int resolution;
        bool showBk;

    } m_configImage;

    struct
    {
        int volume;
    } m_configAudio;

    bool m_configChanged;
    bool m_triggerConfigChanged;

    bool m_showPanel;
    bool m_showConfig;


    bool CheckReechoRequestConfig();
    long ParseReechoResponse(cpr::Response& r, Json::Value &value);
    long ReechoPost(std::string url, Json::Value &data, Json::Value &response, int timeout = 10000);
    long ReechoGet(std::string url, cpr::Parameters &parameters, Json::Value &response, int timeout = 10000);

    const char *T(std::string &text) { return T(text.c_str()); };
    const char *T(const char *text);

    bool GetChatServerIsRunning();
    std::string& GetChatEmotion();

private:
    void RefreshVC();

    void CreateChat();

    class CWindow *m_window;

    std::vector<std::string> m_vcListShow;
    std::vector<std::string> m_vcListId;
    int m_vcIndex;

    std::thread *m_threadChat;
    class CChat *m_chat;

    char m_chatSystemPromptTmp[65536];
    bool m_edittingChatSystemPrompt;
};