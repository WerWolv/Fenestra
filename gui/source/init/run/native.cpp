#if !defined(OS_WEB)

    #include <fenestra/api/event_manager.hpp>
    #include <fenestra/events/system_events.hpp>
    #include <wolv/utils/guards.hpp>

    #include <init/run.hpp>
    #include <window.hpp>

    namespace fene::init {

        int runApplication() {

            bool shouldRestart = false;
            do {
                // Register an event handler that will make the application restart when requested
                shouldRestart = false;
                RequestRestartApplication::subscribe([&] {
                    shouldRestart = true;
                });

                {
                    auto splashWindow = initializeApplication();
                    // Draw the splash window while tasks are running
                    if (!splashWindow->loop())
                        FenestraApi::System::impl::addInitArgument("tasks-failed");
                }

                // Clean up everything after the main window is closed
                ON_SCOPE_EXIT {
                    deinitializeApplication();
                };

                // Main window
                Window window;
                window.loop();

            } while (shouldRestart);

            return EXIT_SUCCESS;
        }

    }

#endif