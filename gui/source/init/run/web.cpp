#if defined(OS_WEB)

    #include <emscripten.h>
    #include <emscripten/html5.h>

    #include <fenestra/api/fenestra_manager.hpp>
    #include <fenestra/api/event_manager.hpp>
    #include <fenestra/api/task_manager.hpp>

    #include <fenestra/events/system_events.hpp>

    #include <window.hpp>

    #include <init/run.hpp>

    namespace fene::init {

        void saveFsData() {
            EM_ASM({
                FS.syncfs(function (err) {
                    if (!err)
                        return;
                    alert("Failed to save permanent file system: "+err);
                });
            });
        }

        int runApplication() {
            static std::unique_ptr<init::WindowSplash> splashWindow;
            splashWindow = initializeApplication();

            RequestRestartApplication::subscribe([&] {
                MAIN_THREAD_EM_ASM({
                    location.reload();
                });
            });

            // Draw the splash window while tasks are running
            emscripten_set_main_loop_arg([](void *arg) {
                auto &splashWindow = *reinterpret_cast<std::unique_ptr<init::WindowSplash>*>(arg);

                FrameResult frameResult = splashWindow->fullFrame();
                if (frameResult == FrameResult::Success) {
                    // Clean up everything after the main window is closed
                    emscripten_set_beforeunload_callback(nullptr, [](int eventType, const void *reserved, void *userData) {
                        std::ignore = eventType;
                        std::ignore = reserved;
                        std::ignore = userData;

                        emscripten_cancel_main_loop();

                        try {
                            saveFsData();
                            deinitializeApplication();
                            return "";
                        } catch (const std::exception &e) {
                            static std::string message;
                            message = fmt::format("Failed to deinitialize Application!\nThis is just a message warning you of this, the application has already closed, you probably can't do anything about it.\n\nError: {}", e.what());
                            return message.c_str();
                        }
                    });

                    // Delete splash window (do it before creating the main window so the backend destroys the window)
                    splashWindow.reset();

                    emscripten_cancel_main_loop();

                    // Main window
                    static std::optional<Window> window;
                    window.emplace();

                    emscripten_set_main_loop([]() {
                        window->fullFrame();
                    }, 60, 0);
                }
            }, &splashWindow, 60, 0);

            return -1;
        }

    }

#endif