#pragma once

#include <fenestra.hpp>

#include <fenestra/helpers/fs.hpp>

#include <string>
#include <functional>
#include <map>
#include <optional>
#include <span>

using ImGuiID = unsigned int;
struct ImVec2;
struct ImFontAtlas;
struct ImFont;

namespace fene {

    namespace impl {
        class AutoResetBase;
    }

    namespace FenestraApi {

        /**
         * @brief Functions to interact with various Application system settings
         */
        namespace System {

            struct InitialWindowProperties {
                i32 x, y;
                u32 width, height;
                bool maximized;
            };

            enum class TaskProgressState {
                Reset,
                Progress,
                Flash
            };

            enum class TaskProgressType {
                Normal,
                Warning,
                Error
            };

            namespace impl {

                void addAutoResetObject(fene::impl::AutoResetBase *object);
                void cleanup();

                void setGPUVendor(const std::string &vendor);
                void setGLRenderer(const std::string &renderer);

                void setGlobalScale(float scale);
                void setNativeScale(float scale);

                void setBorderlessWindowMode(bool enabled);
                void setMultiWindowMode(bool enabled);


                void addInitArgument(const std::string &key, const std::string &value = { });

                void setMainInstanceStatus(bool status);
                void setInitialWindowProperties(InitialWindowProperties properties);

                void setMainWindowPosition(i32 x, i32 y);
                void setMainWindowSize(u32 width, u32 height);
                void setMainDockSpaceId(ImGuiID id);
                void setMainWindowHandle(void *window);

                void setLastFrameTime(double time);

                bool isWindowResizable();

                bool isDPIScalingEnabled();

            }

            double getGlobalScale();
            double getNativeScale();

            void closeApplication(bool noQuestions = false);

            bool isMainInstance();


            /**
             * @brief Gets the current target FPS
             * @return The current target FPS
             */
            float getTargetFPS();

            /**
             * @brief Gets the current GPU vendor
             * @return The current GPU vendor
             */
            const std::string& getGPUVendor();

            /**
             * @brief Gets the current GPU vendor
             * @return The current GPU vendor
             */
            const std::string& getGLRenderer();

            /**
             * @brief Checks if Application is running in portable mode
             * @return Whether Application is running in portable mode
             */
            bool isPortableVersion();

            /**
             * @brief Gets the current Operating System name
             * @return Operating System name
             */
            std::string getOSName();

            /**
             * @brief Gets the current Operating System version
             * @return Operating System version
             */
            std::string getOSVersion();

            /**
             * @brief Gets the current CPU architecture
             * @return CPU architecture
             */
            std::string getArchitecture();


            struct LinuxDistro {
                std::string name;
                std::string version;
            };
            /**
             * @brief Gets information related to the Linux distribution, if running on Linux
             */
            std::optional<LinuxDistro> getLinuxDistro();

            /**
             * @brief Gets the current Application version
             * @return Application version
             */
            std::string getApplicationVersion(bool withBuildType = true);

            /**
             * @brief Gets the current git commit hash
             * @param longHash Whether to return the full hash or the shortened version
             * @return Git commit hash
             */
            std::string getCommitHash(bool longHash = false);

            /**
             * @brief Gets the current git commit branch
             * @return Git commit branch
             */
            std::string getCommitBranch();

            /**
             * @brief Checks if Application was built in debug mode
             * @return True if Application was built in debug mode, false otherwise
             */
            bool isDebugBuild();

            /**
             * @brief Checks if this version of Application is a nightly build
             * @return True if this version is a nightly, false if it's a release
             */
            bool isNightlyBuild();

            /**
             * @brief Gets the current main window position
             * @return Position of the main window
             */
            ImVec2 getMainWindowPosition();

            /**
             * @brief Gets the current main window size
             * @return Size of the main window
             */
            ImVec2 getMainWindowSize();

            /**
             * @brief Gets the current main dock space ID
             * @return ID of the main dock space
             */
            ImGuiID getMainDockSpaceId();

            /**
             * @brief Gets the main window's backend window handle
             * @return Backend window handle
             */
            void* getMainWindowHandle();

            /**
             * @brief Checks if borderless window mode is enabled currently
             * @return Whether borderless window mode is enabled
             */
            bool isBorderlessWindowModeEnabled();

            /**
             * @brief Checks if multi-window mode is enabled currently
             * @return Whether multi-window mode is enabled
             */
            bool isMutliWindowModeEnabled();

            /**
             * @brief Gets the init arguments passed to the main Application from the splash screen
             * @return Init arguments
             */
            const std::map<std::string, std::string>& getInitArguments();

            /**
             * @brief Gets a init arguments passed to the main Application from the splash screen
             * @param key The key of the init argument
             * @return Init argument
            */
            std::string getInitArgument(const std::string &key);

            /**
             * @brief Gets the initial window properties
             * @return Initial window properties
             */
            std::optional<InitialWindowProperties> getInitialWindowProperties();

            /**
             * @brief Sets the progress bar in the task bar
             * @param state The state of the progress bar
             * @param type The type of the progress bar progress
             * @param progress The progress of the progress bar
             */
            void setTaskBarProgress(TaskProgressState state, TaskProgressType type, u32 progress);

            /**
             * @brief Sets if the Application should follow the system theme
             * @param enabled Whether to follow the system theme
             */
            void enableSystemThemeDetection(bool enabled);

            /**
             * @brief Checks if the Application follows the system theme
             * @return Whether the Application follows the system theme
             */
            bool usesSystemThemeDetection();

            /**
             * @brief Add a new startup task that will be run while the Application's splash screen is shown
             * @param name Name to be shown in the UI
             * @param async Whether to run the task asynchronously
             * @param function The function to run
             */
            void addStartupTask(const std::string &name, bool async, const std::function<bool()> &function);

        }

        /**
         * @brief Cross-instance messaging system
         * This allows you to send messages to the "main" instance of the application, from any other instance
         */
        namespace Messaging {

            namespace impl {

                using MessagingHandler = std::function<void(const std::vector<u8> &)>;

                const std::map<std::string, MessagingHandler>& getHandlers();
                void runHandler(const std::string &eventName, const std::vector<u8> &args);

            }

            /**
             * @brief Register the handler for this specific event name
             */
            void registerHandler(const std::string &eventName, const impl::MessagingHandler &handler);
        }

        namespace Fonts {

            struct GlyphRange { u16 begin, end; };
            struct Offset { float x, y; };

            struct Font {
                std::string name;
                std::vector<u8> fontData;
                std::vector<GlyphRange> glyphRanges;
                Offset offset;
                u32 flags;
                std::optional<u32> defaultSize;
            };

            namespace impl {

                const std::vector<Font>& getFonts();

                void setCustomFontPath(const std::fs::path &path);
                void setFontSize(float size);
                void setFontAtlas(ImFontAtlas *fontAtlas);

                void setFonts(ImFont *bold, ImFont *italic);

                void setLoadAllUnicodeCharacters(bool enabled);
            }

            GlyphRange glyph(const char *glyph);
            GlyphRange glyph(u32 codepoint);
            GlyphRange range(const char *glyphBegin, const char *glyphEnd);
            GlyphRange range(u32 codepointBegin, u32 codepointEnd);

            void loadFont(const std::fs::path &path, const std::vector<GlyphRange> &glyphRanges = {}, Offset offset = {}, u32 flags = 0, std::optional<u32> defaultSize = std::nullopt);
            void loadFont(const std::string &name, const std::span<const u8> &data, const std::vector<GlyphRange> &glyphRanges = {}, Offset offset = {}, u32 flags = 0, std::optional<u32> defaultSize = std::nullopt);

            constexpr static float DefaultFontSize = 13.0;

            ImFont* Bold();
            ImFont* Italic();

            enum class SelectedFont {
                BuiltinPixelPerfect,
                BuiltinSmooth,
                CustomFont,
                SystemFont
            };

            struct CustomFont {
                std::filesystem::path fontPath;
                bool bold, italic, antiAliased;
            };

            SelectedFont getSelectedFont();

            /**
             * @brief Gets the current custom font path
             * @return The current custom font path
             */
            std::optional<CustomFont> getCustomFont();

            /**
             * @brief Gets the current font size
             * @return The current font size
             */
            float getFontSize();

            /**
             * @brief Gets the current font atlas
             * @return Current font atlas
             */
            ImFontAtlas* getFontAtlas();

            bool shouldLoadAllUnicodeCharacters();
            bool shouldUsePixelPerfectDefaultFont();

        }

    }

}