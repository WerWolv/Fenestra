#pragma once

#include <fenestra.hpp>

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <string>
#include <list>
#include <vector>

#include <fenestra/ui/view.hpp>

struct GLFWwindow;
struct ImGuiSettingsHandler;


namespace fene {

    void nativeErrorMessage(const std::string &message);

    class Window {
    public:
        Window();
        ~Window();

        void loop();
        void fullFrame();

        static void initNative();

        void resize(i32 width, i32 height);

    private:
        void configureBackend();
        void setupNativeWindow();
        void beginNativeWindowFrame();
        void endNativeWindowFrame();
        void drawTitleBar();

        void frameBegin();
        void frame();
        void frameEnd();

        void initGLFW();
        void initImGui();
        void exitGLFW();
        void exitImGui();

        void registerEventHandlers();

        SDL_Window *m_window = nullptr;
        SDL_GLContext m_glContext = nullptr;
        bool m_windowOpen = true;

        std::string m_windowTitle, m_windowTitleFull;

        u64 m_lastStartFrameTime = 0;
        u64 m_lastFrameTime = 0;

        ImGuiExt::Texture m_logoTexture;

        std::mutex m_popupMutex;
        std::list<std::string> m_popupsToOpen;
        std::vector<int> m_pressedKeys;

        bool m_unlockFrameRate = false;

        ImGuiExt::ImGuiCustomData m_imguiCustomData;

        u32 m_searchBarPosition = 0;
        bool m_emergencyPopupOpen = false;
    };

}