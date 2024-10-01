target_sources(${APP_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plat.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/utils/tinyxml2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/tinyxml2.h

    ${CMAKE_CURRENT_SOURCE_DIR}/server/world.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/world.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/chat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/chat.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/rtc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/rtc.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/front/window.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/statebar.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/statebar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/live2DModel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/live2DModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/live2DManager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/live2DManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/cubismView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/cubismView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/cubismAllocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/cubismAllocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/texManager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/texManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/front/GUI/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/GUI/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/GUI/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/GUI/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/GUI/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/front/GUI/imgui_draw.cpp
)