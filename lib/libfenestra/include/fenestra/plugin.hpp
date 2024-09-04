#pragma once

#include <fenestra.hpp>
#include <fenestra/api/plugin_manager.hpp>
#include <fenestra/helpers/logger.hpp>

#include <string>

#include <imgui.h>
#include <imgui_internal.h>

#include <wolv/utils/string.hpp>
#include <wolv/utils/preproc.hpp>
#include <wolv/utils/guards.hpp>

namespace {
    struct PluginFunctionHelperInstantiation {};
}

template<typename T>
struct PluginFeatureFunctionHelper {
    static void* getFeatures();
};

template<typename T>
struct PluginSubCommandsFunctionHelper {
    static void* getSubCommands();
};

template<typename T>
void* PluginFeatureFunctionHelper<T>::getFeatures() {
    return nullptr;
}

template<typename T>
void* PluginSubCommandsFunctionHelper<T>::getSubCommands() {
    return nullptr;
}

[[maybe_unused]] static auto& getFeaturesImpl() {
    static std::vector<fene::Feature> features;
    return features;
}

#if defined (FENESTRA_STATIC_LINK_PLUGINS)
    #define FENESTRA_PLUGIN_VISIBILITY_PREFIX static
#else
    #define FENESTRA_PLUGIN_VISIBILITY_PREFIX extern "C" [[gnu::visibility("default")]]
#endif

#define FENESTRA_FEATURE_ENABLED(feature) WOLV_TOKEN_CONCAT(WOLV_TOKEN_CONCAT(WOLV_TOKEN_CONCAT(FENESTRA_PLUGIN_, FENESTRA_PLUGIN_NAME), _FEATURE_), feature)
#define FENESTRA_DEFINE_PLUGIN_FEATURES() FENESTRA_DEFINE_PLUGIN_FEATURES_IMPL()
#define FENESTRA_DEFINE_PLUGIN_FEATURES_IMPL()                                                 \
    template<>                                                                              \
    struct PluginFeatureFunctionHelper<PluginFunctionHelperInstantiation> {                 \
        static void* getFeatures();                                                         \
    };                                                                                      \
    void* PluginFeatureFunctionHelper<PluginFunctionHelperInstantiation>::getFeatures() {   \
        return &getFeaturesImpl();                                                          \
    }                                                                                       \
    static auto initFeatures = [] { getFeaturesImpl() = std::vector<fene::Feature>({ FENESTRA_PLUGIN_FEATURES_CONTENT }); return 0; }()

#define FENESTRA_PLUGIN_FEATURES ::getFeaturesImpl()

/**
 * This macro is used to define all the required entry points for a plugin.
 * Name, Author and Description will be displayed in the plugin list on the Welcome screen.
 */
#define FENESTRA_PLUGIN_SETUP(name, author, description) FENESTRA_PLUGIN_SETUP_IMPL(name, author, description)
#define FENESTRA_LIBRARY_SETUP(name) FENESTRA_LIBRARY_SETUP_IMPL(name)
#define FENESTRA_PLUGIN_STATIC_SETUP(name, author, description) FENESTRA_PLUGIN_STATIC_SETUP_IMPL(name, author, description)

#define FENESTRA_LIBRARY_SETUP_IMPL(name)                                                                                          \
    namespace { static struct EXIT_HANDLER { ~EXIT_HANDLER() { fene::log::debug("Unloaded library '{}'", name); } } HANDLER; }   \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void WOLV_TOKEN_CONCAT(initializeLibrary_, FENESTRA_PLUGIN_NAME)();                             \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *WOLV_TOKEN_CONCAT(getLibraryName_, FENESTRA_PLUGIN_NAME)() { return name; }         \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void WOLV_TOKEN_CONCAT(setImGuiContext_, FENESTRA_PLUGIN_NAME)(ImGuiContext *ctx) {             \
        ImGui::SetCurrentContext(ctx);                                                                                          \
        GImGui = ctx;                                                                                                           \
    }                                                                                                                           \
    extern "C" [[gnu::visibility("default")]] void WOLV_TOKEN_CONCAT(forceLinkPlugin_, FENESTRA_PLUGIN_NAME)() {                   \
        fene::PluginManager::addPlugin(name, fene::PluginFunctions {                                                              \
            nullptr,                                                                                                            \
            WOLV_TOKEN_CONCAT(initializeLibrary_, FENESTRA_PLUGIN_NAME),                                                           \
            nullptr,                                                                                                            \
            WOLV_TOKEN_CONCAT(getLibraryName_, FENESTRA_PLUGIN_NAME),                                                              \
            nullptr,                                                                                                            \
            nullptr,                                                                                                            \
            nullptr,                                                                                                            \
            WOLV_TOKEN_CONCAT(setImGuiContext_, FENESTRA_PLUGIN_NAME),                                                             \
            nullptr,                                                                                                            \
            nullptr,                                                                                                            \
            nullptr                                                                                                             \
        });                                                                                                                     \
    }                                                                                                                           \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void WOLV_TOKEN_CONCAT(initializeLibrary_, FENESTRA_PLUGIN_NAME)()

#define FENESTRA_PLUGIN_SETUP_IMPL(name, author, description)                                                                      \
    namespace { static struct EXIT_HANDLER { ~EXIT_HANDLER() { fene::log::debug("Unloaded plugin '{}'", name); } } HANDLER; }  \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getPluginName() { return name; }                                                 \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getPluginAuthor() { return author; }                                             \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getPluginDescription() { return description; }                                   \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getCompatibleVersion() { return FENESTRA_VERSION; }                                 \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void setImGuiContext(ImGuiContext *ctx) {                                                    \
        ImGui::SetCurrentContext(ctx);                                                                                          \
        GImGui = ctx;                                                                                                           \
    }                                                                                                                           \
    FENESTRA_DEFINE_PLUGIN_FEATURES();                                                                                             \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void* getFeatures() {                                                                        \
        return PluginFeatureFunctionHelper<PluginFunctionHelperInstantiation>::getFeatures();                                   \
    }                                                                                                                           \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void* getSubCommands() {                                                                     \
        return PluginSubCommandsFunctionHelper<PluginFunctionHelperInstantiation>::getSubCommands();                            \
    }                                                                                                                           \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void initializePlugin();                                                                     \
    extern "C" [[gnu::visibility("default")]] void WOLV_TOKEN_CONCAT(forceLinkPlugin_, FENESTRA_PLUGIN_NAME)() {                   \
        fene::PluginManager::addPlugin(name, fene::PluginFunctions {                                                              \
            initializePlugin,                                                                                                   \
            nullptr,                                                                                                            \
            getPluginName,                                                                                                      \
            nullptr,                                                                                                            \
            getPluginAuthor,                                                                                                    \
            getPluginDescription,                                                                                               \
            getCompatibleVersion,                                                                                               \
            setImGuiContext,                                                                                                    \
            nullptr,                                                                                                            \
            getSubCommands,                                                                                                     \
            getFeatures                                                                                                         \
        });                                                                                                                     \
    }                                                                                                                           \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void initializePlugin()

#define FENESTRA_PLUGIN_STATIC_SETUP_IMPL(name, author, description)                              \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void initializePlugin();                                    \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getPluginName() { return name; }                \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getPluginAuthor() { return author; }            \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX const char *getPluginDescription() { return description; }  \
    namespace {                                                                                \
        struct FenestraStaticPlugin {                                                             \
            FenestraStaticPlugin() {                                                              \
                fene::PluginManager::addPlugin(name, fene::PluginFunctions {                 \
                    initializePlugin,                                                          \
                    nullptr,                                                                   \
                    getPluginName,                                                             \
                    nullptr,                                                                   \
                    getPluginAuthor,                                                           \
                    getPluginDescription,                                                      \
                    nullptr,                                                                   \
                    nullptr,                                                                   \
                    nullptr,                                                                   \
                    nullptr,                                                                   \
                    nullptr                                                                    \
                });                                                                            \
            }                                                                                  \
        }  fenestraPlugin;                                                                        \
    }                                                                                          \
    FENESTRA_PLUGIN_VISIBILITY_PREFIX void initializePlugin()

/**
 * This macro is used to define subcommands defined by the plugin
 * A subcommand consists of a key, a description, and a callback
 * The key is what the first argument to the application should be, prefixed by `--`
 * For example, if the key if `help`, the application should be started with `--help` as its first argument to trigger the subcommand
 * when the subcommand is triggerred, it's callback will be executed. The callback is executed BEFORE most of the application initialization
 * so to do anything meaningful, you should subscribe to an event (like EventApplicationStartupFinished) and run your code there.
 */
#define FENESTRA_PLUGIN_SUBCOMMANDS() FENESTRA_PLUGIN_SUBCOMMANDS_IMPL()

#define FENESTRA_PLUGIN_SUBCOMMANDS_IMPL()                                                             \
    extern std::vector<fene::SubCommand> g_subCommands;                                            \
    template<>                                                                                      \
    struct PluginSubCommandsFunctionHelper<PluginFunctionHelperInstantiation> {                     \
        static void* getSubCommands();                                                              \
    };                                                                                              \
    void* PluginSubCommandsFunctionHelper<PluginFunctionHelperInstantiation>::getSubCommands() {    \
        return &g_subCommands;                                                                      \
    }                                                                                               \
    std::vector<fene::SubCommand> g_subCommands
