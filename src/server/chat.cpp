/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include "chat.hpp"
#include "world.hpp"
#include <iostream>
#include <asio.hpp>
#include <algorithm>
#include "../plat.hpp"

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include <soundio/soundio.h>

using asio::ip::udp;
#define DEFAULT_CHAT_UDP_PORT 12888

#define MP3_SR 44100

static void sound_write_callback(struct SoundIoOutStream *outstream,int frame_count_min, int frame_count_max)
{
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    int sample_rate = outstream->sample_rate;
    struct SoundIoChannelArea *areas;
    SStreamPlayUserData* userdata = (SStreamPlayUserData*)outstream->userdata;
    SStreamBuffer<short>* streamPlayBuffer = userdata->streamBuffer;
    int* lipEnergy = userdata->lipEnergy;

    int buffer_frames_left = streamPlayBuffer->writePos - streamPlayBuffer->readPos;
    int frames_left = std::min(frame_count_max, buffer_frames_left);
    frames_left = std::max(frames_left, frame_count_min);
    int err;

    if ((err = soundio_outstream_begin_write(outstream, &areas, &frames_left))) {
        std::cout << "SoundPlay: begin write error: " << soundio_strerror(err) << std::endl;
        exit(1);
    }

    if (!frames_left) return;

    short maxFrameValue = 0;
    short minFrameValue = 0;
    for (int frame = 0; frame < frames_left; frame += 1) {
        for (int channel = 0; channel < layout->channel_count; channel += 1) {
            short *ptr = (short*)(areas[channel].ptr + areas[channel].step * frame);
            if (streamPlayBuffer->readPos < streamPlayBuffer->writePos) {
                *ptr = streamPlayBuffer->buffer[streamPlayBuffer->readPos];
                if (*ptr > maxFrameValue)
                    maxFrameValue = *ptr;
                if (*ptr < minFrameValue)
                    minFrameValue = *ptr;
            }
            else
                *ptr = 0;
        }
        if (streamPlayBuffer->readPos < streamPlayBuffer->writePos)
            streamPlayBuffer->readPos++;
    }

    *lipEnergy = (maxFrameValue - minFrameValue);

    // std::cout << streamPlayBuffer->readPos << " " << streamPlayBuffer->writePos << std::endl;
    // std::cout << maxFrameValue << " " << minFrameValue << " " << *lipEnergy << std::endl;

    if ((err = soundio_outstream_end_write(outstream))) {
        std::cout << "SoundPlay: end write error: " << soundio_strerror(err) << std::endl;
        exit(1);
    }
}
void SoundPlayThread(SStreamPlayUserData* userData)
{
    int err;
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        std::cout << "SoundPlay: out of memory" << std::endl;
        return;
    }

    if ((err = soundio_connect(soundio))) {
        std::cout << "SoundPlay: error connecting: " << soundio_strerror(err) << std::endl;
        return;
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
        std::cout << "SoundPlay: no output device found" << std::endl;
        return;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, default_out_device_index);
    if (!device) {
        std::cout << "SoundPlay: out of memory" << std::endl;
        return;
    }

    std::cout << "SoundPlay: Output device: " << device->name << std::endl;

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    outstream->format = SoundIoFormatS16NE;
    outstream->write_callback = sound_write_callback;
    outstream->userdata = userData;
    outstream->sample_rate = MP3_SR;

    if ((err = soundio_outstream_open(outstream))) {
        std::cout << "SoundPlay: unable to open device: " << soundio_strerror(err) << std::endl;
        return;
    }

    if (outstream->layout_error)
        std::cout << "SoundPlay: unable to set channel layout: " << soundio_strerror(outstream->layout_error) << std::endl;

    if ((err = soundio_outstream_start(outstream))) {
        std::cout << "SoundPlay: unable to start device: " << soundio_strerror(err) << std::endl;
        return;
    }

    for (;;)
        soundio_wait_events(soundio);

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
}

CChat::CChat(CWorld *pWorld)
    : m_pWorld(pWorld)
{
    m_running = false;
    m_chatContentsJsonPath = CPlat::GetExecuteAbsolutePath() + "/m_chatContents.json";
    if (std::filesystem::exists(m_chatContentsJsonPath))
    {
        std::ifstream ifs(m_chatContentsJsonPath);
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!parseFromStream(builder, ifs, &m_chatContents, &errs))
        {
            AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Failed to parse m_chatContents.json! Resetting to default.")});
            // Remove the file
            std::filesystem::remove(m_chatContentsJsonPath);
        }

        ChatContents2show();
    }

    m_streamPlayUserData.lipEnergy = &m_lipEnergyShow;
    m_streamPlayUserData.streamBuffer = &m_streamPlayBuffer;


    m_threadSoundPlay = new std::thread(&SoundPlayThread, &m_streamPlayUserData);

    m_streamPlayBuffer.writePos = 0;
    m_streamPlayBuffer.readPos = 0;

    m_streamDecodeBuffer.writePos = 0;
    m_streamDecodeBuffer.readPos = 0;
    m_lipEnergyShow = 0;
}

CChat::~CChat()
{
    m_threadSoundPlay->join();
    delete m_threadSoundPlay;
}

std::string CChat::CheckChatConfig()
{

    std::string error;

    // Check openai key
    if (m_pWorld->m_configGeneral.openAIAPIKey[0] == '\0' || m_pWorld->m_configLLM.LLMApiUrl[0] == '\0')
        error += TRAN("! LLM API key is not set.\n");
    if (m_pWorld->m_configLLM.model[0] == '\0')
        error += TRAN("! LLM API model is not set.\n");
    if (m_pWorld->m_configChat.vcID[0] == '\0')
        error += TRAN("! Reecho ID is not set.\n");

    return error;
}

void CChat::StartRecv()
{
    asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), DEFAULT_CHAT_UDP_PORT));

    for (;;)
    {
        std::array<char, 32768> recvBuffer;
        udp::endpoint remote_endpoint;
        auto size = socket.receive_from(asio::buffer(recvBuffer), remote_endpoint);
        recvBuffer[size] = '\0';

        SendCommand2Chat({CHAT_COMMAND_CHAT, std::string(recvBuffer.data())});
    }
}

void CChat::SaveChatContents()
{
    std::ofstream ofs;
    ofs.open(m_chatContentsJsonPath, std::ofstream::out | std::ofstream::trunc);
    ofs << m_chatContents;
    ofs.close();
}

void CChat::ChatContents2show()
{
    int startChatContent = 0;

    this->m_systemPromptShow = "";
    this->m_chatContentShow = "";

    if (m_chatContents.size() > 0)
    {
        if (m_chatContents[0]["role"].asString() == "system")
        {
            m_systemPromptShow = m_chatContents[0]["content"].asString();
            startChatContent = 1;
        }
        if (m_chatContents.size() > startChatContent)
        {
            for (int i = startChatContent; i < m_chatContents.size(); i++)
            {
                this->m_chatContentShow += m_chatContents[i]["role"].asString();
                this->m_chatContentShow += ": ";
                this->m_chatContentShow += m_chatContents[i]["content"].asString() + "\n";
            }
        }
    }
}

bool CChat::ChatCompletion(SChatResponse &chatResponse)
{
    // LLM
    cpr::Session session;

    if (m_pWorld->m_configLLM.proxyUrl[0] != '\0')
    {
        session.SetProxies(cpr::Proxies{{"http", m_pWorld->m_configLLM.proxyUrl},
                                        {"https", m_pWorld->m_configLLM.proxyUrl}});
    }

    Json::Value body = Json::Value();
    body["model"] = m_pWorld->m_configLLM.model;
    body["messages"] = m_chatContents;

    std::string url = m_pWorld->m_configLLM.LLMApiUrl;

    session.SetUrl(cpr::Url{url});
    session.SetHeader(cpr::Header{{"Authorization", std::string("Bearer ") + m_pWorld->m_configGeneral.openAIAPIKey},
                                  {"Content-Type", "application/json"}});

    session.SetBody(cpr::Body{body.toStyledString()});
    session.SetTimeout(cpr::Timeout{10000});

    auto response = session.Post();

    if (response.status_code != 200)
    {
        std::cout << response.status_code << " " << response.text << std::endl;
        AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Failed to send chat to LLM.")});
        return false;
    }

    Json::Value responseJson;
    Json::CharReaderBuilder builder;

    JSONCPP_STRING errs;

    auto strStream = std::istringstream(response.text);

    if (!parseFromStream(builder, strStream, &responseJson, &errs))
    {
        AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Failed to parse LLM response.")});
        return false;
    }

    if (responseJson["error"].asString() != "")
    {
        AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, responseJson["error"].asString()});
        return false;
    }

    auto resonse = responseJson["choices"][0]["message"];
    std::cout << "Response from LLM: " << resonse << std::endl;
    m_chatContents.append(resonse);

    chatResponse.emoMotion = "";
    chatResponse.text = resonse["content"].asString();

    while (true)
    {
        auto emoMotionStart = chatResponse.text.find("[");
        auto emoMotionEnd = chatResponse.text.find("]");

        if (emoMotionStart != std::string::npos && emoMotionEnd != std::string::npos && emoMotionEnd > emoMotionStart)
        {
            chatResponse.emoMotion = chatResponse.text.substr(emoMotionStart, emoMotionEnd - emoMotionStart + 1); // [...]

            auto content = chatResponse.text;
            chatResponse.text = "";
            if (emoMotionStart > 0)
                chatResponse.text = content.substr(0, emoMotionStart - 1);
            chatResponse.text += content.substr(emoMotionEnd + 1);
        }
        else
            break;
    }

    std::cout << chatResponse.text << std::endl;
    std::cout << chatResponse.emoMotion << std::endl;

    m_emotionShow = chatResponse.emoMotion;

    return true;
}

void CChat::DecodeMP3(bool last_chunk) {
    mp3dec_frame_info_t info;
    mp3dec_t mp3d;
    while (true)
    {
        auto input_buf = (const uint8_t *)(m_streamDecodeBuffer.buffer + m_streamDecodeBuffer.readPos);
        int buf_size = m_streamDecodeBuffer.writePos - m_streamDecodeBuffer.readPos;
        /*unsigned char *input_buf; - input byte stream*/
        int samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, m_streamPlayBuffer.buffer + m_streamPlayBuffer.writePos, &info);

        if (samples == 0 && info.frame_bytes == 0)
            break;
        m_streamPlayBuffer.writePos += samples;
        m_streamDecodeBuffer.readPos += info.frame_bytes;

        if (!last_chunk && buf_size < MP3_DECODE_CHUNK_SIZE / 2) // 32768 / 2
            break;
    }
}

bool CChat::StreamDecode(std::string data, intptr_t userdata)
{
    if (m_streamDecodeBuffer.writePos + data.size() > STREAM_BUFFER_SIZE)
    {
        std::cout << "StreamDecode buffer overflow!" << std::endl;
        return false;
    }

    memcpy(m_streamDecodeBuffer.buffer + m_streamDecodeBuffer.writePos, data.c_str(), data.size());
    m_streamDecodeBuffer.writePos += data.size();

    int buf_size = m_streamDecodeBuffer.writePos - m_streamDecodeBuffer.readPos;

    if (buf_size > MP3_DECODE_CHUNK_SIZE) 
        DecodeMP3();
    return true; // Return `true` on success, or `false` to **cancel** the transfer.
}

typedef struct WAV_HEADER {
  /* RIFF Chunk Descriptor */
  uint8_t RIFF[4] = {'R', 'I', 'F', 'F'}; // RIFF Header Magic header
  uint32_t ChunkSize;                     // RIFF Chunk Size
  uint8_t WAVE[4] = {'W', 'A', 'V', 'E'}; // WAVE Header
  /* "fmt" sub-chunk */
  uint8_t fmt[4] = {'f', 'm', 't', ' '}; // FMT header
  uint32_t Subchunk1Size = 16;           // Size of the fmt chunk
  uint16_t AudioFormat = 1; // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                            // Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t NumOfChan = 1;   // Number of channels 1=Mono 2=Sterio
  uint32_t SamplesPerSec = 44100;   // Sampling Frequency in Hz
  uint32_t bytesPerSec = 44100 * 2; // bytes per second
  uint16_t blockAlign = 2;          // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitsPerSample = 16;      // Number of bits per sample
  /* "data" sub-chunk */
  uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
  uint32_t Subchunk2Size;                        // Sampled data length
} wav_hdr;

void CChat::TTS(SChatResponse &chatResponse)
{
    // TTS
    Json::Value response = Json::Value();
    Json::Value data = Json::Value();

    // Synthesis parameters from https://dev.reecho.cn/

    data["voiceId"] = m_pWorld->m_configChat.vcID;
    // Find the character_id and emotion_id
    auto prompts = m_voiceCharacterInfo["data"]["metadata"]["prompts"];
    std::string e_id = "";
    for (int i = 0; i < prompts.size(); i++)
    {

        if (chatResponse.emoMotion.find(prompts[i]["name"].asString()) != std::string::npos)
        {
            std::cout << "Found voice emotion: " << prompts[i]["name"].asString() << std::endl;
            e_id = prompts[i]["id"].asString();
            break;
        }
    }
    // Random emotion if not found
    if (e_id.length() == 0)
        e_id = prompts[rand() % prompts.size()]["id"].asString();
    data["promptId"] = e_id;
    data["text"] = chatResponse.text;
    data["model"] = "reecho-neural-voice-001",
    data["randomness"] = 97;
    data["stability_boost"] = 100;
    data["probability_optimization"] = 99;
    data["break_clone"] = false;
    data["flash"] = true;
    data["stream"] = true;

    std::cout << "Request to synthesis: " << data << std::endl;

    std::string url = "/tts/simple-generate";
    auto parameters = cpr::Parameters();
    auto state_code = m_pWorld->ReechoPost(url, data, response);
    std::cout << "Response from synthesis: " << response << std::endl;

    if ((state_code != 200) || (response["status"] && response["status"] != 200))
    {
        AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Failed to synthesis voice.")});
        return;
    }

    cpr::Session session;
    session.SetUrl(cpr::Url{(response["data"]["streamUrl"].asString())});
    // session.SetTimeout(cpr::Timeout{100000}); // Max 100s

    m_streamDecodeBuffer.writePos = 0;
    m_streamDecodeBuffer.readPos = 0;

    m_streamPlayBuffer.writePos = 0; // Set write index to 0 first
    m_streamPlayBuffer.readPos = 0;

    session.SetWriteCallback(cpr::WriteCallback{std::bind(&CChat::StreamDecode, this, std::placeholders::_1, std::placeholders::_2), (intptr_t)nullptr});
    cpr::Response r = session.Get();
    DecodeMP3(true); // Decode the remaining data

    // Save m_streamDecodeBuffer to mp3
    std::ofstream outMP3("test.mp3", std::ios::binary);
    outMP3.write(reinterpret_cast<const char *>(m_streamDecodeBuffer.buffer), m_streamDecodeBuffer.writePos);
    outMP3.close();

    // Save m_streamPlayBuffer to wav
    static_assert(sizeof(wav_hdr) == 44, "");
    wav_hdr wav;
    uint32_t fsize = m_streamPlayBuffer.writePos * 2;
    wav.ChunkSize = fsize + sizeof(wav_hdr) - 8;
    wav.Subchunk2Size = fsize + sizeof(wav_hdr) - 44;

    std::ofstream out("test.wav", std::ios::binary);
    out.write(reinterpret_cast<const char *>(&wav), sizeof(wav));

    out.write(reinterpret_cast<const char *>(m_streamPlayBuffer.buffer), fsize);

    out.close();

}

void CChat::Run()
{
    std::thread t(&CChat::StartRecv, this);

    while (true)
    {
        SChatCommand cmd;

        if (GetChatCommand(&m_chatCommands2Chat, cmd))
        {
            if (cmd.cmd == CHAT_COMMAND_STOP)
                break;
            else if (cmd.cmd == CHAT_COMMAND_RELOAD_CONFIG)
            {
                std::string error = CheckChatConfig();
                if (!error.empty())
                {
                    AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, error});
                    m_running = false;
                }
                else
                {
                    m_running = true;

                    // Get chat info from reecho
                    std::string url = "/tts/voice/";
                    std::string vcID = m_pWorld->m_configChat.vcID;
                    /*
                    *  Maybe marketing character.
                    *  Marketing character is not compatible with private character,
                    *  we will fix it.
                    */
                    bool isMarket = (vcID.find("market") != std::string::npos);
                    if (isMarket)
                    {
                        url = "/market/voice/";
                        vcID = vcID.substr(7);
                    }

                    url += vcID;
                    auto parameters = cpr::Parameters();
                    auto state_code = m_pWorld->ReechoGet(url, parameters, m_voiceCharacterInfo);
                    if (state_code != 200)
                    {
                        AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Failed to get voice character from Reecho.")});
                        m_running = false;
                    }
                    std::cout << m_voiceCharacterInfo << std::endl;

                    if (isMarket)
                        m_voiceCharacterInfo["data"]["metadata"]["prompts"] = m_voiceCharacterInfo["data"]["metadata"]["voice"]["metadata"]["prompts"];

                    if (m_voiceCharacterInfo["data"]["metadata"]["prompts"].size() == 0)
                    {
                        AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Voice character has no prompts. Please add emotion voice prompt in reecho.ai")});
                        m_running = false;
                    }
                }
            }
            else if (cmd.cmd == CHAT_COMMAND_SET_SYSTEM_PROMPT)
            {
                m_chatContents = Json::Value();
                m_chatContents = Json::Value(Json::arrayValue);
                Json::Value systemPrompt;
                systemPrompt["role"] = "system";
                systemPrompt["content"] = cmd.content;
                m_chatContents.append(systemPrompt);
            }
            else if (cmd.cmd == CHAT_COMMAND_CLEAR_CHAT_CONTENT)
            {
                // Get system prompt
                Json::Value systemPrompt;
                // Check if array[0] is system prompt
                if (m_chatContents.size() > 0 && m_chatContents[0]["role"].asString() == "system")
                    systemPrompt = m_chatContents[0];

                m_chatContents = Json::Value();
                m_chatContents = Json::Value(Json::arrayValue);
                if (!systemPrompt.empty())
                    m_chatContents.append(systemPrompt);
            }
            else if (cmd.cmd == CHAT_COMMAND_CHAT)
            {
                if (m_running == false)
                {
                    AddChatCommand(&m_chatCommands2World, {CHAT_COMMAND_ERROR, m_pWorld->T("Chat is not running. Please check the configuration.")});
                    continue;
                }

                Json::Value chatContent;
                chatContent["role"] = "user";
                chatContent["content"] = cmd.content;
                m_chatContents.append(chatContent);

                SChatResponse chatResponse;

                if (!ChatCompletion(chatResponse)) 
                    continue;

                SaveChatContents();
                ChatContents2show();

                TTS(chatResponse);
            }

            if (cmd.cmd != CHAT_COMMAND_CHAT)
            {
                SaveChatContents();
                ChatContents2show();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}