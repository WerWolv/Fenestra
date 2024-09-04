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
        void macosMarkContentEdited(SDL_Window *window, bool edited = true);

    }

#endif
