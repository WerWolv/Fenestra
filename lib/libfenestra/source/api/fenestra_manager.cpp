#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/helpers/auto_reset.hpp>

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <fenestra/events/fenestra_events.hpp>
#include <fenestra/events/system_events.hpp>
#include <SDL3/SDL_clipboard.h>

#include <wolv/utils/string.hpp>

#if defined(OS_WINDOWS)
    #include <Windows.h>
#elif defined(OS_LINUX) || defined(OS_MACOS) || defined(OS_WEB)
    #include <sys/utsname.h>
#endif

namespace fene::FenestraManager {

    namespace System {

        namespace impl {

            // Default to true means we forward to ourselves by default
            static bool s_isMainInstance = true;
            void setMainInstanceStatus(bool status) {
                s_isMainInstance = status;
            }

            static ImVec2 s_mainWindowPos;
            static ImVec2 s_mainWindowSize;
            void setMainWindowPosition(i32 x, i32 y) {
                s_mainWindowPos = ImVec2(float(x), float(y));
            }

            void setMainWindowSize(u32 width, u32 height) {
                s_mainWindowSize = ImVec2(float(width), float(height));
            }

            static ImGuiID s_mainDockSpaceId;
            void setMainDockSpaceId(ImGuiID id) {
                s_mainDockSpaceId = id;
            }

            static void *s_mainWindowHandle;
            void setMainWindowHandle(void *window) {
                s_mainWindowHandle = window;
            }


            static float s_globalScale = 1.0;
            void setGlobalScale(float scale) {
                s_globalScale = scale;
            }

            static float s_nativeScale = 1.0;
            void setNativeScale(float scale) {
                s_nativeScale = scale;
            }


            static bool s_borderlessWindowMode;
            void setBorderlessWindowMode(bool enabled) {
                s_borderlessWindowMode = enabled;
            }

            static bool s_multiWindowMode = true;
            void setMultiWindowMode(bool enabled) {
                s_multiWindowMode = enabled;
            }

            static std::optional<InitialWindowProperties> s_initialWindowProperties;
            void setInitialWindowProperties(InitialWindowProperties properties) {
                s_initialWindowProperties = properties;
            }


            static AutoReset<std::string> s_gpuVendor;
            void setGPUVendor(const std::string &vendor) {
                s_gpuVendor = vendor;
            }

            static AutoReset<std::string> s_glRenderer;
            void setGLRenderer(const std::string &renderer) {
                s_glRenderer = renderer;
            }

            static AutoReset<std::map<std::string, std::string>> s_initArguments;
            void addInitArgument(const std::string &key, const std::string &value) {
                static std::mutex initArgumentsMutex;
                std::scoped_lock lock(initArgumentsMutex);

                (*s_initArguments)[key] = value;
            }

            static double s_lastFrameTime;
            void setLastFrameTime(double time) {
                s_lastFrameTime = time;
            }

            static bool s_windowResizable = true;
            bool isWindowResizable() {
                return s_windowResizable;
            }

            static std::vector<fene::impl::AutoResetBase*> s_autoResetObjects;
            void addAutoResetObject(fene::impl::AutoResetBase *object) {
                s_autoResetObjects.emplace_back(object);
            }

            void cleanup() {
                for (const auto &object : s_autoResetObjects)
                    object->reset();
            }

            bool isDPIScalingEnabled() {
                return false;
            }

        }

        double getGlobalScale() {
            return impl::s_globalScale;
        }

        double getNativeScale() {
            return impl::s_nativeScale;
        }

        bool isMainInstance() {
            return impl::s_isMainInstance;
        }

        void closeApplication(bool noQuestions) {
            RequestCloseApplication::post(noQuestions);
        }

        void restartApplication() {
            RequestRestartApplication::post();
            RequestCloseApplication::post(false);
        }

        void setTaskBarProgress(TaskProgressState state, TaskProgressType type, u32 progress) {
            EventSetTaskBarIconState::post(u32(state), u32(type), progress);
        }

        static bool s_systemThemeDetection;
        void enableSystemThemeDetection(bool enabled) {
            s_systemThemeDetection = enabled;

            EventOSThemeChanged::post();
        }

        bool usesSystemThemeDetection() {
            return s_systemThemeDetection;
        }

        void addStartupTask(const std::string &name, bool async, const std::function<bool()> &function) {
            RequestAddInitTask::post(name, async, function);
        }



        static float s_targetFPS = 14.0F;

        float getTargetFPS() {
            return s_targetFPS;
        }

        void setTargetFPS(float fps) {
            s_targetFPS = fps;
        }

        ImVec2 getMainWindowPosition() {
            if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != ImGuiConfigFlags_None)
                return impl::s_mainWindowPos;
            else
                return { 0, 0 };
        }

        ImVec2 getMainWindowSize() {
            return impl::s_mainWindowSize;
        }


        ImGuiID getMainDockSpaceId() {
            return impl::s_mainDockSpaceId;
        }

        void* getMainWindowHandle() {
            return impl::s_mainWindowHandle;
        }

        bool isBorderlessWindowModeEnabled() {
            return impl::s_borderlessWindowMode;
        }

        bool isMutliWindowModeEnabled() {
            return impl::s_multiWindowMode;
        }

        std::optional<InitialWindowProperties> getInitialWindowProperties() {
            return impl::s_initialWindowProperties;
        }

        const std::map<std::string, std::string>& getInitArguments() {
            return *impl::s_initArguments;
        }

        std::string getInitArgument(const std::string &key) {
            if (impl::s_initArguments->contains(key))
                return impl::s_initArguments->at(key);
            else
                return "";
        }

        const std::string &getGPUVendor() {
            return impl::s_gpuVendor;
        }

        const std::string &getGLRenderer() {
            return impl::s_glRenderer;
        }

        bool isPortableVersion() {
            static std::optional<bool> portable;
            if (portable.has_value())
                return portable.value();

            if (const auto executablePath = wolv::io::fs::getExecutablePath(); executablePath.has_value()) {
                const auto flagFile = executablePath->parent_path() / "PORTABLE";

                portable = wolv::io::fs::exists(flagFile) && wolv::io::fs::isRegularFile(flagFile);
            } else {
                portable = false;
            }

            return portable.value();
        }

        std::string getOSName() {
            #if defined(OS_WINDOWS)
                return "Windows";
            #elif defined(OS_LINUX)
                #if defined(OS_FREEBSD)
                    return "FreeBSD";
                #else
                    return "Linux";
                #endif
            #elif defined(OS_MACOS)
                return "macOS";
            #elif defined(OS_WEB)
                return "Web";
            #else
                return "Unknown";
            #endif
        }

        std::string getOSVersion() {
            #if defined(OS_WINDOWS)
                OSVERSIONINFOA info;
                info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
                ::GetVersionExA(&info);

                return fmt::format("{}.{}.{}", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
            #elif defined(OS_LINUX) || defined(OS_MACOS) || defined(OS_WEB)
                struct utsname details = { };

                if (uname(&details) != 0) {
                    return "Unknown";
                }

                return std::string(details.release) + " " + std::string(details.version);
            #else
                return "Unknown";
            #endif
        }

        std::string getArchitecture() {
            #if defined(OS_WINDOWS)
                SYSTEM_INFO info;
                ::GetNativeSystemInfo(&info);

                switch (info.wProcessorArchitecture) {
                    case PROCESSOR_ARCHITECTURE_AMD64:
                        return "x86_64";
                    case PROCESSOR_ARCHITECTURE_ARM:
                        return "ARM";
                    case PROCESSOR_ARCHITECTURE_ARM64:
                        return "ARM64";
                    case PROCESSOR_ARCHITECTURE_IA64:
                        return "IA64";
                    case PROCESSOR_ARCHITECTURE_INTEL:
                        return "x86";
                    default:
                        return "Unknown";
                }
            #elif defined(OS_LINUX) || defined(OS_MACOS) || defined(OS_WEB)
                struct utsname details = { };

                if (uname(&details) != 0) {
                    return "Unknown";
                }

                return { details.machine };
            #else
                return "Unknown";
            #endif
        }

        std::optional<LinuxDistro> getLinuxDistro() {
            wolv::io::File file("/etc/os-release", wolv::io::File::Mode::Read);
            std::string name;
            std::string version;

            auto fileContent = file.readString();
            for (const auto &line : wolv::util::splitString(fileContent, "\n")) {
                if (line.find("PRETTY_NAME=") != std::string::npos) {
                    name = line.substr(line.find("=") + 1);
                    std::erase(name, '\"');
                } else if (line.find("VERSION_ID=") != std::string::npos) {
                    version = line.substr(line.find("=") + 1);
                    std::erase(version, '\"');
                }
            }

            return { { name, version } };
        }

        std::string getApplicationVersion(bool withBuildType) {
            #if defined FENESTRA_VERSION
                if (withBuildType) {
                    return FENESTRA_VERSION;
                } else {
                    auto version = std::string(FENESTRA_VERSION);
                    return version.substr(0, version.find('-'));
                }
            #else
                return "Unknown";
            #endif
        }

        std::string getCommitHash(bool longHash) {
            #if defined GIT_COMMIT_HASH_LONG
                if (longHash) {
                    return GIT_COMMIT_HASH_LONG;
                } else {
                    return std::string(GIT_COMMIT_HASH_LONG).substr(0, 7);
                }
            #else
                std::ignore = longHash;
                return "Unknown";
            #endif
        }

        std::string getCommitBranch() {
            #if defined GIT_BRANCH
                return GIT_BRANCH;
            #else
                return "Unknown";
            #endif
        }

        bool isDebugBuild() {
            #if defined DEBUG
                return true;
            #else
                return false;
            #endif
        }

        bool isNightlyBuild() {
            return getApplicationVersion(false).ends_with("WIP");
        }


        void addClipboardString(std::string_view string) {
            addClipboardData({ reinterpret_cast<const u8*>(string.data()), string.size() }, "text/plain;charset=utf-8");
        }

        void addClipboardStringCallback(const std::function<std::string()> &callback) {
            addClipboardDataCallback([callback]() -> std::vector<u8> {
                auto data = callback();
                return { data.begin(), data.end() };
            }, "text/plain;charset=utf-8");
        }

        void addClipboardData(std::span<const u8> data, const std::string &mimeType) {
            std::array mimeTypes = { mimeType.c_str() };

            struct Storage {
                std::vector<u8> data;
            };

            const auto storage = new Storage;
            storage->data.assign(data.begin(), data.end());

            SDL_SetClipboardData(
                [](void *userData, const char *, size_t *size) -> const void * {
                    auto storage = static_cast<Storage*>(userData);
                    *size = storage->data.size();
                    return storage->data.data();
                },
                [](void *userData) -> void { delete static_cast<Storage*>(userData); },
                storage,
                mimeTypes.data(),
                mimeTypes.size()
            );
        }

        void addClipboardDataCallback(const std::function<std::vector<u8>()> &callback, const std::string &mimeType) {
            std::array mimeTypes = { mimeType.c_str() };

            struct Storage {
                std::function<std::vector<u8>()> function;
                std::vector<u8> data;
            };
            const auto storage = new Storage { callback, {} };

            SDL_SetClipboardData(
                [](void *userData, const char *, size_t *size) -> const void * {
                    auto storage = static_cast<Storage*>(userData);

                    storage->data = storage->function();

                    *size = storage->data.size();
                    return storage->data.data();
                },
                [](void *userData) -> void { delete static_cast<Storage*>(userData); },
                storage,
                mimeTypes.data(),
                mimeTypes.size()
            );
        }

    }

    namespace Messaging {

        namespace impl {

            static AutoReset<std::map<std::string, MessagingHandler>> s_handlers;
            const std::map<std::string, MessagingHandler>& getHandlers() {
                return *s_handlers;
            }

            void runHandler(const std::string &eventName, const std::vector<u8> &args) {
                const auto& handlers = getHandlers();
                const auto matchHandler = handlers.find(eventName);

                if (matchHandler == handlers.end()) {
                    log::error("Forward event handler {} not found", eventName);
                } else {
                    matchHandler->second(args);
                }

            }

        }

        void registerHandler(const std::string &eventName, const impl::MessagingHandler &handler) {
            log::debug("Registered new forward event handler: {}", eventName);

            impl::s_handlers->insert({ eventName, handler });
        }

    }

    namespace Fonts {

        namespace impl {

            static AutoReset<std::vector<Font>> s_fonts;
            const std::vector<Font>& getFonts() {
                return *s_fonts;
            }

            static AutoReset<std::fs::path> s_customFontPath;
            void setCustomFontPath(const std::fs::path &path) {
                s_customFontPath = path;
            }

            static float s_fontSize = DefaultFontSize;
            void setFontSize(float size) {
                s_fontSize = size;
            }

            static AutoReset<ImFontAtlas*> s_fontAtlas;
            void setFontAtlas(ImFontAtlas* fontAtlas) {
                s_fontAtlas = fontAtlas;
            }

            static ImFont *s_boldFont = nullptr;
            static ImFont *s_italicFont = nullptr;
            void setFonts(ImFont *bold, ImFont *italic) {
                s_boldFont   = bold;
                s_italicFont = italic;
            }

            static SelectedFont s_selectedFont = SelectedFont::BuiltinPixelPerfect;
            static std::optional<CustomFont> s_customFont;
            void setCustomFont(const CustomFont &font) {
                s_customFont = font;
                s_selectedFont = SelectedFont::CustomFont;
            }

            void setSelectedFont(SelectedFont font) {
                s_selectedFont = font;
            }

            bool s_shouldLoadAllUnicodeCharacters = false;
            void setLoadAllUnicodeCharacters(bool enabled) {
                s_shouldLoadAllUnicodeCharacters = enabled;
            }

        }

        GlyphRange glyph(const char *glyph) {
            u32 codepoint;
            ImTextCharFromUtf8(&codepoint, glyph, nullptr);

            return {
                .begin = u16(codepoint),
                .end   = u16(codepoint)
            };
        }
        GlyphRange glyph(u32 codepoint) {
            return {
                .begin = u16(codepoint),
                .end   = u16(codepoint)
            };
        }
        GlyphRange range(const char *glyphBegin, const char *glyphEnd) {
            u32 codepointBegin, codepointEnd;
            ImTextCharFromUtf8(&codepointBegin, glyphBegin, nullptr);
            ImTextCharFromUtf8(&codepointEnd, glyphEnd, nullptr);

            return {
                .begin = u16(codepointBegin),
                .end   = u16(codepointEnd)
            };
        }

        GlyphRange range(u32 codepointBegin, u32 codepointEnd) {
            return {
                .begin = u16(codepointBegin),
                .end   = u16(codepointEnd)
            };
        }

        void loadFont(const std::fs::path &path, const std::vector<GlyphRange> &glyphRanges, Offset offset, u32 flags, std::optional<u32> defaultSize) {
            wolv::io::File fontFile(path, wolv::io::File::Mode::Read);
            if (!fontFile.isValid()) {
                log::error("Failed to load font from file '{}'", wolv::util::toUTF8String(path));
                return;
            }

            impl::s_fonts->emplace_back(Font {
                wolv::util::toUTF8String(path.filename()),
                fontFile.readVector(),
                glyphRanges,
                offset,
                flags,
                defaultSize
            });
        }

        void loadFont(const std::string &name, const std::span<const u8> &data, const std::vector<GlyphRange> &glyphRanges, Offset offset, u32 flags, std::optional<u32> defaultSize) {
            impl::s_fonts->emplace_back(Font {
                name,
                { data.begin(), data.end() },
                glyphRanges,
                offset,
                flags,
                defaultSize
            });
        }

        const std::fs::path& getCustomFontPath() {
            return impl::s_customFontPath;
        }

        float getFontSize() {
            return impl::s_fontSize;
        }

        ImFontAtlas* getFontAtlas() {
            return impl::s_fontAtlas;
        }

        ImFont* Bold() {
            return impl::s_boldFont;
        }

        ImFont* Italic() {
            return impl::s_italicFont;
        }

        std::optional<CustomFont> getCustomFont() {
            return impl::s_customFont;
        }

        SelectedFont getSelectedFont() {
            return impl::s_selectedFont;
        }

        bool shouldLoadAllUnicodeCharacters() {
            return impl::s_shouldLoadAllUnicodeCharacters;
        }

    }


}
