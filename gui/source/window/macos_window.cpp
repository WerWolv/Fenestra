#include "window.hpp"

#if defined(OS_MACOS)

    #include <fenestra/api/fenestra_manager.hpp>
    #include <fenestra/events/system_events.hpp>
    #include <fenestra/api/task_manager.hpp>

    #include <fenestra/helpers/utils_macos.hpp>
    #include <fenestra/helpers/logger.hpp>
    #include <fenestra/helpers/default_paths.hpp>

    #include <cstdio>
    #include <unistd.h>

namespace fene {

    void nativeErrorMessage(const std::string &message) {
        log::fatal(message);
        errorMessageMacos(message.c_str());
    }

    void Window::configureBackend() {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    }

    void Window::initNative() {
        log::impl::enableColorPrinting();

        // Add plugin library folders to dll search path
        for (const auto &path : paths::Libraries.read())  {
            if (std::fs::exists(path))
                setenv("LD_LIBRARY_PATH", fmt::format("{};{}", fene::getEnvironmentVariable("LD_LIBRARY_PATH").value_or(""), path.string().c_str()).c_str(), true);
        }

        // Various libraries sadly directly print to stderr with no way to disable it
        // We redirect stderr to /dev/null to prevent this
        freopen("/dev/null", "w", stderr);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // Redirect stdout to log file if we're not running in a terminal
        if (!isatty(STDOUT_FILENO)) {
            log::impl::redirectToFile();
        }

        enumerateFontsMacos();
    }

    void Window::setupNativeWindow() {
        bool themeFollowSystem = FenestraManager::System::usesSystemThemeDetection();
        EventOSThemeChanged::subscribe(this, [themeFollowSystem] {
            if (!themeFollowSystem) return;

            if (!isMacosSystemDarkModeEnabled())
                RequestChangeTheme::post("Light");
            else
                RequestChangeTheme::post("Dark");
        });

        if (themeFollowSystem)
            EventOSThemeChanged::post();

        setupMacosWindowStyle(m_window, FenestraManager::System::isBorderlessWindowModeEnabled());
    }

    void Window::beginNativeWindowFrame() {

    }

    void Window::endNativeWindowFrame() {

    }

}

#endif