#include "init/tasks.hpp"

#include <imgui.h>

#include <fenestra/helpers/logger.hpp>

#include <fenestra/api/plugin_manager.hpp>
#include <fenestra/api/task_manager.hpp>

#include <set>

#include <wolv/io/fs.hpp>
#include <wolv/io/file.hpp>
#include <wolv/utils/string.hpp>

#include <fenestra/events/system_events.hpp>
#include <fenestra/helpers/default_paths.hpp>

namespace fene::init {

    using namespace std::literals::string_literals;

    namespace {

        bool getDefaultBorderlessWindowMode() {
            bool result;

            #if defined (OS_WINDOWS)
                result = true;

                // Intel's OpenGL driver is extremely buggy. Its issues can manifest in lots of different ways
                // such as "colorful snow" appearing on the screen or, the most annoying issue,
                // it might draw the window content slightly offset to the bottom right as seen in issue #1625

                // The latter issue can be circumvented by using the native OS decorations or by using the software renderer.
                // This code here tries to detect if the user has a problematic Intel GPU and if so, it will default to the native OS decorations.
                // This doesn't actually solve the issue at all but at least it makes Fenestra applications usable on these GPUs.
                const bool isIntelGPU = fene::containsIgnoreCase(FenestraApi::System::getGPUVendor(), "Intel");
                if (isIntelGPU) {
                    log::warn("Intel GPU detected! Intel's OpenGL GPU drivers are extremely buggy and can cause issues. If you experience any rendering bugs, please enable the Native OS Decoration setting or try the software rendererd -NoGPU release.");

                    // Non-exhaustive list of bad GPUs.
                    // If more GPUs are found to be problematic, they can be added here.
                    constexpr static std::array BadGPUs = {
                        // Sandy Bridge Series, Second Gen HD Graphics
                        "HD Graphics 2000",
                        "HD Graphics 3000"
                    };

                    const auto &glRenderer = FenestraApi::System::getGLRenderer();
                    for (const auto &badGPU : BadGPUs) {
                        if (fene::containsIgnoreCase(glRenderer, badGPU)) {
                            result = false;
                            break;
                        }
                    }
                }

            #elif defined(OS_MACOS)
                result = true;
            #elif defined(OS_LINUX)
                // On Linux, things like Window snapping and moving is hard to implement
                // without hacking e.g. libdecor, therefor we default to the native window decorations.
                result = false;
            #else
                result = false;
            #endif

            return result;
        }

    }

    bool setupEnvironment() {
        FenestraApi::System::impl::setBorderlessWindowMode(getDefaultBorderlessWindowMode());

        return true;
    }

    bool prepareExit() {
        // Terminate all asynchronous tasks
        TaskManager::exit();

        // Unlock font atlas, so it can be deleted in case of a crash
        if (ImGui::GetCurrentContext() != nullptr) {
            if (ImGui::GetIO().Fonts != nullptr) {
                ImGui::GetIO().Fonts->Locked = false;
                ImGui::GetIO().Fonts = nullptr;
            }
        }

        // Print a nice message if a crash happened while cleaning up resources
        // To the person fixing this:
        //     ALWAYS wrap static heap allocated objects inside libfenestra and other shared libraries such as std::vector, std::string, std::function, etc. in a AutoReset<T>
        //     e.g `AutoReset<std::vector<MyStruct>> m_structs;`
        //
        //     The reason this is necessary because each plugin / dynamic library gets its own instance of `std::allocator`
        //     which will try to free the allocated memory when the object is destroyed. However since the storage is static, this
        //     will happen only when these shared libraries are unloaded after main() returns. At this point all plugins have been unloaded already so
        //     the std::allocator will try to free memory in a heap that does not exist anymore which will cause a crash.
        //     By wrapping the object in a AutoReset<T>, the `EventApplicationClosing` event will automatically handle clearing the object
        //     while the heap is still valid.
        //     The heap stays valid right up to the point where `PluginManager::unload()` is called.
        EventAbnormalTermination::subscribe([](int) {
            log::fatal("A crash happened while cleaning up resources during exit!");
            log::fatal("This is most certainly because WerWolv again forgot to mark a heap allocated object as 'AutoReset'.");
            log::fatal("To the person fixing this, read the comment above this message for more information.");
        });

        FenestraApi::System::impl::cleanup();

        EventApplicationClosing::post();
        EventManager::clear();

        return true;
    }

    bool loadPlugins() {
        // Load all plugins
        #if !defined(FENESTRA_STATIC_LINK_PLUGINS)
            for (const auto &dir : paths::Plugins.read()) {
                PluginManager::addLoadPath(dir);
            }

            PluginManager::loadLibraries();
            PluginManager::load();
        #endif

        // Get loaded plugins
        const auto &plugins = PluginManager::getPlugins();

        // If no plugins were loaded, the Application wasn't installed properly. This will trigger an error popup later on
        if (plugins.empty()) {
            log::error("No plugins found!");

            FenestraApi::System::impl::addInitArgument("no-plugins");
            return false;
        }

        const auto shouldLoadPlugin = [executablePath = wolv::io::fs::getExecutablePath()](const Plugin &plugin) {
            // In debug builds, ignore all plugins that are not part of the executable directory
            #if !defined(DEBUG)
                return true;
            #endif

            if (!executablePath.has_value())
                return true;

            if (!PluginManager::getPluginLoadPaths().empty())
                return true;

            // Check if the plugin is somewhere in the same directory tree as the executable
            return !std::fs::relative(plugin.getPath(), executablePath->parent_path()).string().starts_with("..");
        };

        u32 loadErrors = 0;
        std::set<std::string> pluginNames;

        // Load library plugins first since plugins might depend on them
        for (const auto &plugin : plugins) {
            if (!plugin.isLibraryPlugin()) continue;

            if (!shouldLoadPlugin(plugin)) {
                log::debug("Skipping library plugin {}", plugin.getPath().string());
                continue;
            }

            // Initialize the library
            if (!plugin.initializePlugin()) {
                log::error("Failed to initialize library plugin {}", wolv::util::toUTF8String(plugin.getPath().filename()));
                loadErrors++;
            }
            pluginNames.insert(plugin.getPluginName());
        }

        // Load all plugins
        for (const auto &plugin : plugins) {
            if (plugin.isLibraryPlugin()) continue;

            if (!shouldLoadPlugin(plugin)) {
                log::debug("Skipping plugin {}", plugin.getPath().string());
                continue;
            }

            // Initialize the plugin
            if (!plugin.initializePlugin()) {
                log::error("Failed to initialize plugin {}", wolv::util::toUTF8String(plugin.getPath().filename()));
                loadErrors++;
            }
            pluginNames.insert(plugin.getPluginName());
        }

        // If no plugins were loaded successfully, the Application wasn't installed properly. This will trigger an error popup later on
        if (loadErrors == plugins.size()) {
            log::error("No plugins loaded successfully!");
            FenestraApi::System::impl::addInitArgument("no-plugins");
            return false;
        }
        if (pluginNames.size() != plugins.size()) {
            log::error("Duplicate plugins detected!");
            FenestraApi::System::impl::addInitArgument("duplicate-plugins");
            return false;
        }

        return true;
    }

    bool unloadPlugins() {
        PluginManager::unload();

        return true;
    }

    // Run all exit tasks, and print to console
    void runExitTasks() {
        for (const auto &[name, task, async] : init::getExitTasks()) {
            const bool result = task();
            log::info("Exit task '{0}' finished {1}", name, result ? "successfully" : "unsuccessfully");
        }
    }

    std::vector<Task> getInitTasks() {
        return {
            { "Setting up environment",  setupEnvironment,    false },
            { "Loading plugins",         loadPlugins,         false },
        };
    }

    std::vector<Task> getExitTasks() {
        return {
            { "Prepare exit",            prepareExit,      false },
            { "Unloading plugins",       unloadPlugins,    false },
        };
    }

}
