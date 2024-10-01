/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#pragma once

#include <string>
#include <queue>
#include <thread>
#include <vector>
#include <json/json.h>

#define STREAM_BUFFER_SIZE 44100 * 60 * 10 // 10 minutes
#define MP3_DECODE_CHUNK_SIZE 65536
template <typename _T>
struct SStreamBuffer
{
    _T buffer[STREAM_BUFFER_SIZE]; 
    int readPos;
    int writePos;
};

struct SStreamPlayUserData
{
    int* lipEnergy;
    SStreamBuffer<short>* streamBuffer;
};

class CChat
{
public:
    CChat(class CWorld* pWorld);
    ~CChat();

    void Run();

    enum EChatCommand
    {
        CHAT_COMMAND_STOP = 0,
        CHAT_COMMAND_ERROR,
        CHAT_COMMAND_CHAT,
        CHAT_COMMAND_SET_SYSTEM_PROMPT,
        CHAT_COMMAND_CLEAR_CHAT_CONTENT,
        CHAT_COMMAND_RELOAD_CONFIG,
    };
        

    struct SChatCommand
    {
        EChatCommand cmd;
        std::string content;
    };

    std::string CheckChatConfig();

    bool GetChatCommand(std::queue<CChat::SChatCommand>* pCmdQ, CChat::SChatCommand& cmd) {
        std::lock_guard<std::mutex> lock(m_chatCmdMutex);
        if (pCmdQ->empty())
            return false;
        cmd = pCmdQ->front();
        pCmdQ->pop();
        return true;
    };
    void AddChatCommand(std::queue<CChat::SChatCommand>* pCmdQ,CChat::SChatCommand cmd) {
        std::lock_guard<std::mutex> lock(m_chatCmdMutex);
        pCmdQ->push(cmd);
    };

    void SendCommand2Chat(CChat::SChatCommand cmd) {
        AddChatCommand(&m_chatCommands2Chat, cmd);
    };

    bool GetCommand2World(CChat::SChatCommand& cmd) {
        return GetChatCommand(&m_chatCommands2World, cmd);
    };

    bool IsRunning() { return m_running; };

    std::string m_systemPromptShow;
    std::string m_chatContentShow;
    std::string m_emotionShow;
    int m_lipEnergyShow;

    struct SChatResponse
    {
        std::string text;
        std::string emoMotion;
    };

private:
    int m_mode; // 0: LLM+TTS 1: RTC
    class CWorld* m_pWorld;
    bool m_running;

    std::mutex m_chatCmdMutex;
    std::queue<CChat::SChatCommand> m_chatCommands2Chat;
    std::queue<CChat::SChatCommand> m_chatCommands2World;

    void StartRecv();
    bool ChatCompletion(SChatResponse& chatResponse);
    void TTS(SChatResponse& chatResponse);

    std::string m_chatContentsJsonPath;
    Json::Value m_chatContents;
    void SaveChatContents();
    void ChatContents2show();

    Json::Value m_voiceCharacterInfo;

    bool StreamDecode(std::string data, intptr_t userdata);
    void DecodeMP3(bool last_chunk = false);


    std::thread* m_threadSoundPlay;

    SStreamBuffer<short> m_streamPlayBuffer;
    SStreamPlayUserData m_streamPlayUserData;

    SStreamBuffer<char> m_streamDecodeBuffer;
};