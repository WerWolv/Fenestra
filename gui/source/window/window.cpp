#include "window.hpp"

#include <fenestra.hpp>

#include <fenestra/api/plugin_manager.hpp>
#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/api/layout_manager.hpp>
#include <fenestra/api/shortcut_manager.hpp>
#include <fenestra/api/workspace_manager.hpp>
#include <fenestra/api/tutorial_manager.hpp>
#include <fenestra/api/theme_manager.hpp>

#include <fenestra/events/system_events.hpp>
#include <fenestra/events/ui_events.hpp>
#include <fenestra/events/fenestra_events.hpp>

#include <fenestra/helpers/utils.hpp>
#include <fenestra/helpers/fs.hpp>
#include <fenestra/helpers/logger.hpp>
#include <fenestra/helpers/default_paths.hpp>

#include <fenestra/ui/view.hpp>
#include <fenestra/ui/popup.hpp>

#include <chrono>
#include <cstring>
#include <ranges>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>
#include <fenestra/ui/imgui_extensions.h>
#include <implot.h>
#include <implot_internal.h>
#include <imnodes.h>
#include <imnodes_internal.h>

#include <wolv/utils/string.hpp>

#include <fenestra/ui/toast.hpp>
#include <wolv/utils/guards.hpp>
#include <fmt/printf.h>
#include <fenestra/api/interface_registry.hpp>
#include <fenestra/api/task_manager.hpp>

namespace fene {

    using namespace std::literals::chrono_literals;

    Window::Window() {
        const static auto openEmergencyPopup = [this](const std::string &title){
            TaskManager::doLater([this, title] {
                ImGui::OpenPopup(title.c_str());
                m_emergencyPopupOpen = true;
            });
        };

        // Handle fatal error popups for errors detected during initialization
        {
            for (const auto &[argument, value] : FenestraApi::System::getInitArguments()) {
                if (argument == "no-plugins") {
                    openEmergencyPopup("No Plugins");
                } else if (argument == "duplicate-plugins") {
                    openEmergencyPopup("Duplicate Plugins loaded");
                }
            }
        }

        // Initialize the window
        this->initBackend();
        this->initImGui();
        this->setupNativeWindow();
        this->registerEventHandlers();

        EventWindowInitialized::post();
        EventApplicationStartupFinished::post();

        TutorialManager::init();
    }

    Window::~Window() {
        RequestCloseApplication::unsubscribe(this);
        RequestUpdateWindowTitle::unsubscribe(this);
        EventAbnormalTermination::unsubscribe(this);
        RequestOpenPopup::unsubscribe(this);

        EventWindowDeinitializing::post(m_window);

        this->exitImGui();
        this->exitBackend();
    }

    void Window::registerEventHandlers() {
        // Initialize default theme
        RequestChangeTheme::post("Dark");

        // Handle the close window request by telling the backend to shut down
        RequestCloseApplication::subscribe(this, [this](bool noQuestions) {
            m_windowOpen = false;

            if (!noQuestions)
                EventWindowClosing::post(m_window);
        });

        // Handle opening popups
        RequestOpenPopup::subscribe(this, [this](auto name) {
            std::scoped_lock lock(m_popupMutex);

            m_popupsToOpen.push_back(name);
        });

        EventDPIChanged::subscribe([this](float oldScaling, float newScaling) {
            int width, height;
            SDL_GetWindowSize(m_window, &width, &height);

            width = float(width) * newScaling / oldScaling;
            height = float(height) * newScaling / oldScaling;

            FenestraApi::System::impl::setMainWindowSize(width, height);
            SDL_SetWindowSize(m_window, width, height);
        });

        EventBackendEventFired::subscribe(this, [this](const SDL_Event *event) {
            switch (event->type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    m_windowOpen = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    if (event->window.windowID == SDL_GetWindowID(m_window))
                        FenestraApi::System::impl::setMainWindowSize(event->window.data1, event->window.data2);
                    break;
                case SDL_EVENT_WINDOW_MOVED:
                    if (event->window.windowID == SDL_GetWindowID(m_window))
                        FenestraApi::System::impl::setMainWindowPosition(event->window.data1, event->window.data2);
                    break;
                case SDL_EVENT_WINDOW_EXPOSED:
                    this->fullFrame();
                    break;

                case SDL_EVENT_SYSTEM_THEME_CHANGED:
                    EventOSThemeChanged::post();
                    break;

                case SDL_EVENT_DROP_BEGIN:
                    EventFileDragged::post(true);
                    break;
                case SDL_EVENT_DROP_COMPLETE:
                    EventFileDragged::post(false);
                    break;
                case SDL_EVENT_DROP_FILE:
                    EventFileDropped::post(event->drop.data);
                    break;

                case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
                    int interfaceScaleSetting = FenestraApi::System::impl::isDPIScalingEnabled();
                    if (interfaceScaleSetting != 0)
                        break;

                    const auto newScale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(m_window));
                    const auto oldScale = FenestraApi::System::getNativeScale();

                    if (newScale == oldScale || newScale == 0 || oldScale == 0)
                        break;

                    EventDPIChanged::post(oldScale, newScale);
                    FenestraApi::System::impl::setNativeScale(newScale);

                    ThemeManager::reapplyCurrentTheme();
                    ImGui::GetStyle().ScaleAllSizes(newScale);
                }

                default:
                    break;
            }
        });

        SDL_SetLogOutputFunction([](void *, int, SDL_LogPriority, const char *message) {
            log::error("{}", message);
        }, nullptr);


        LayoutManager::registerLoadCallback([this](std::string_view line) {
            int width = 0, height = 0;
                sscanf(line.data(), "MainWindowSize=%d,%d", &width, &height);

                if (width > 0 && height > 0) {
                    TaskManager::doLater([width, height, this]{
                        SDL_SetWindowSize(m_window, width, height);
                    });
                }
        });
    }

    void handleException() {
        try {
            throw;
        } catch (const std::exception &e) {
            log::fatal("Unhandled exception: {}", e.what());
            EventCrashRecovered::post(e);
        } catch (...) {
            log::fatal("Unhandled exception: Unknown exception");
        }
    }

    void errorRecoverLogCallback(void*, const char* fmt, ...) {
        va_list args;

        std::string message;

        va_start(args, fmt);
        message.resize(std::vsnprintf(nullptr, 0, fmt, args));
        va_end(args);

        va_start(args, fmt);
        std::vsnprintf(message.data(), message.size(), fmt, args);
        va_end(args);

        message.resize(message.size() - 1);

        log::error("{}", message);
    }

    void Window::fullFrame() {
        [[maybe_unused]] static u32 crashWatchdog = 0;

        if (auto g = ImGui::GetCurrentContext(); g == nullptr || g->WithinFrameScope) {
            return;
        }

        #if !defined(DEBUG)
        try {
        #endif

            // Render an entire frame
            this->frameBegin();
            this->frame();
            this->frameEnd();

        #if !defined(DEBUG)
            // Feed the watchdog
            crashWatchdog = 0;
        } catch (...) {
            // If an exception keeps being thrown, abort the application after 10 frames
            // This is done to avoid the application getting stuck in an infinite loop of exceptions
            crashWatchdog += 1;
            if (crashWatchdog > 10) {
                log::fatal("Crash watchdog triggered, aborting");
                std::abort();
            }

            // Try to recover from the exception by bringing ImGui back into a working state
            ImGui::ErrorCheckEndFrameRecover(errorRecoverLogCallback, nullptr);
            ImGui::EndFrame();
            ImGui::UpdatePlatformWindows();

            // Handle the exception
            handleException();
        }
        #endif
    }

    void Window::loop() {
        SDL_ShowWindow(m_window);

        SDL_AddEventWatch([](void *userData, SDL_Event *event) -> SDL_bool {
            auto window = static_cast<Window*>(userData);
            if (!window->m_windowOpen)
                return SDL_FALSE;

            EventBackendEventFired::post(event);
            ImGui_ImplSDL3_ProcessEvent(event);

            return SDL_TRUE;
        }, this);

        while (m_windowOpen) {
            m_lastStartFrameTime = SDL_GetTicks();

            {
                int x = 0, y = 0;
                int width = 0, height = 0;
                SDL_GetWindowPosition(m_window, &x, &y);
                SDL_GetWindowSize(m_window, &width, &height);

                FenestraApi::System::impl::setMainWindowPosition(x, y);
                FenestraApi::System::impl::setMainWindowSize(width, height);
            }

            // Determine if the application should be in long sleep mode
            bool shouldLongSleep = !m_unlockFrameRate;

            static double lockTimeout = 0;
            if (!shouldLongSleep) {
                lockTimeout = 0.05;
            } else if (lockTimeout > 0) {
                lockTimeout -= m_lastFrameTime;
            }

            if (shouldLongSleep && lockTimeout > 0)
                shouldLongSleep = false;

            m_unlockFrameRate = false;

            SDL_Event event;
            while (SDL_PollEvent(&event)) { }

            m_lastStartFrameTime = SDL_GetTicks();

            static ImVec2 lastWindowSize = FenestraApi::System::getMainWindowSize();
            if (FenestraApi::System::impl::isWindowResizable()) {
                lastWindowSize = FenestraApi::System::getMainWindowSize();
            } else {
            }

            this->fullFrame();

            FenestraApi::System::impl::setLastFrameTime(SDL_GetTicks() - m_lastStartFrameTime);
        }

        // Hide the window as soon as the render loop exits to make the window
        // disappear as soon as it's closed
        SDL_HideWindow(m_window);
    }

    void Window::drawTitleBar() {
    }

    void Window::frameBegin() {
        // Start new ImGui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        EventFrameBegin::post();

        // Handle all undocked floating windows
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(FenestraApi::System::getMainWindowSize() - ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (!m_emergencyPopupOpen)
            windowFlags |= ImGuiWindowFlags_MenuBar;

        // Render main dock space
        if (ImGui::Begin("FenestraDockSpace", nullptr, windowFlags)) {
            ImGui::PopStyleVar();

            this->beginNativeWindowFrame();
        } else {
            ImGui::PopStyleVar();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        // Plugin load error popups
        // These are not translated because they should always be readable, no matter if any localization could be loaded or not
        {
            const static auto drawPluginFolderTable = [] {
                ImGuiExt::UnderlinedText("Plugin folders");
                if (ImGui::BeginTable("plugins", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 100_scaled))) {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 0.2);
                    ImGui::TableSetupColumn("Exists", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() * 3);

                    ImGui::TableHeadersRow();

                    for (const auto &path : paths::Plugins.all()) {
                        const auto filePath = path / "builtin.hexplug";
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(wolv::util::toUTF8String(filePath).c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(wolv::io::fs::exists(filePath) ? "Yes" : "No");
                    }
                    ImGui::EndTable();
                }
            };

            if (m_emergencyPopupOpen) {
                const auto pos = FenestraApi::System::getMainWindowPosition();
                const auto size = FenestraApi::System::getMainWindowSize();
                ImGui::GetBackgroundDrawList()->AddRectFilled(pos, pos + size, ImGui::GetColorU32(ImGuiCol_WindowBg) | 0xFF000000);
            }

            ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, 0x00);
            ON_SCOPE_EXIT { ImGui::PopStyleColor(); };

            // No plugins error popup
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5F, 0.5F));
            if (ImGui::BeginPopupModal("No Plugins", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindowRead());
                ImGui::TextUnformatted("No plugins loaded (including the built-in plugin)!");
                ImGui::TextUnformatted("Make sure you installed the application correctly.");
                ImGui::TextUnformatted("There should be at least a 'builtin.hexplug' file in your plugins folder.");

                ImGui::NewLine();

                drawPluginFolderTable();

                ImGui::NewLine();
                if (ImGuiExt::DimmedButton("Close Application", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    FenestraApi::System::closeApplication(true);

                ImGui::EndPopup();
            }

            // Duplicate plugins error popup
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5F, 0.5F));
            if (ImGui::BeginPopupModal("Duplicate Plugins loaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindowRead());
                ImGui::TextUnformatted("Application found and attempted to load multiple plugins with the same name!");
                ImGui::TextUnformatted("Make sure you installed the application correctly and, if needed,");
                ImGui::TextUnformatted("cleaned up older installations correctly.");
                ImGui::TextUnformatted("Each plugin should only ever be loaded once.");

                ImGui::NewLine();

                drawPluginFolderTable();

                ImGui::NewLine();
                if (ImGuiExt::DimmedButton("Close Application", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    FenestraApi::System::closeApplication(true);

                ImGui::EndPopup();
            }
        }

        // Open popups when plugins requested it
        {
            std::scoped_lock lock(m_popupMutex);
            m_popupsToOpen.remove_if([](const auto &name) {
                if (ImGui::IsPopupOpen(name.c_str()))
                    return true;
                else
                    ImGui::OpenPopup(name.c_str());

                return false;
            });
        }

        // Draw popup stack
        {
            static bool positionSet = false;
            static bool sizeSet = false;
            static double popupDelay = -2.0;
            static u32 displayFrameCount = 0;

            static std::unique_ptr<impl::PopupBase> currPopup;
            static Lang name("");

            AT_FIRST_TIME {
                EventApplicationClosing::subscribe([] {
                    currPopup.reset();
                });
            };

            if (auto &popups = impl::PopupBase::getOpenPopups(); !popups.empty()) {
                if (!ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopupId)) {
                    if (popupDelay <= -1.0) {
                        popupDelay = 0.2;
                    } else {
                        popupDelay -= m_lastFrameTime;
                        if (popupDelay < 0 || popups.size() == 1) {
                            popupDelay = -2.0;
                            currPopup = std::move(popups.back());
                            name = Lang(currPopup->getUnlocalizedName());
                            displayFrameCount = 0;

                            ImGui::OpenPopup(name);
                            popups.pop_back();
                        }
                    }
                }
            }

            if (currPopup != nullptr) {
                bool open = true;

                const auto &minSize = currPopup->getMinSize();
                const auto &maxSize = currPopup->getMaxSize();
                const bool hasConstraints = minSize.x != 0 && minSize.y != 0 && maxSize.x != 0 && maxSize.y != 0;

                if (hasConstraints)
                    ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
                else
                    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Appearing);

                auto* closeButton = currPopup->hasCloseButton() ? &open : nullptr;

                const auto flags = currPopup->getFlags() | (!hasConstraints ? (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize) : ImGuiWindowFlags_None);

                if (!positionSet) {
                    ImGui::SetNextWindowPos(FenestraApi::System::getMainWindowPosition() + (FenestraApi::System::getMainWindowSize() / 2.0F), ImGuiCond_Always, ImVec2(0.5F, 0.5F));

                    if (sizeSet)
                        positionSet = true;
                }

                const auto createPopup = [&](bool displaying) {
                    if (displaying) {
                        displayFrameCount += 1;
                        currPopup->drawContent();

                        if (ImGui::GetWindowSize().x > ImGui::GetStyle().FramePadding.x * 10)
                            sizeSet = true;

                        // Reset popup position if it's outside the main window when multi-viewport is not enabled
                        // If not done, the popup will be stuck outside the main window and cannot be accessed anymore
                        if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == ImGuiConfigFlags_None) {
                            const auto currWindowPos = ImGui::GetWindowPos();
                            const auto minWindowPos = FenestraApi::System::getMainWindowPosition() - ImGui::GetWindowSize();
                            const auto maxWindowPos = FenestraApi::System::getMainWindowPosition() + FenestraApi::System::getMainWindowSize();
                            if (currWindowPos.x > maxWindowPos.x || currWindowPos.y > maxWindowPos.y || currWindowPos.x < minWindowPos.x || currWindowPos.y < minWindowPos.y) {
                                positionSet = false;
                                GImGui->MovingWindow = nullptr;
                            }
                        }

                        ImGui::EndPopup();
                    }
                };

                if (currPopup->isModal())
                    createPopup(ImGui::BeginPopupModal(name, closeButton, flags));
                else
                    createPopup(ImGui::BeginPopup(name, flags));

                if (!ImGui::IsPopupOpen(name) && displayFrameCount < 5) {
                    ImGui::OpenPopup(name);
                }

                if (currPopup->shouldClose() || !open) {
                    log::debug("Closing popup '{}'", name);
                    positionSet = sizeSet = false;

                    currPopup = nullptr;
                }
            }
        }

        // Draw Toasts
        {
            u32 index = 0;
            for (const auto &toast : impl::ToastBase::getQueuedToasts() | std::views::take(4)) {
                const auto toastHeight = 60_scaled;
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5_scaled);
                ImGui::SetNextWindowSize(ImVec2(280_scaled, toastHeight));
                ImGui::SetNextWindowPos((FenestraApi::System::getMainWindowPosition() + FenestraApi::System::getMainWindowSize()) - scaled({ 10, 10 }) - scaled({ 0, (10 + toastHeight) * index }), ImGuiCond_Always, ImVec2(1, 1));
                if (ImGui::Begin(fmt::format("##Toast_{}", index).c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing)) {
                    auto drawList = ImGui::GetWindowDrawList();

                    const auto min = ImGui::GetWindowPos();
                    const auto max = min + ImGui::GetWindowSize();

                    drawList->PushClipRect(min, min + scaled({ 5, 60 }));
                    drawList->AddRectFilled(min, max, toast->getColor(), 5_scaled);
                    drawList->PopClipRect();

                    ImGui::Indent();
                    toast->draw();
                    ImGui::Unindent();

                    if (ImGui::IsWindowHovered() || toast->getAppearTime() <= 0)
                        toast->setAppearTime(ImGui::GetTime());
                }
                ImGui::End();
                ImGui::PopStyleVar();

                index += 1;
            }

            std::erase_if(impl::ToastBase::getQueuedToasts(), [](const auto &toast){
                return toast->getAppearTime() > 0 && (toast->getAppearTime() + impl::ToastBase::VisibilityTime) < ImGui::GetTime();
            });
        }

        // Run all deferred calls
        TaskManager::runDeferredCalls();
    }

    void Window::frame() {
        auto &io = ImGui::GetIO();

        // Loop through all views and draw them
        for (auto &view : InterfaceRegistry::impl::getOpenViews()) {
            ImGui::GetCurrentContext()->NextWindowData.ClearFlags();

            // Skip views that shouldn't be processed currently
            if (!view->shouldProcess())
                continue;

            const auto openViewCount = std::ranges::count_if(InterfaceRegistry::impl::getOpenViews(), [](const auto &entry) {
                const auto &view = entry;

                return view->hasViewMenuItemEntry() && view->shouldProcess();
            });

            ImGuiWindowClass windowClass = {};

            windowClass.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoCloseButton;

            if (openViewCount <= 1 || LayoutManager::isLayoutLocked())
                windowClass.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoTabBar;

            ImGui::SetNextWindowClass(&windowClass);

            auto window    = ImGui::FindWindowByName(view->getName().c_str());
            if (window != nullptr && window->DockNode == nullptr)
                ImGui::SetNextWindowBgAlpha(1.0F);

            // Draw view
            view->draw();

            bool focused = false;

            // Get the currently focused view
            if (window != nullptr && (window->Flags & ImGuiWindowFlags_Popup) != ImGuiWindowFlags_Popup) {
                const auto windowName = view->getName();
                ImGui::Begin(windowName.c_str());

                // Detect if the window is focused
                focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_NoPopupHierarchy);

                // Dock the window if it's not already docked
                if (view->didWindowJustOpen() && !ImGui::IsWindowDocked()) {
                    ImGui::DockBuilderDockWindow(windowName.c_str(), FenestraApi::System::getMainDockSpaceId());
                    EventViewOpened::post(view.get());
                }

                ImGui::End();
            }

            // Pass on currently pressed keys to the shortcut handler
            for (const auto &key : m_pressedKeys) {
                ShortcutManager::process(view.get(), io.KeyCtrl, io.KeyAlt, io.KeyShift, io.KeySuper, focused, key);
            }
        }

        // Handle global shortcuts
        for (const auto &key : m_pressedKeys) {
            ShortcutManager::processGlobals(io.KeyCtrl, io.KeyAlt, io.KeyShift, io.KeySuper, key);
        }

        m_pressedKeys.clear();
    }

    void Window::frameEnd() {
        EventFrameEnd::post();

        TutorialManager::drawTutorial();

        // Clean up all tasks that are done
        TaskManager::collectGarbage();

        this->endNativeWindowFrame();

        ImGui::ErrorCheckEndFrameRecover(errorRecoverLogCallback, nullptr);

        // Finalize ImGui frame
        ImGui::Render();

        // Compare the previous frame buffer to the current one to determine if the window content has changed
        // If not, there's no point in sending the draw data off to the GPU and swapping buffers
        // NOTE: For anybody looking at this code and thinking "why not just hash the buffer and compare the hashes",
        // the reason is that hashing the buffer is significantly slower than just comparing the buffers directly.
        // The buffer might become quite large if there's a lot of vertices on the screen but it's still usually less than
        // 10MB (out of which only the active portion needs to actually be compared) which is worth the ~60x speedup.
        bool shouldRender = false;
        {
            static std::vector<u8> previousVtxData;
            static size_t previousVtxDataSize = 0;

            size_t offset = 0;
            size_t vtxDataSize = 0;

            for (const auto viewPort : ImGui::GetPlatformIO().Viewports) {
                auto drawData = viewPort->DrawData;
                for (int n = 0; n < drawData->CmdListsCount; n++) {
                    vtxDataSize += drawData->CmdLists[n]->VtxBuffer.size() * sizeof(ImDrawVert);
                }
            }
            for (const auto viewPort : ImGui::GetPlatformIO().Viewports) {
                auto drawData = viewPort->DrawData;
                for (int n = 0; n < drawData->CmdListsCount; n++) {
                    const ImDrawList *cmdList = drawData->CmdLists[n];

                    if (vtxDataSize == previousVtxDataSize) {
                        shouldRender = shouldRender || std::memcmp(previousVtxData.data() + offset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.size() * sizeof(ImDrawVert)) != 0;
                    } else {
                        shouldRender = true;
                    }

                    if (previousVtxData.size() < offset + cmdList->VtxBuffer.size() * sizeof(ImDrawVert)) {
                        previousVtxData.resize(offset + cmdList->VtxBuffer.size() * sizeof(ImDrawVert));
                    }

                    std::memcpy(previousVtxData.data() + offset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.size() * sizeof(ImDrawVert));
                    offset += cmdList->VtxBuffer.size() * sizeof(ImDrawVert);
                }
            }

            previousVtxDataSize = vtxDataSize;
        }

        auto backupContext = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(m_window, backupContext);

        if (shouldRender) {
            auto* drawData = ImGui::GetDrawData();
            
            // Avoid accidentally clearing the viewport when the application is minimized,
            // otherwise the OS will display an empty frame during deminimization on macOS
            if (drawData->DisplaySize.x != 0 && drawData->DisplaySize.y != 0) {
                int displayWidth, displayHeight;
                SDL_GetWindowSizeInPixels(m_window, &displayWidth, &displayHeight);
                glViewport(0, 0, displayWidth, displayHeight);
                glClearColor(0.00F, 0.00F, 0.00F, 0.00F);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                SDL_GL_SwapWindow(m_window);
            }

            m_unlockFrameRate = true;
        }

        // Process layout load requests
        // NOTE: This needs to be done before a new frame is started, otherwise ImGui won't handle docking correctly
        LayoutManager::process();
        WorkspaceManager::process();
    }

    void Window::initBackend() {
        auto initialWindowProperties = FenestraApi::System::getInitialWindowProperties();

        if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
            log::fatal("Failed to initialize SDL3!");
            log::fatal("Error: {}", SDL_GetError());
            std::abort();
        }

        configureBackend();

        // Create window

        auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (initialWindowProperties.has_value()) {
            flags |= SDL_WINDOW_MAXIMIZED;
        }

        m_windowTitle = FENESTRA_APPLICATION_NAME;
        m_window = SDL_CreateWindow(m_windowTitle.c_str(), 1280_scaled, 720_scaled, flags);

        m_glContext = SDL_GL_CreateContext(m_window);
        SDL_GL_MakeCurrent(m_window, m_glContext);

        FenestraApi::System::impl::setMainWindowHandle(m_window);

        if (m_window == nullptr) {
            log::fatal("Failed to create window!");
            std::abort();
        }

        // Force window to be fully opaque by default
        SDL_SetWindowOpacity(m_window, 1.0F);


        // Disable VSync. Not like any graphics driver actually cares
        SDL_GL_SwapWindow(m_window);

        // Center window
        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        // Set up initial window position
        {
            int x = 0, y = 0;
            SDL_GetWindowPosition(m_window, &x, &y);

            if (initialWindowProperties.has_value()) {
                x = initialWindowProperties->x;
                y = initialWindowProperties->y;
            }

            FenestraApi::System::impl::setMainWindowPosition(x, y);
            SDL_SetWindowPosition(m_window, x, y);
        }

        // Set up initial window size
        {
            int width = 0, height = 0;
            SDL_GetWindowSize(m_window, &width, &height);

            if (initialWindowProperties.has_value()) {
                width  = initialWindowProperties->width;
                height = initialWindowProperties->height;
            }

            FenestraApi::System::impl::setMainWindowSize(width, height);
            SDL_SetWindowSize(m_window, width, height);
        }
    }

    void Window::resize(i32 width, i32 height) {
        SDL_SetWindowSize(m_window, width, height);
    }

    void Window::initImGui() {
        IMGUI_CHECKVERSION();

        auto fonts = FenestraApi::Fonts::getFontAtlas();

        if (fonts == nullptr) {
            fonts = IM_NEW(ImFontAtlas)();

            fonts->AddFontDefault();
            fonts->Build();
        }

        // Initialize ImGui and all other ImGui extensions
        GImGui   = ImGui::CreateContext(fonts);
        GImPlot  = ImPlot::CreateContext();
        GImNodes = ImNodes::CreateContext();

        ImGuiIO &io       = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();

        ImNodes::GetStyle().Flags = ImNodesStyleFlags_NodeOutline | ImNodesStyleFlags_GridLines;

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        if (FenestraApi::System::isMutliWindowModeEnabled())
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.ConfigViewportsNoTaskBarIcon = false;

        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);

        // Allow ImNodes links to always be detached without holding down any button
        {
            static bool always = true;
            ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &always;
        }

        io.UserData = &m_imguiCustomData;

        const auto scale = float(FenestraApi::System::getGlobalScale());
        style.ScaleAllSizes(scale);
        io.DisplayFramebufferScale = ImVec2(scale, scale);
        io.Fonts->SetTexID(fonts->TexID);

        #if defined(OS_MACOS)
            io.FontGlobalScale = 1.0F / 2.0F;
        #else
            io.FontGlobalScale = 1.0F;
        #endif


        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.IndentSpacing            = 10.0F;
        style.DisplaySafeAreaPadding  = ImVec2(0.0F, 0.0F);

        style.Colors[ImGuiCol_TabSelectedOverline]          = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);
        style.Colors[ImGuiCol_TabDimmedSelectedOverline]    = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);

        style.Colors[ImGuiCol_TitleBg]             = style.Colors[ImGuiCol_MenuBarBg];
        style.Colors[ImGuiCol_TitleBgActive]       = style.Colors[ImGuiCol_MenuBarBg];
        style.Colors[ImGuiCol_TitleBgCollapsed]    = style.Colors[ImGuiCol_MenuBarBg];

        // Install custom settings handler
        {
            ImGuiSettingsHandler handler;
            handler.TypeName   = FENESTRA_APPLICATION_NAME;
            handler.TypeHash   = ImHashStr(handler.TypeName);

            handler.ReadOpenFn = [](ImGuiContext *ctx, ImGuiSettingsHandler *, const char *) -> void* { return ctx; };

            handler.ReadLineFn = [](ImGuiContext *, ImGuiSettingsHandler *, void *, const char *line) {
                LayoutManager::onLoad(line);
            };

            handler.WriteAllFn = [](ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buffer) {
                buffer->appendf("[%s][General]\n", handler->TypeName);
                LayoutManager::onStore(buffer);
                buffer->append("\n");
            };

            handler.UserData   = this;

            auto context = ImGui::GetCurrentContext();
            context->SettingsHandlers.push_back(handler);
            context->TestEngineHookItems = true;

            io.IniFilename = nullptr;
        }


        ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);

        #if defined(OS_MACOS)
            ImGui_ImplOpenGL3_Init("#version 150");
        #elif defined(OS_WEB)
            ImGui_ImplOpenGL3_Init();
        #else
            ImGui_ImplOpenGL3_Init("#version 130");
        #endif

        for (const auto &plugin : PluginManager::getPlugins())
            plugin.setImGuiContext(ImGui::GetCurrentContext());

        RequestInitThemeHandlers::post();
    }

    void Window::exitBackend() {
        SDL_DestroyWindow(m_window);
        SDL_Quit();

        m_window = nullptr;
    }

    void Window::exitImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

}
