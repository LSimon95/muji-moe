/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#include <fstream>
#include <iostream>
#include <sstream>

#include "world.hpp"
#include "../front/window.hpp"

#include "../front/GUI/imgui.h"
#include "../front/GUI/imgui_impl_glfw.h"
#include "../front/GUI/imgui_impl_opengl3.h"

#include "../plat.hpp"
#include "../utils/tinyxml2.h"
#include "chat.hpp"

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#define U8(_LITERAL) (const char *)u8##_LITERAL

namespace
{
    CWorld *s_instance = NULL;
}

bool VectorOfStringGetter(void *data, int n, const char **out_text)
{
    const std::vector<std::string> *v = (std::vector<std::string> *)data;
    *out_text = (*v)[n].c_str();
    return true;
}

std::map<std::string, en2lang> en2lang_map = {

    {"Are you sure you want to close the application?", {U8("您确定要关闭应用程序吗？")}},

    {"Language", {U8("语言")}},

    {"Chat History", {U8("聊天历史")}},
    {"Configuration", {U8("系统配置")}},

    {"Message", {U8("消息")}},
    {"Cancel", {U8("取消")}},
    {"OK", {U8("确认")}},
    {"Edit", {U8("编辑")}},
    {"Save", {U8("保存")}},

    {"General", {U8("常规")}},
    {"Reecho Key", {U8("Reecho API密钥")}},
    {"LLM API Key", {U8("LLM API密钥")}},

    {"LLM Config", {U8("LLM 配置")}},
    {"LLM API Url", {U8("LLM API Url")}},
    {"Proxy Url", {U8("代理 Url")}},
    {"LLM Model", {U8("LLM 模型")}},
    {"Chat Max Tokens", {U8("聊天最大令牌数")}},

    {"Voice Chat Config", {U8("Voice Chat 配置")}},
    {"Refresh voice character from Server", {U8("从服务器刷新声音角色")}},
    {"Live2D Model Path", {U8("Live2D模型路径")}},

    {"Image", {U8("图像")}},
    {"Resolution", {U8("分辨率")}},
    {"Show Background", {U8("显示背景")}},

    {"Audio", {U8("音频")}},
    {"Volume", {U8("音量")}},

    {"\"A simple AI robot!\"", {U8("一个简单的AI机器人！")}},
    {"Version: %s", {U8("版本：%s")}},

    {"Reset", {U8("重置")}},
    {"Save & Close", {U8("保存并关闭")}},

    {"Are you sure you want to reset the configuration?", {U8("您确定要重置配置吗？")}},
    {"Please input Reecho Key in the configuration first.", {U8("请先在配置中输入Reecho Key。")}},

    // Chat
    {"! LLM API key is not set.\n", {U8("! LLM API密钥未设置。\n")}},
    {"! LLM API model is not set.\n", {U8("! LLM API模型未设置。\n")}},

    {"! Voice character ID is not set.\n", {U8("! 语音角色ID未设置。\n")}},

    {"Chat", {U8("聊天")}},

    {"Reset chat content", {U8("重置聊天内容")}},
    {"Are you sure you want to reset chat content?", {U8("您确定要重置聊天内容吗？")}},

    {"Set system prompt", {U8("设置system prompt")}},

    {"System prompt", {U8("系统提示")}},
    {"Chat history", {U8("聊天历史")}},

    {"System prompt and chat history can aslo change in the chatContents.json and restart the app.", {U8("系统提示和聊天历史也可以在chatContents.json中更改并重新启动应用程序。")}},
    {"Are you sure you want to save system prompt and clear chat history?", {U8("您确定要保存系统提示并清除聊天历史吗？")}},
    {"Chat is not running. Please check the configuration.", {U8("聊天服务未运行。请检查配置。")}},
    {"Failed to parse chatContents.json! Resetting to default.", {U8("解析chatContents.json失败！重置为默认值。")}},

    {"Failed to send chat to LLM.", {U8("发送聊天到LLM失败。")}},
    {"Failed to parse LLM response.", {U8("解析LLM响应失败。")}},

    {"Failed to get voice character from Reecho.", {U8("从Reecho获取语音角色信息失败。")}},
    {"Voice character has no prompts. Please add emotion voice prompt in reecho.ai", {U8("语音角色没有情感语音。请在reecho.ai中添加情感语音。")}},

    {"Failed to complete chat.", {U8("聊天失败。")}},

    {"Muji Moe", {U8("木几萌")}}
};

CWorld::CWorld()
{
    m_configChanged = false;
    m_threadChat = nullptr;
    m_chat = nullptr;

    m_showPanel = false;
    m_showConfig = false;

    m_window = new CWindow();

    m_edittingChatSystemPrompt = false;
    memset(m_chatSystemPromptTmp, 0, sizeof(m_chatSystemPromptTmp));

    m_vcIndex = -1;
}

CWorld::~CWorld()
{
    if (m_chat)
    {
        m_chat->SendCommand2Chat(CChat::SChatCommand{CChat::EChatCommand::CHAT_COMMAND_STOP});
        m_threadChat->join();
    }
}

CWorld *CWorld::GetInstance()
{
    if (s_instance == NULL)
        s_instance = new CWorld();
    return s_instance;
}

void CWorld::ResetWorld()
{
    // Reset General
    strcpy(m_configGeneral.version, VOICE_CHAT);
    m_configGeneral.language = 0;
    m_configGeneral.reechoKey[0] = '\0';
    m_configGeneral.openAIAPIKey[0] = '\0';

    // Reset LLM
    m_configLLM.LLMApiUrl[0] = '\0';
    m_configLLM.proxyUrl[0] = '\0';
    m_configLLM.model[0] = '\0';
    m_configLLM.chatMaxTokens = 7168;

    // Reset VoiceChat
    m_configChat.live2DModelPath[0] = '\0';
    m_configChat.vcID[0] = '\0';

    // Reset Image
    m_configImage.resolution = 6;
    m_configImage.showBk = true;

    // Reset Audio
    m_configAudio.volume = 100;
}

#define SAVE_CONFIOG_STRING(configEle, config, name)    \
    tinyxml2::XMLElement *name = doc.NewElement(#name); \
    name->SetText(config.name);                         \
    configEle->InsertEndChild(name);

#define SAVE_CONFIOG_INT(configEle, config, name)       \
    tinyxml2::XMLElement *name = doc.NewElement(#name); \
    name->SetText(config.name);                         \
    configEle->InsertEndChild(name);

#define SAVE_CONFIOG_BOOL(configEle, config, name)      \
    tinyxml2::XMLElement *name = doc.NewElement(#name); \
    name->SetText(config.name);                         \
    configEle->InsertEndChild(name);

void CWorld::SaveWorld()
{
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement *root = doc.NewElement("config");
    doc.InsertFirstChild(root);

    // Save General
    tinyxml2::XMLElement *general = doc.NewElement("general");
    root->InsertEndChild(general);

    SAVE_CONFIOG_STRING(general, m_configGeneral, version);
    SAVE_CONFIOG_INT(general, m_configGeneral, language)
    SAVE_CONFIOG_STRING(general, m_configGeneral, reechoKey);
    SAVE_CONFIOG_STRING(general, m_configGeneral, openAIAPIKey);

    // Save LLM
    tinyxml2::XMLElement *llm = doc.NewElement("llm");
    root->InsertEndChild(llm);

    SAVE_CONFIOG_STRING(llm, m_configLLM, LLMApiUrl);
    SAVE_CONFIOG_STRING(llm, m_configLLM, proxyUrl);
    SAVE_CONFIOG_STRING(llm, m_configLLM, model);
    SAVE_CONFIOG_INT(llm, m_configLLM, chatMaxTokens);

    // Save VoiceChat
    tinyxml2::XMLElement *voiceChat = doc.NewElement("voiceChat");
    root->InsertEndChild(voiceChat);

    SAVE_CONFIOG_STRING(voiceChat, m_configChat, live2DModelPath);
    SAVE_CONFIOG_STRING(voiceChat, m_configChat, vcID)

    // Save Image
    tinyxml2::XMLElement *image = doc.NewElement("image");
    root->InsertEndChild(image);

    SAVE_CONFIOG_INT(image, m_configImage, resolution);
    SAVE_CONFIOG_BOOL(image, m_configImage, showBk);

    // Save Audio
    tinyxml2::XMLElement *audio = doc.NewElement("audio");
    root->InsertEndChild(audio);

    SAVE_CONFIOG_INT(audio, m_configAudio, volume);

    auto executeAbsolutePath = CPlat::GetExecuteAbsolutePath();

    tinyxml2::XMLPrinter printer;
    doc.Accept(&printer);

    std::ofstream file;

    file.open((executeAbsolutePath + "/config.xml").c_str());

    file << "<!-- This file is generated by VoiceChatAPP -->\n";
    file << "<!-- Do not modify this file unless you know what you are doing -->\n";
    file << "<!-- If you have any questions, please contact us -->\n";
    file << "<!-- www.reecho.ai -->\n";
    file << printer.CStr();

    file.close();
}

#define LOAD_CONFIOG_STRING(configEle, config, name)                  \
    value = configEle->FirstChildElement(STRINGIFY(name))->GetText(); \
    if (value)                                                        \
    {                                                                 \
        strcpy(config.name, value);                                   \
    }                                                                 \
    else                                                              \
    {                                                                 \
        config.name[0] = '\0';                                        \
    }

#define LOAD_CONFIOG_INT(configEle, config, name) \
    config.name = configEle->FirstChildElement(STRINGIFY(name))->IntText();

#define LOAD_CONFIOG_BOOL(configEle, config, name) \
    config.name = configEle->FirstChildElement(STRINGIFY(name))->BoolText();

void CWorld::Initialize()
{
    const char *value;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError res = doc.LoadFile((CPlat::GetExecuteAbsolutePath() + "/config.xml").c_str());

    if (res != tinyxml2::XML_SUCCESS)
    {
        ResetWorld();
        SaveWorld();
        return;
    }

    tinyxml2::XMLElement *root = doc.FirstChildElement("config");
    // Load General
    tinyxml2::XMLElement *general = root->FirstChildElement("general");
    LOAD_CONFIOG_STRING(general, m_configGeneral, version);

    if (strcmp(m_configGeneral.version, VOICE_CHAT) != 0)
    {
        ResetWorld();
        SaveWorld();
        return;
    }

    LOAD_CONFIOG_INT(general, m_configGeneral, language);
    LOAD_CONFIOG_STRING(general, m_configGeneral, reechoKey);
    LOAD_CONFIOG_STRING(general, m_configGeneral, openAIAPIKey);

    // Load LLM
    tinyxml2::XMLElement *llm = root->FirstChildElement("llm");

    LOAD_CONFIOG_STRING(llm, m_configLLM, LLMApiUrl);
    LOAD_CONFIOG_STRING(llm, m_configLLM, proxyUrl);
    LOAD_CONFIOG_STRING(llm, m_configLLM, model);
    LOAD_CONFIOG_INT(llm, m_configLLM, chatMaxTokens);

    // Load voiceChat
    tinyxml2::XMLElement *voiceChat = root->FirstChildElement("voiceChat");

    auto path = voiceChat->FirstChildElement("live2DModelPath")->GetText();

    LOAD_CONFIOG_STRING(voiceChat, m_configChat, live2DModelPath);
    LOAD_CONFIOG_STRING(voiceChat, m_configChat, vcID);

    // Load Image
    tinyxml2::XMLElement *image = root->FirstChildElement("image");

    LOAD_CONFIOG_INT(image, m_configImage, resolution);
    LOAD_CONFIOG_BOOL(image, m_configImage, showBk);

    // Load Audio
    tinyxml2::XMLElement *audio = root->FirstChildElement("audio");

    LOAD_CONFIOG_INT(audio, m_configAudio, volume);

    RefreshVC();

    m_window->Initialize();

    CreateChat();
}

void CWorld::CreateChat()
{
    auto threadChat = [](CWorld *world, CChat **chat)
    {
        *chat = new CChat(world);
        (*chat)->Run();
    };

    m_threadChat = new std::thread(threadChat, this, &m_chat);

    while (m_chat == nullptr)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    m_chat->SendCommand2Chat(CChat::SChatCommand{CChat::EChatCommand::CHAT_COMMAND_RELOAD_CONFIG});
}

const char *CWorld::T(const char *text)
{
    auto it = en2lang_map.find(text);
    if (it == en2lang_map.end() || m_configGeneral.language == 0 || m_configGeneral.language > LANGUAGES_COUNT - 1)
    {
        return text;
    }
    return it->second.lang[m_configGeneral.language - 1].c_str();
}

void CWorld::FrontRender(int width, int height)
{
    if (m_showPanel)
    {
        ImGui::SetNextWindowPos(ImVec2(width / 2, 0.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(width / 2, height), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(width / 2, height), ImVec2(width / 2, height));

        if (m_showConfig)
        {
            ImGui::Begin(TRAN("Configuration"), nullptr);
            if (ImGui::Button(TRAN("Chat History"), ImVec2(-1, 0)))
                m_showConfig = false;
            ImGui::PushItemWidth(-1);

            if (ImGui::CollapsingHeader(TRAN("General")))
            {
                ImGui::Text("%s", TRAN("Language"));
                ImGui::Combo(TRAN("##Language"), &m_configGeneral.language, CONFIG_LANGUAGES, IM_ARRAYSIZE(CONFIG_LANGUAGES));
                ImGui::Text("%s", TRAN("Reecho Key"));
                ImGui::InputText("##Reecho Key", m_configGeneral.reechoKey, IM_ARRAYSIZE(m_configGeneral.reechoKey));
                ImGui::Text("%s", TRAN("LLM API Key"));
                ImGui::InputText("##LLM API Key", m_configGeneral.openAIAPIKey, IM_ARRAYSIZE(m_configGeneral.openAIAPIKey));
            }

            if (ImGui::CollapsingHeader(TRAN("LLM Config")))
            {
                ImGui::Text("%s", TRAN("LLM API Url"));
                ImGui::InputText("##LLM API Url", m_configLLM.LLMApiUrl, IM_ARRAYSIZE(m_configLLM.LLMApiUrl));
                ImGui::Text("%s", TRAN("Proxy Url"));
                ImGui::InputText("##Proxy Url", m_configLLM.proxyUrl, IM_ARRAYSIZE(m_configLLM.proxyUrl));
                ImGui::Text("%s", TRAN("LLM Model"));
                ImGui::InputText("##LLM API", m_configLLM.model, IM_ARRAYSIZE(m_configLLM.model));
                ImGui::Text("%s", TRAN("Chat Max Tokens"));
                ImGui::InputInt("##Chat Max Tokens", &m_configLLM.chatMaxTokens);
            }

            if (ImGui::CollapsingHeader(TRAN("Voice Chat Config")))
            {
                ImGui::Text("Voice Chat");
                if (ImGui::Button(TRAN("Refresh voice character from Server"), ImVec2(-1, 0)))
                    RefreshVC();

                ImGui::Combo(TRAN("##Voice Chat"), &m_vcIndex, VectorOfStringGetter, (void *)&m_vcListShow, m_vcListShow.size());
                if (m_vcIndex != -1) {
                    strcpy(m_configChat.vcID, m_vcListId[m_vcIndex].c_str());
                }

                ImGui::Text("%s", TRAN("Live2D Model Path"));
                ImGui::InputText("##Live2D Model Path", m_configChat.live2DModelPath, IM_ARRAYSIZE(m_configChat.live2DModelPath));
            }
            if (ImGui::CollapsingHeader(TRAN("Image")))
            {
                ImGui::Text("%s", TRAN("Resolution"));
                ImGui::Combo("##Resolution", &m_configImage.resolution, CONFIG_IMAGE_RESOLUTIONS, IM_ARRAYSIZE(CONFIG_IMAGE_RESOLUTIONS));
                ImGui::Checkbox(TRAN("Show Background"), &m_configImage.showBk);
            }
            if (ImGui::CollapsingHeader(TRAN("Audio")))
            {
                ImGui::Text("%s", TRAN("Volume"));
                ImGui::SliderInt("##Volume", &m_configAudio.volume, 0, 100);
            }
            ImGui::NewLine();
            ImGui::Text(TRAN("Muji Moe"));
            ImGui::Text("%s", TRAN("\"A simple AI robot!\""));
            ImGui::Text("%s %s", TRAN("Version:"), m_configGeneral.version);
            ImGui::Text("www.reecho.ai");
            ImGui::Text("Copyright(c) 2024 Reecho inc.");
            ImGui::Text("All rights reserved.");
            ImGui::NewLine();
            if (ImGui::Button(TRAN("Reset"), ImVec2(-1, 0)))
            {
                CWindow::GetInstance()->SetMessage(
                    TRAN("Are you sure you want to reset the configuration?"),
                    [this]()
                    {
                        this->ResetWorld();
                        this->SaveWorld();
                        this->m_triggerConfigChanged = true;
                    });
            }
            if (ImGui::Button(TRAN("Save & Close"), ImVec2(-1, 0)))
            {
                m_showPanel = false;
                SaveWorld();
                m_triggerConfigChanged = true;
                m_chat->SendCommand2Chat(CChat::SChatCommand{CChat::EChatCommand::CHAT_COMMAND_RELOAD_CONFIG});
            }
        }
        else
        {
            ImGui::Begin(TRAN("Chat"), nullptr);
            if (ImGui::Button(TRAN("Configuration"), ImVec2(-1, 0)))
                m_showConfig = true;
            ImGui::TextWrapped("%s", TRAN("System prompt and chat history can aslo change in the chatContents.json and restart the app."));

            if (ImGui::CollapsingHeader(TRAN("System prompt")))
            {
                if (!m_edittingChatSystemPrompt)
                {
                    ImGui::TextWrapped("%s", m_chat->m_systemPromptShow.c_str());
                    if (ImGui::Button(TRAN("Edit"), ImVec2(-1, 0)))
                    {
                        m_edittingChatSystemPrompt = true;
                        strcpy(m_chatSystemPromptTmp, m_chat->m_systemPromptShow.c_str());
                    }
                }
                else
                {
                    ImGui::InputTextMultiline(
                        "##System prompt",
                        m_chatSystemPromptTmp,
                        IM_ARRAYSIZE(m_chatSystemPromptTmp),
                        ImVec2(-1, 300));
                    if (ImGui::Button(TRAN("Save"), ImVec2(-1, 0)))
                    {
                        CWindow::GetInstance()->SetMessage(
                            TRAN("Are you sure you want to save system prompt and clear chat history?"),
                            [this]()
                            {
                                m_chat->SendCommand2Chat(CChat::SChatCommand{CChat::EChatCommand::CHAT_COMMAND_SET_SYSTEM_PROMPT, m_chatSystemPromptTmp});
                                m_edittingChatSystemPrompt = false;
                            });
                    }
                    if (ImGui::Button(TRAN("Cancel"), ImVec2(-1, 0)))
                    {
                        m_edittingChatSystemPrompt = false;
                    }
                }
            }
            if (ImGui::CollapsingHeader(TRAN("Chat history")))
            {
                ImGui::TextWrapped("%s", m_chat->m_chatContentShow.c_str());
                if (ImGui::Button(TRAN("Reset chat content"), ImVec2(-1, 0)))
                {
                    CWindow::GetInstance()->SetMessage(
                        TRAN("Are you sure you want to reset chat content?"),
                        [this]()
                        {
                            m_chat->SendCommand2Chat(CChat::SChatCommand{CChat::EChatCommand::CHAT_COMMAND_CLEAR_CHAT_CONTENT});
                        });
                }
            }
        }

        // bool isHovered = ImGui::IsItemHovered();
        // bool isFocused = ImGui::IsItemFocused();
        // ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
        // ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
        // ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);
        // ImGui::Text("Is mouse over screen? %s", isHovered ? "Yes" : "No");
        // ImGui::Text("Is screen focused? %s", isFocused ? "Yes" : "No");
        // ImGui::Text("Position: %f, %f", mousePositionAbsolute.x, mousePositionAbsolute.y);
        // ImGui::Text("Mouse clicked: %s", ImGui::IsMouseDown(ImGuiMouseButton_Left) ? "Yes" : "No");
        // ImGui::Text("mouse Position: %d, %d", m_mouseWinXPx, m_mouseWinYPx);
        // ImGui::Text("mouse Win Position: %f, %f", m_mouseWinX, m_mouseWinY);

        ImGui::End();
    }
}

void CWorld::ServerRender()
{
    if (m_triggerConfigChanged)
    {
        m_configChanged = true;
        m_triggerConfigChanged = false;
    }
    else if (m_configChanged)
    {
        m_configChanged = false;
    }

    // Chat
    CChat::SChatCommand cmd;
    if (m_chat->GetCommand2World(cmd))
    {
        if (cmd.cmd == CChat::EChatCommand::CHAT_COMMAND_ERROR)
            CWindow::GetInstance()->SetMessage(cmd.content);
    }
}

void CWorld::Render(int width, int height)
{
    FrontRender(width, height);
    ServerRender();
}

bool CWorld::GetChatServerIsRunning()
{
    if (m_chat)
        return m_chat->IsRunning();
    else
        return false;
};

std::string &CWorld::GetChatEmotion() { return m_chat->m_emotionShow; };

bool CWorld::CheckReechoRequestConfig()
{
    if (strlen(m_configGeneral.reechoKey) == 0)
    {
        CWindow::GetInstance()->SetMessage(TRAN("Please input Reecho Key in the configuration first."));
        std::string empty;
        return false;
    }
    return true;
}

long CWorld::ParseReechoResponse(cpr::Response &r, Json::Value &value)
{
    if (r.status_code != 200)
    {
        std::ostringstream ss;
        ss << "Reecho API Error(" << r.status_code << "): " << r.text;
        CWindow::GetInstance()->SetMessage(ss.str());
        return r.status_code;
    }

    Json::Reader reader;

    if (!reader.parse(r.text, value))
    {
        CWindow::GetInstance()->SetMessage("Reecho API Error: Failed to parse JSON.");
        return -1;
    }

    return r.status_code;
}

long CWorld::ReechoPost(std::string url, Json::Value &data, Json::Value &response, int timeout)
{
    if (!CheckReechoRequestConfig())
        return -1;
    // Add Reecho Key to headers
    cpr::Response r = cpr::Post(
        cpr::Url{std::string(REECHO_API_URL) + url},
        cpr::Header{{"Authorization", std::string("Bearer ") + m_configGeneral.reechoKey}, {"Content-Type", "application/json"}},
        cpr::Body{data.toStyledString()},
        cpr::Timeout{timeout});

    return ParseReechoResponse(r, response);
}

long CWorld::ReechoGet(std::string url, cpr::Parameters &parameters, Json::Value &response, int timeout)
{
    if (!CheckReechoRequestConfig())
        return -1;

    // Add Reecho Key to headers
    cpr::Response r = cpr::Get(
        cpr::Url{std::string(REECHO_API_URL) + url},
        cpr::Parameters{parameters},
        cpr::Header{{"Authorization", std::string("Bearer ") + m_configGeneral.reechoKey}},
        cpr::Timeout{timeout});

    return ParseReechoResponse(r, response);
}

void CWorld::RefreshVC()
{
    cpr::Parameters parameters = {{"showMarket", "true"}};
    Json::Value res;
    long code = ReechoGet("/tts/voice", parameters, res);
    std::cout << "RefreshVC: " << res << std::endl;
    if (code != 200)
        return;

    m_vcListShow.clear();
    m_vcIndex = -1;
    for (const auto &character : res["data"])
    {
        std::ostringstream ss;
        auto id = character["id"].asString();
        if (character["type"] && character["type"].asString().size() > 0) {
            id = "market:" + id;
        }
        m_vcListId.push_back(id);
        ss << character["name"].asString() << " (" << id << ")";
        m_vcListShow.push_back(ss.str());

        if (std::string(m_configChat.vcID) == id)
            m_vcIndex = m_vcListShow.size() - 1;
    }
}