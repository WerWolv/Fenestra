#include <fenestra/api/interface_registry.hpp>
#include <fenestra/api/task_manager.hpp>

#include <fenestra/helpers/auto_reset.hpp>
#include <fenestra/ui/imgui_extensions.h>

namespace fene::InterfaceRegistry {

    namespace impl {

        static AutoReset<std::multimap<u32, MainMenuItem>> s_mainMenuItems;
        const std::multimap<u32, MainMenuItem>& getMainMenuItems() {
            return *s_mainMenuItems;
        }

        static AutoReset<std::multimap<u32, MenuItem>> s_menuItems;
        const std::multimap<u32, MenuItem>& getMenuItems() {
            return *s_menuItems;
        }

        static AutoReset<std::vector<MenuItem*>> s_toolbarMenuItems;
        const std::vector<MenuItem*>& getToolbarMenuItems() {
            return s_toolbarMenuItems;
        }


        std::multimap<u32, MenuItem>& getMenuItemsMutable() {
            return *s_menuItems;
        }

        static AutoReset<std::vector<DrawCallback>> s_welcomeScreenEntries;
        const std::vector<DrawCallback>& getWelcomeScreenEntries() {
            return *s_welcomeScreenEntries;
        }

        static AutoReset<std::vector<DrawCallback>> s_footerItems;
        const std::vector<DrawCallback>& getFooterItems() {
            return *s_footerItems;
        }

        static AutoReset<std::vector<DrawCallback>> s_toolbarItems;
        const std::vector<DrawCallback>& getToolbarItems() {
            return *s_toolbarItems;
        }

        static AutoReset<std::vector<SidebarItem>> s_sidebarItems;
        const std::vector<SidebarItem>& getSidebarItems() {
            return *s_sidebarItems;
        }

        static AutoReset<std::vector<TitleBarButton>> s_titlebarButtons;
        const std::vector<TitleBarButton>& getTitlebarButtons() {
            return *s_titlebarButtons;
        }

        static AutoReset<std::map<std::string, std::unique_ptr<View>>> s_views;
        const std::map<std::string, std::unique_ptr<View>>& getViews() {
            return *s_views;
        }

        AutoReset<std::map<UnlocalizedString, ViewCreator>> s_viewCreators;
        const std::map<UnlocalizedString, ViewCreator>& getViewCreators() {
            return *s_viewCreators;
        }

        void addViewCreator(const UnlocalizedString &unlocalizedName, const char *icon, std::function<std::unique_ptr<View>()> &&viewCreator) {
            s_viewCreators->insert({ unlocalizedName, { icon, std::move(viewCreator) } });
        }

        AutoReset<std::list<std::unique_ptr<View>>> s_openViews;
        const std::list<std::unique_ptr<View>>& getOpenViews() {
            return *s_openViews;
        }

        static AutoReset<std::unique_ptr<SplashScreen>> s_splashScreen;
        const std::unique_ptr<SplashScreen>& getSplashScreen() {
            return s_splashScreen;
        }

    }

    void openView(const UnlocalizedString &unlocalizedName) {
        impl::s_openViews->push_back(impl::getViewCreators().at(unlocalizedName).creator());
    }

    void closeView(View *view) {
        TaskManager::doLater([view] {
            impl::s_openViews->remove_if([view](const auto &v) {
                return v.get() == view;
            });
        });
    }

    void registerMainMenuItem(const UnlocalizedString &unlocalizedName, u32 priority) {
        log::debug("Registered new main menu item: {}", unlocalizedName.get());

        impl::s_mainMenuItems->insert({ priority, { unlocalizedName } });
    }

    void addMenuItem(const std::vector<UnlocalizedString> &unlocalizedMainMenuNames, u32 priority, const Shortcut &shortcut, const impl::MenuCallback &function, const impl::EnabledCallback& enabledCallback, const impl::SelectedCallback &selectedCallback, View *view) {
        addMenuItem(unlocalizedMainMenuNames, "", priority, shortcut, function, enabledCallback, selectedCallback, view);
    }

    void addMenuItem(const std::vector<UnlocalizedString> &unlocalizedMainMenuNames, const Icon &icon, u32 priority, const Shortcut &shortcut, const impl::MenuCallback &function, const impl::EnabledCallback& enabledCallback, View *view) {
        addMenuItem(unlocalizedMainMenuNames, icon, priority, shortcut, function, enabledCallback, []{ return false; }, view);
    }

    void addMenuItem(const std::vector<UnlocalizedString> &unlocalizedMainMenuNames, u32 priority, const Shortcut &shortcut, const impl::MenuCallback &function, const impl::EnabledCallback& enabledCallback, View *view) {
        addMenuItem(unlocalizedMainMenuNames, "", priority, shortcut, function, enabledCallback, []{ return false; }, view);
    }

    void addMenuItem(const std::vector<UnlocalizedString> &unlocalizedMainMenuNames, const Icon &icon, u32 priority, const Shortcut &shortcut, const impl::MenuCallback &function, const impl::EnabledCallback& enabledCallback, const impl::SelectedCallback &selectedCallback, View *view) {
        log::debug("Added new menu item to menu {} with priority {}", unlocalizedMainMenuNames[0].get(), priority);

        Icon coloredIcon = icon;
        if (coloredIcon.color == 0x00)
            coloredIcon.color = ImGuiCustomCol_ToolbarGray;

        impl::s_menuItems->insert({
            priority, impl::MenuItem { unlocalizedMainMenuNames, coloredIcon, std::make_unique<Shortcut>(shortcut), view, function, enabledCallback, selectedCallback, -1 }
        });

        if (shortcut != Shortcut::None) {
            if (shortcut.isLocal() && view != nullptr)
                ShortcutManager::addShortcut(view, shortcut, unlocalizedMainMenuNames.back(), function);
            else
                ShortcutManager::addGlobalShortcut(shortcut, unlocalizedMainMenuNames.back(), function);
        }
    }

    void addMenuItemSubMenu(std::vector<UnlocalizedString> unlocalizedMainMenuNames, u32 priority, const impl::MenuCallback &function, const impl::EnabledCallback& enabledCallback) {
        addMenuItemSubMenu(std::move(unlocalizedMainMenuNames), "", priority, function, enabledCallback);
    }

    void addMenuItemSubMenu(std::vector<UnlocalizedString> unlocalizedMainMenuNames, const char *icon, u32 priority, const impl::MenuCallback &function, const impl::EnabledCallback& enabledCallback) {
        log::debug("Added new menu item sub menu to menu {} with priority {}", unlocalizedMainMenuNames[0].get(), priority);

        unlocalizedMainMenuNames.emplace_back(impl::SubMenuValue);
        impl::s_menuItems->insert({
            priority, impl::MenuItem { unlocalizedMainMenuNames, icon, std::make_unique<Shortcut>(), nullptr, function, enabledCallback, []{ return false; }, -1 }
        });
    }

    void addMenuItemSeparator(std::vector<UnlocalizedString> unlocalizedMainMenuNames, u32 priority) {
        unlocalizedMainMenuNames.emplace_back(impl::SeparatorValue);
        impl::s_menuItems->insert({
            priority, impl::MenuItem { unlocalizedMainMenuNames, "", std::make_unique<Shortcut>(), nullptr, []{}, []{ return true; }, []{ return false; }, -1 }
        });
    }

    void addWelcomeScreenEntry(const impl::DrawCallback &function) {
        impl::s_welcomeScreenEntries->push_back(function);
    }

    void addFooterItem(const impl::DrawCallback &function) {
        impl::s_footerItems->push_back(function);
    }

    void addToolbarItem(const impl::DrawCallback &function) {
        impl::s_toolbarItems->push_back(function);
    }

    void addMenuItemToToolbar(const UnlocalizedString& unlocalizedName, ImGuiCustomCol color) {
        const auto maxIndex = std::ranges::max_element(impl::getMenuItems(), [](const auto &a, const auto &b) {
            return a.second.toolbarIndex < b.second.toolbarIndex;
        })->second.toolbarIndex;

        for (auto &[priority, menuItem] : *impl::s_menuItems) {
            if (menuItem.unlocalizedNames.back() == unlocalizedName) {
                menuItem.toolbarIndex = maxIndex + 1;
                menuItem.icon.color = color;
                updateToolbarItems();

                break;
            }
        }
    }

    struct MenuItemSorter {
        bool operator()(const auto *a, const auto *b) const {
            return a->toolbarIndex < b->toolbarIndex;
        }
    };

    void updateToolbarItems() {
        std::set<impl::MenuItem*, MenuItemSorter> menuItems;

        for (auto &[priority, menuItem] : impl::getMenuItemsMutable()) {
            if (menuItem.toolbarIndex != -1) {
                menuItems.insert(&menuItem);
            }
        }

        impl::s_toolbarMenuItems->clear();
        for (auto menuItem : menuItems) {
            impl::s_toolbarMenuItems->push_back(menuItem);
        }
    }



    void addSidebarItem(const std::string &icon, const impl::DrawCallback &function, const impl::EnabledCallback &enabledCallback) {
        impl::s_sidebarItems->push_back({ icon, function, enabledCallback });
    }

    void addTitleBarButton(const std::string &icon, const UnlocalizedString &unlocalizedTooltip, const impl::ClickCallback &function) {
        impl::s_titlebarButtons->push_back({ icon, unlocalizedTooltip, function });
    }

    View* getViewByName(const UnlocalizedString &unlocalizedName) {
        auto &views = *impl::s_views;

        if (views.contains(unlocalizedName))
            return views[unlocalizedName].get();
        else
            return nullptr;
    }

    void setSplashScreen(std::unique_ptr<SplashScreen> &&splashScreen) {
        impl::s_splashScreen = std::move(splashScreen);
    }

}
