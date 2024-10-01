# muji_moe
Lightweight desktop voice chatbot developed by [Reecho.ai](www.reecho.ai), using Live2D Cubism SDK and C++. Reecho.ai is a group that focuses on multilanguage speech synthesis (Chinese, English...), and this project is a part of our research on voice chatbots. 
In this project, we use Live2D Cubism SDK to create a 2D character to interact with users. Customized OpenAI API is supported in this project to generate chat content and synthesize speech using Reecho.ai's speech synthesis engine. Furthermore, switching to RTC/WS mode can get the low-latency voice chat experience (~800 ms) but cost more tokens.
Live2D MUJI_MOE model is created by XWdit and is for non-commercial use only.

## Roadmap
- [x] Basic UI
- [x] Chatbot
- [x] Customized OpenAI API
- [x] Voice chat (TTS Mode)
- [ ] Voice chat (RTC/WS Mode)
- [ ] Windows support
- [ ] More input examples

## Video Demo
<video src="https://github.com/user-attachments/assets/36173054-afb0-404f-b28e-09dc2caac4b3" controls="controls" width="768"></video>

## Build Requirements
Put following libraries in `lib/` directory:
- Live2D Cubism Core SDK For Native(Frameworks) [5-r.1]
- asio [1.30.2]
- cpr [1.11.0]
- glew [2.1.0]
- glfw [3.4.0]
- libsoundio [1.1.0]
- jsoncpp
- stb_image

## Build and Run
```bash
cd mac && mkdir build && cd build
cmake .. && make
cd build/bin/ && ./muji_moe
```

## License
- MUJI_MOE Live2D Model (Resources/muji_moe_auto) is licensed under AGPL-3.0 License - see the [LICENSE MUJI MOE](LICENSE_MUJI_MOE).
- Live2D Cubism SDK is licensed under the Live2D Proprietary Software License Agreement.
- Live2D Model is licensed under the Live2D Proprietary Software License Agreement.
- Source code is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
