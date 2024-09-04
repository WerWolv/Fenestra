#include <fenestra.hpp>

#include <fenestra/helpers/logger.hpp>

#include "window.hpp"
#include "init/splash_window.hpp"

#include "crash_handlers.hpp"

#include <fenestra/api/task_manager.hpp>
#include <fenestra/api/plugin_manager.hpp>

namespace fene::init {

    int runApplication();
    void runCommandLine(int argc, char **argv);

}
/**
 * @brief Main entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char **argv) {
    using namespace fene;

    // Set the main thread's name to "Main"
    TaskManager::setCurrentThreadName("Main");

    // Setup crash handlers right away to catch crashes as early as possible
    crash::setupCrashHandlers();

    // Run platform-specific initialization code
    Window::initNative();

    // Handle command line arguments if any have been passed
    if (argc > 1) {
        init::runCommandLine(argc, argv);
    }

    // Log some system information to aid debugging when users share their logs
    log::info("Welcome to {} {}!", FENESTRA_APPLICATION_NAME, FenestraApi::System::getApplicationVersion());
    log::info("Compiled using commit {}@{}", FenestraApi::System::getCommitBranch(), FenestraApi::System::getCommitHash());
    log::info("Running on {} {} ({})", FenestraApi::System::getOSName(), FenestraApi::System::getOSVersion(), FenestraApi::System::getArchitecture());
    #if defined(OS_LINUX)
    auto distro = FenestraApi::System::getLinuxDistro().value();
    log::info("Linux distribution: {}. Version: {}", distro.name, distro.version == "" ? "None" : distro.version);
    #endif


    // Run Application
    return init::runApplication();
}
