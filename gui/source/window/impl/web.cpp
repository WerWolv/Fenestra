#if defined(OS_WEB)
#include "window.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <fenestra/api/event_manager.hpp>
#include <fenestra/events/system_events.hpp>

#include <imgui.h>
#include <imgui_internal.h>

// Function used by c++ to get the size of the html canvas
EM_JS(int, canvas_get_width, (), {
    return Module.canvas.width;
});

// Function used by c++ to get the size of the html canvas
EM_JS(int, canvas_get_height, (), {
    return Module.canvas.height;
});

// Function called by javascript
EM_JS(void, resizeCanvas, (), {
    js_resizeCanvas();
});

EM_JS(void, fixCanvasInPlace, (), {
    document.getElementById('canvas').classList.add('canvas-fixed');
});

EM_JS(void, setupThemeListener, (), {
    window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', event => {
        Module._handleThemeChange();
    });
});

EM_JS(bool, isDarkModeEnabled, (), {
    return window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches
});

EMSCRIPTEN_KEEPALIVE
extern "C" void handleThemeChange() {
    EventOSThemeChanged::post();
}


EM_JS(void, setupInputModeListener, (), {
    Module.canvas.addEventListener('mousedown', function() {
        Module._enterMouseMode();
    });

    Module.canvas.addEventListener('touchstart', function() {
        Module._enterTouchMode();
    });
});

EMSCRIPTEN_KEEPALIVE
extern "C" void enterMouseMode() {
    ImGui::GetIO().AddMouseSourceEvent(ImGuiMouseSource_Mouse);
}

EMSCRIPTEN_KEEPALIVE
extern "C" void enterTouchMode() {
    ImGui::GetIO().AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
}

namespace fene {

    void nativeErrorMessage(const std::string &message) {
        log::fatal(message);
        EM_ASM({
            alert(UTF8ToString($0));
        }, message.c_str());
    }

    void Window::configureBackend() {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    }

    void Window::initNative() {
        EM_ASM({
            // Save data directory
            FS.mkdir("/home/web_user/.local");
            FS.mount(IDBFS, {}, '/home/web_user/.local');

            FS.syncfs(true, function (err) {
                if (!err)
                    return;
                alert("Failed to load permanent file system: "+err);
            });

            // Center splash screen
            document.getElementById('canvas').classList.remove('canvas-fixed');
        });
    }

    void Window::setupNativeWindow() {
        resizeCanvas();
        setupThemeListener();
        setupInputModeListener();
        fixCanvasInPlace();

        bool themeFollowSystem = FenestraManager::System::usesSystemThemeDetection();
        EventOSThemeChanged::subscribe(this, [themeFollowSystem] {
            if (!themeFollowSystem) return;

            RequestChangeTheme::post(!isDarkModeEnabled() ? "Light" : "Dark");
        });

        EventBackendEventFired::subscribe(this, [this](const SDL_Event *event) {
            switch (event->type) {
                case SDL_EVENT_WINDOW_RESIZED:
                    this->resize(event->window.data1, event->window.data2);
                    break;
                default:
                    break;
            }
        });

        if (themeFollowSystem)
            EventOSThemeChanged::post();
    }

}

#endif