#include <fenestra/api/event_manager.hpp>
#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/api/task_manager.hpp>
#include <fenestra/helpers/utils.hpp>

#include <init/splash_window.hpp>
#include <init/tasks.hpp>

namespace fene::init {

    /**
     * @brief Displays the splash screen and runs all initialization tasks. The splash screen will be displayed until all tasks have finished.
     */
    [[maybe_unused]]
    std::unique_ptr<init::WindowSplash> initializeApplication() {
        auto splashWindow = std::make_unique<init::WindowSplash>();

        log::info("Using '{}' GPU", FenestraManager::System::getGPUVendor());

        // Add initialization tasks to run
        TaskManager::init();
        for (const auto &[name, task, async] : init::getInitTasks())
            splashWindow->addStartupTask(name, task, async);

        splashWindow->startStartupTasks();

        return splashWindow;
    }


    /**
     * @brief Deinitializes the application by running all exit tasks
     */
    void deinitializeApplication() {
        // Run exit tasks
        init::runExitTasks();

    }

}