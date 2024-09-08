#include <fenestra/api/interface_registry.hpp>
#include <fenestra/api/event_manager.hpp>
#include <fenestra/api/shortcut_manager.hpp>
#include <fenestra/api/task_manager.hpp>

#include <fenestra/ui/view.hpp>
#include <fenestra/helpers/utils.hpp>
#include <fenestra/events/ui_events.hpp>
#include <fenestra/events/fenestra_events.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <fenestra/ui/imgui_extensions.h>

#include <romfs/romfs.hpp>
#include <wolv/utils/guards.hpp>

#include <ranges>

#include <SDL3/SDL.h>

#include <fonts/codicons_font.h>

namespace fene::plugin::bundled {

    // Function that draws the provider popup, defined in the ui_items.cpp file
    void drawProviderTooltip(const prv::Provider *provider);

    namespace {

        std::string s_windowTitle, s_windowTitleFull;
        u32 s_searchBarPosition = 0;
        [[maybe_unused]] ImGuiExt::Texture s_logoTexture;

        bool s_showSearchBar = true;

        void createNestedMenu(std::span<const UnlocalizedString> menuItems, const char *icon, const Shortcut &shortcut, const InterfaceRegistry::impl::MenuCallback &callback, const InterfaceRegistry::impl::EnabledCallback &enabledCallback, const InterfaceRegistry::impl::SelectedCallback &selectedCallback) {
            const auto &name = menuItems.front();

            if (name.get() == InterfaceRegistry::impl::SeparatorValue) {
                ImGui::Separator();
                return;
            }

            if (name.get() == InterfaceRegistry::impl::SubMenuValue) {
                callback();
            } else if (menuItems.size() == 1) {
                if (ImGui::MenuItemEx(Lang(name), icon, shortcut.toString().c_str(), selectedCallback(), enabledCallback()))
                    callback();
            } else {
                bool isSubmenu = (menuItems.begin() + 1)->get() == InterfaceRegistry::impl::SubMenuValue;

                if (ImGui::BeginMenuEx(Lang(name), std::next(menuItems.begin())->get() == InterfaceRegistry::impl::SubMenuValue ? icon : nullptr, isSubmenu ? enabledCallback() : true)) {
                    createNestedMenu({ std::next(menuItems.begin()), menuItems.end() }, icon, shortcut, callback, enabledCallback, selectedCallback);
                    ImGui::EndMenu();
                }
            }
        }

        void drawFooter(ImDrawList *drawList, ImVec2 dockSpaceSize) {
            auto dockId = ImGui::DockSpace(ImGui::GetID("FenestraMainDock"), dockSpaceSize);
            FenestraManager::System::impl::setMainDockSpaceId(dockId);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y - 1_scaled);
            ImGui::Separator();
            ImGui::SetCursorPosX(8);
            for (const auto &callback : InterfaceRegistry::impl::getFooterItems()) {
                const auto y = ImGui::GetCursorPosY();
                const auto prevIdx = drawList->_VtxCurrentIdx;
                callback();
                const auto currIdx = drawList->_VtxCurrentIdx;
                ImGui::SetCursorPosY(y);

                // Only draw separator if something was actually drawn
                if (prevIdx != currIdx) {
                    ImGui::SameLine();
                    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                    ImGui::SameLine();
                }
            }
        }

        void drawSidebar(ImVec2 dockSpaceSize, ImVec2 sidebarPos, float sidebarWidth) {
            static i32 openWindow = -1;
            i32 index = 0;
            u32 drawIndex = 0;
            ImGui::PushID("SideBarWindows");
            for (const auto &[icon, callback, enabledCallback] : InterfaceRegistry::impl::getSidebarItems()) {
                ImGui::SetCursorPosY(sidebarPos.y + sidebarWidth * float(drawIndex));

                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));


                if (enabledCallback()) {
                    drawIndex += 1;
                    {
                        if (ImGui::Button(icon.c_str(), ImVec2(sidebarWidth, sidebarWidth))) {
                            if (openWindow == index)
                                openWindow = -1;
                            else
                                openWindow = index;
                        }
                    }
                }

                ImGui::PopStyleColor(3);

                auto sideBarFocused = ImGui::IsWindowFocused();

                bool open = openWindow == index;
                if (open) {
                    ImGui::SetNextWindowPos(ImGui::GetWindowPos() + sidebarPos + ImVec2(sidebarWidth - 1_scaled, -1_scaled));
                    ImGui::SetNextWindowSizeConstraints(ImVec2(0, dockSpaceSize.y + 5_scaled), ImVec2(FLT_MAX, dockSpaceSize.y + 5_scaled));

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
                    ImGui::PushStyleColor(ImGuiCol_WindowShadow, 0x00000000);
                    if (ImGui::Begin("SideBarWindow", &open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                        if (ImGui::BeginChild("##Content", ImGui::GetContentRegionAvail())) {
                            callback();
                        }
                        ImGui::EndChild();

                        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !sideBarFocused) {
                            openWindow = -1;
                        }
                    }
                    ImGui::End();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }

                ImGui::NewLine();
                index += 1;
            }
            ImGui::PopID();
        }

        void drawTitleBar() {
            auto titleBarHeight = ImGui::GetCurrentWindowRead()->MenuBarHeight;

            #if defined (OS_MACOS)
                titleBarHeight *= 0.7F;
            #endif

            auto buttonSize = ImVec2(titleBarHeight * 1.5F, titleBarHeight - 1);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));

            const auto windowSize = ImGui::GetWindowSize();
            auto searchBoxSize = ImVec2(s_showSearchBar ? windowSize.x / 2.5 : ImGui::CalcTextSize(s_windowTitle.c_str()).x, titleBarHeight);
            auto searchBoxPos = ImVec2((windowSize / 2 - searchBoxSize / 2).x, 0);
            auto titleBarButtonPosY = 0.0F;

            #if defined(OS_MACOS)
                searchBoxPos.y = ImGui::GetStyle().FramePadding.y * 2;
                titleBarButtonPosY = searchBoxPos.y;
            #else
                titleBarButtonPosY = 0;

                if (s_showSearchBar) {
                    searchBoxPos.y = 3_scaled;
                    searchBoxSize.y -= 3_scaled;
                }
            #endif

            s_searchBarPosition = searchBoxPos.x;

            // Custom title bar buttons implementation for borderless window mode
            [[maybe_unused]] auto window = static_cast<SDL_Window*>(FenestraManager::System::getMainWindowHandle());
            bool titleBarButtonsVisible = false;
            if (FenestraManager::System::isBorderlessWindowModeEnabled()) {
                #if defined(OS_WINDOWS)
                    titleBarButtonsVisible = true;

                    // Draw minimize, restore and maximize buttons
                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonSize.x * 3);
                    if (ImGuiExt::TitleBarButton(ICON_VS_CHROME_MINIMIZE, buttonSize))
                        SDL_MinimizeWindow(window);
                    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) {
                        if (ImGuiExt::TitleBarButton(ICON_VS_CHROME_RESTORE, buttonSize))
                            SDL_RestoreWindow(window);
                    } else {
                        if (ImGuiExt::TitleBarButton(ICON_VS_CHROME_MAXIMIZE, buttonSize))
                            SDL_MaximizeWindow(window);
                    }

                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF7A70F1);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF2311E8);

                    // Draw close button
                    if (ImGuiExt::TitleBarButton(ICON_VS_CHROME_CLOSE, buttonSize)) {
                        FenestraManager::System::closeApplication();
                    }

                    ImGui::PopStyleColor(2);

                #endif
            }

            auto &titleBarButtons = InterfaceRegistry::impl::getTitlebarButtons();

            // Draw custom title bar buttons
            if (!titleBarButtons.empty()) {
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 7_scaled - buttonSize.x * float((titleBarButtonsVisible ? 4 : 0) + titleBarButtons.size()));

                if (ImGui::GetCursorPosX() > (searchBoxPos.x + searchBoxSize.x)) {
                    for (const auto &[icon, tooltip, callback] : titleBarButtons) {
                        ImGui::SetCursorPosY(titleBarButtonPosY);
                        if (ImGuiExt::TitleBarButton(icon.c_str(), buttonSize)) {
                            callback();
                        }
                        ImGuiExt::InfoTooltip(Lang(tooltip));
                    }
                }
            }

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();

            {
                ImGui::SetCursorPos(searchBoxPos);

                if (s_showSearchBar) {
                    const auto buttonColor = [](float alpha) {
                        return ImU32(ImColor(ImGui::GetStyleColorVec4(ImGuiCol_DockingEmptyBg) * ImVec4(1, 1, 1, alpha)));
                    };

                    ImGui::PushStyleColor(ImGuiCol_Button, buttonColor(0.5F));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColor(0.7F));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor(0.9F));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0_scaled);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4_scaled);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, scaled({ 1, 1 }));


                    if (ImGui::Button(s_windowTitle.c_str(), searchBoxSize)) {
                        EventSearchBoxClicked::post(ImGuiMouseButton_Left);
                    }

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                        EventSearchBoxClicked::post(ImGuiMouseButton_Right);

                    ImGui::PushTextWrapPos(300_scaled);

                    if (!s_windowTitleFull.empty()) {
                        if (ImGuiExt::InfoTooltip()) {
                            ImGui::BeginTooltip();
                            ImGui::TextUnformatted(s_windowTitleFull.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    ImGui::PopTextWrapPos();

                    ImGui::PopStyleVar(3);
                    ImGui::PopStyleColor(3);
                } else {
                    ImGui::TextUnformatted(s_windowTitle.c_str());
                }
            }
        }

        void populateMenu(const UnlocalizedString &menuName) {
            for (auto &[priority, menuItem] : InterfaceRegistry::impl::getMenuItems()) {
                if (!menuName.empty()) {
                    if (menuItem.unlocalizedNames[0] != menuName)
                        continue;
                }

                const auto &[
                    unlocalizedNames,
                    icon,
                    shortcut,
                    view,
                    callback,
                    enabledCallback,
                    selectedCallack,
                    toolbarIndex
                ] = menuItem;

                createNestedMenu(unlocalizedNames | std::views::drop(1), icon.glyph.c_str(), *shortcut, callback, enabledCallback, selectedCallack);
            }
        }

        void defineMenu(const UnlocalizedString &menuName) {
            if (ImGui::BeginMenu(Lang(menuName))) {
                populateMenu(menuName);
                ImGui::EndMenu();
            }
        }

        void drawMenu() {
            auto cursorPos = ImGui::GetCursorPosX();
            u32 fittingItems = 0;

            const auto &menuItems = InterfaceRegistry::impl::getMainMenuItems();
            for (const auto &[priority, menuItem] : menuItems) {
                auto menuName = Lang(menuItem.unlocalizedName);

                const auto padding = ImGui::GetStyle().FramePadding.x;
                bool lastItem = (fittingItems + 1) == menuItems.size();
                auto width = ImGui::CalcTextSize(menuName).x + padding * (lastItem ? -3.0F : 4.0F);

                if ((cursorPos + width) > (s_searchBarPosition - ImGui::CalcTextSize(ICON_VS_ELLIPSIS).x - padding * 2))
                    break;

                cursorPos += width;
                fittingItems += 1;
            }

            if (fittingItems <= 2)
                fittingItems = 0;

            {
                u32 count = 0;
                for (const auto &[priority, menuItem] : menuItems) {
                    if (count >= fittingItems)
                        break;

                    defineMenu(menuItem.unlocalizedName);

                    count += 1;
                }

                if (count == 0)
                    return;
            }

            if (fittingItems == 0) {
                if (ImGui::BeginMenu(ICON_VS_MENU)) {
                    for (const auto &[priority, menuItem] : menuItems) {
                        defineMenu(menuItem.unlocalizedName);
                    }
                    ImGui::EndMenu();
                }
            } else if (fittingItems < menuItems.size()) {
                u32 count = 0;
                if (ImGui::BeginMenu(ICON_VS_ELLIPSIS)) {
                    for (const auto &[priority, menuItem] : menuItems) {
                        ON_SCOPE_EXIT { count += 1; };
                        if (count < fittingItems)
                            continue;

                        defineMenu(menuItem.unlocalizedName);
                    }
                    ImGui::EndMenu();
                }
            }
        }

        void drawMainMenu([[maybe_unused]] float menuBarHeight) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0F);
            ImGui::SetNextWindowScroll(ImVec2(0, 0));

            #if defined(OS_MACOS)
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 8_scaled));
                ON_SCOPE_EXIT { ImGui::PopStyleVar(); };
            #endif

            if (ImGui::BeginMainMenuBar()) {
                ImGui::Dummy({});

                auto window = static_cast<SDL_Window*>(FenestraManager::System::getMainWindowHandle());

                ImGui::PopStyleVar(2);

                if (FenestraManager::System::isBorderlessWindowModeEnabled()) {
                    #if defined(OS_WINDOWS)
                        ImGui::SetCursorPosX(5_scaled);
                        ImGui::Image(s_logoTexture, ImVec2(menuBarHeight, menuBarHeight));
                        ImGui::SetCursorPosX(5_scaled);
                        ImGui::InvisibleButton("##logo", ImVec2(menuBarHeight, menuBarHeight));
                        if (ImGui::IsItemHovered() && ImGui::IsAnyMouseDown())
                            ImGui::OpenPopup("WindowingMenu");
                    #elif defined(OS_MACOS)
                        if (!isMacosFullScreenModeEnabled(window))
                            ImGui::SetCursorPosX(68_scaled);
                    #endif
                }

                if (ImGui::BeginPopup("WindowingMenu")) {
                    bool maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;

                    ImGui::BeginDisabled(!maximized);
                    if (ImGui::MenuItem(ICON_VS_CHROME_RESTORE " Restore"))  SDL_RestoreWindow(window);
                    ImGui::EndDisabled();

                    if (ImGui::MenuItem(ICON_VS_CHROME_MINIMIZE " Minimize")) SDL_MinimizeWindow(window);

                    ImGui::BeginDisabled(maximized);
                    if (ImGui::MenuItem(ICON_VS_CHROME_MAXIMIZE " Maximize")) SDL_MaximizeWindow(window);
                    ImGui::EndDisabled();

                    ImGui::Separator();

                    if (ImGui::MenuItem(ICON_VS_CHROME_CLOSE " Close"))    FenestraManager::System::closeApplication();

                    ImGui::EndPopup();
                }

                drawMenu();
                drawTitleBar();

                #if defined(OS_MACOS)
                    if (FenestraManager::System::isBorderlessWindowModeEnabled()) {
                        const auto windowSize = FenestraManager::System::getMainWindowSize();
                        const auto menuUnderlaySize = ImVec2(windowSize.x, ImGui::GetCurrentWindowRead()->MenuBarHeight);

                        ImGui::SetCursorPos(ImVec2());

                        // Drawing this button late allows widgets rendered before it to grab click events, forming an "input underlay"
                        if (ImGui::InvisibleButton("##mainMenuUnderlay", menuUnderlaySize, ImGuiButtonFlags_PressedOnDoubleClick)) {
                            macosHandleTitlebarDoubleClickGesture(window);
                        }
                    }
                #endif

                ImGui::EndMainMenuBar();
            } else {
                ImGui::PopStyleVar(2);
            }
        }

        void drawToolbar() {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0F);
            if (ImGui::BeginMenuBar()) {
                for (const auto &callback : InterfaceRegistry::impl::getToolbarItems()) {
                    callback();
                    ImGui::SameLine();
                }

                ImGui::EndMenuBar();
            }
            ImGui::PopStyleVar(2);
        }

        bool anySidebarItemsAvailable() {
            if (const auto &items = InterfaceRegistry::impl::getSidebarItems(); items.empty()) {
                return false;
            } else {
                return std::ranges::any_of(items, [](const auto &item) {
                    return item.enabledCallback();
                });
            }
        }

        void drawWelcomeScreen(const std::unique_ptr<InterfaceRegistry::WelcomeScreen> &welcomeScreen) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
            ImGui::PushStyleColor(ImGuiCol_WindowShadow, 0x00);
            if (ImGui::Begin("FenestraDockSpace", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus)) {
                static auto title = []{
                    std::array<char, 256> title = {};
                    ImFormatString(title.data(), title.size(), "%s/DockSpace_%08X", ImGui::GetCurrentWindowRead()->Name, ImGui::GetID("FenestraMainDock"));
                    return title;
                }();

                if (ImGui::Begin(title.data(), nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
                    ImGui::Dummy({});
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scaled({ 10, 10 }));
                    ImGui::PushStyleColor(ImGuiCol_WindowBg, u32(welcomeScreen->getBackgroundColor()));
                    ImGui::PushStyleColor(ImGuiCol_Border, 0x00);

                    ImGui::SetNextWindowScroll({ 0.0F, -1.0F });
                    ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail() + scaled({ 2_scaled, 6 }));
                    ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos() - ImVec2(1_scaled, ImGui::GetStyle().FramePadding.y + 2_scaled));
                    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
                    if (ImGui::Begin("Welcome Screen", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                        ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindowRead());

                        welcomeScreen->draw();
                    }
                    ImGui::End();

                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar();
                }
                ImGui::End();
                ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindowRead());
            }
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

    }

    void addWindowDecoration() {
        EventFrameBegin::subscribe([]{

            constexpr static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(FenestraManager::System::getMainWindowSize() - ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

            // Draw main window decoration
            if (ImGui::Begin("FenestraDockSpace", nullptr, windowFlags)) {
                ImGui::PopStyleVar(2);

                const auto drawList = ImGui::GetWindowDrawList();
                const auto shouldDrawSidebar = anySidebarItemsAvailable();

                const auto menuBarHeight = ImGui::GetCurrentWindowRead()->MenuBarHeight;
                const auto sidebarPos   = ImGui::GetCursorPos();
                const auto sidebarWidth = shouldDrawSidebar ? 20_scaled : 0;

                auto footerHeight  = ImGui::GetTextLineHeightWithSpacing() + 1_scaled;
                #if defined(OS_MACOS)
                    footerHeight += ImGui::GetStyle().WindowPadding.y * 2;
                #else
                    footerHeight += ImGui::GetStyle().FramePadding.y * 2;
                #endif

                const auto dockSpaceSize = FenestraManager::System::getMainWindowSize() - ImVec2(sidebarWidth, menuBarHeight * 2 + footerHeight - ImGui::GetStyle().FramePadding.y);

                ImGui::SetCursorPosX(sidebarWidth);
                drawFooter(drawList, dockSpaceSize);

                if (shouldDrawSidebar) {
                    // Draw sidebar background and separator
                    drawList->AddRectFilled(
                        ImGui::GetWindowPos() - ImVec2(0, ImGui::GetStyle().FramePadding.y + 1_scaled),
                        ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(dockSpaceSize.x, footerHeight - ImGui::GetStyle().FramePadding.y + 1_scaled),
                        ImGui::GetColorU32(ImGuiCol_MenuBarBg)
                    );

                    ImGui::SetCursorPos(sidebarPos);
                    drawSidebar(dockSpaceSize, sidebarPos, sidebarWidth);
                }

                drawMainMenu(menuBarHeight);
                drawToolbar();
            } else {
                ImGui::PopStyleVar(2);
            }
            ImGui::End();

            ImGui::PopStyleVar(2);

            if (const auto &welcomeScreen = InterfaceRegistry::impl::getWelcomeScreen(); welcomeScreen != nullptr)
                drawWelcomeScreen(welcomeScreen);

            // Draw main menu popups
            for (auto &[priority, menuItem] : InterfaceRegistry::impl::getMenuItems()) {
                const auto &[unlocalizedNames, icon, shortcut, view, callback, enabledCallback, selectedCallback, toolbarIndex] = menuItem;

                if (ImGui::BeginPopup(unlocalizedNames.front().get().c_str())) {
                    createNestedMenu({ unlocalizedNames.begin() + 1, unlocalizedNames.end() }, icon.glyph.c_str(), *shortcut, callback, enabledCallback, selectedCallback);
                    ImGui::EndPopup();
                }
            }
        });


        constexpr static auto ApplicationTitle = FENESTRA_APPLICATION_NAME;

        s_windowTitle = ApplicationTitle;

        // Handle updating the window title
        RequestUpdateWindowTitle::subscribe([] {
            std::string prefix, postfix;
            std::string title = ApplicationTitle;

            s_windowTitle     = prefix + fene::limitStringLength(title, 32) + postfix;
            s_windowTitleFull = prefix + title + postfix;

            auto window = static_cast<SDL_Window*>(FenestraManager::System::getMainWindowHandle());
            if (window != nullptr) {
                if (title != ApplicationTitle)
                    title = std::string(ApplicationTitle) + " - " + title;

                SDL_SetWindowTitle(window, title.c_str());
            }
        });
    }


}
