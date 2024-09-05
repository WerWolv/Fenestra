#pragma once

#if defined(OS_MACOS)

    struct SDL_Window;

    extern "C" {

        void errorMessageMacos(const char *message);
        void openWebpageMacos(const char *url);
        bool isMacosSystemDarkModeEnabled();
        bool isMacosFullScreenModeEnabled(SDL_Window *window);
        float getBackingScaleFactor();

        void setupMacosWindowStyle(SDL_Window *window, bool borderlessWindowMode);

        void enumerateFontsMacos();

        void macosHandleTitlebarDoubleClickGesture(SDL_Window *window);
        bool macosIsWindowBeingResizedByUser(SDL_Window *window);
        void macosMarkContentEdited(SDL_Window *window, bool edited);

        typedef void (*MenuAction)(void *);
        void macosAddMainMenuEntry(const char **path, unsigned long pathSize, MenuAction action, void *userdata);

        void macosSetDockProgressBar(double progress);

    }

    #include <fenestra/helpers/auto_reset.hpp>
    #include <span>
    #include <string>

    namespace fene::macos {

        template<typename T>
        void addMainMenuEntry(std::span<std::string> path, T &&callback) {
            std::vector<const char*> paths;
            for (const auto &entry : path) {
                paths.push_back(entry.c_str());
            }

            static AutoReset<std::list<std::function<void()>>> callbackHolder;
            callbackHolder->push_back(std::forward<T>(callback));

            macosAddMainMenuEntry(paths.data(), paths.size(), [](void *userdata) {
                (*static_cast<std::function<void()>*>(userdata))();
            }, &callbackHolder->back());
        }

    }

#endif
