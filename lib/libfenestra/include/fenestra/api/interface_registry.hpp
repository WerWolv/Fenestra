#pragma once

#include <fenestra.hpp>

#include <fenestra/api/localization_manager.hpp>
#include <fenestra/api/shortcut_manager.hpp>

#include <fenestra/ui/view.hpp>

#include <imgui.h>

#include <string>
#include <functional>
#include <list>

enum ImGuiCustomCol : int;

namespace fene {

    namespace InterfaceRegistry {
        struct Icon {
            Icon(const char *glyph, ImGuiCustomCol color = ImGuiCustomCol(0x00)) : glyph(glyph), color(color) {}

            std::string glyph;
            ImGuiCustomCol color;
        };

        class SplashScreen {
        public:
            SplashScreen(ImVec2 size) : m_size(size) { }
            virtual ~SplashScreen() = default;

            ImVec2 getSize() const { return m_size; }
            virtual void draw(u32 completedTasks, u32 totalTasks) = 0;

        private:
            ImVec2 m_size;
        };

        namespace impl {

            using DrawCallback      = std::function<void()>;
            using MenuCallback      = std::function<void()>;
            using EnabledCallback   = std::function<bool()>;
            using SelectedCallback  = std::function<bool()>;
            using ClickCallback     = std::function<void()>;

            struct MainMenuItem {
                UnlocalizedString unlocalizedName;
            };

            struct MenuItem {
                std::vector<UnlocalizedString> unlocalizedNames;
                Icon icon;
                std::unique_ptr<Shortcut> shortcut;
                View *view;
                MenuCallback callback;
                EnabledCallback enabledCallback;
                SelectedCallback selectedCallback;
                i32 toolbarIndex;
            };

            struct SidebarItem {
                std::string icon;
                DrawCallback callback;
                EnabledCallback enabledCallback;
            };

            struct TitleBarButton {
                std::string icon;
                UnlocalizedString unlocalizedTooltip;
                ClickCallback callback;
            };

            constexpr static auto SeparatorValue = "$SEPARATOR$";
            constexpr static auto SubMenuValue = "$SUBMENU$";

            const std::multimap<u32, MainMenuItem>& getMainMenuItems();

            const std::multimap<u32, MenuItem>& getMenuItems();
            const std::vector<MenuItem*>& getToolbarMenuItems();
            std::multimap<u32, MenuItem>& getMenuItemsMutable();

            const std::vector<DrawCallback>& getWelcomeScreenEntries();
            const std::vector<DrawCallback>& getFooterItems();
            const std::vector<DrawCallback>& getToolbarItems();
            const std::vector<SidebarItem>& getSidebarItems();
            const std::vector<TitleBarButton>& getTitlebarButtons();

            struct ViewCreator {
                const char *icon;
                std::function<std::unique_ptr<View>()> creator;
            };

            void addViewCreator(const UnlocalizedString &unlocalizedName, const char *icon, std::function<std::unique_ptr<View>()> &&viewCreator);
            const std::map<UnlocalizedString, ViewCreator>& getViewCreators();
            const std::list<std::unique_ptr<View>>& getOpenViews();

            const std::unique_ptr<SplashScreen>& getSplashScreen();

        }

        template<typename T>
        concept ViewType = requires {
            requires std::derived_from<View, T>;
            { T::Icon } -> std::convertible_to<const char *>;
            { T::UnlocalizedName } -> std::convertible_to<const char *>;
            { T::AllowMultipleInstances } -> std::convertible_to<bool>;
        };

        /**
         * @brief Adds a new view
         * @tparam T The custom view class that extends View
         * @tparam Args Arguments types
         * @param args Arguments passed to the constructor of the view
         */
        template<ViewType T, typename... Args>
        void addView(Args && ... args) {
            return impl::addViewCreator(T::UnlocalizedName, T::Icon, [args...] {
                auto view = std::make_unique<T>(auto(args)...);

                view->setWindowJustOpened(true);
                view->setProperties(T::Icon, T::UnlocalizedName, T::AllowMultipleInstances);
                return view;
            });
        }

        void openView(const UnlocalizedString &unlocalizedName);
        void closeView(View *view);

        /**
         * @brief Gets a view by its unlocalized name
         * @param unlocalizedName The unlocalized name of the view
         * @return The view if it exists, nullptr otherwise
         */
        View* getViewByName(const UnlocalizedString &unlocalizedName);

        /**
         * @brief Adds a new top-level main menu entry
         * @param unlocalizedName The unlocalized name of the entry
         * @param priority The priority of the entry. Lower values are displayed first
         */
        void registerMainMenuItem(const UnlocalizedString &unlocalizedName, u32 priority);

        /**
         * @brief Adds a new main menu entry
         * @param unlocalizedMainMenuNames The unlocalized names of the main menu entries
         * @param icon The icon to use for the entry
         * @param priority The priority of the entry. Lower values are displayed first
         * @param shortcut The shortcut to use for the entry
         * @param function The function to call when the entry is clicked
         * @param enabledCallback The function to call to determine if the entry is enabled
         * @param view The view to use for the entry. If nullptr, the shortcut will work globally
         */
        void addMenuItem(
            const std::vector<UnlocalizedString> &unlocalizedMainMenuNames,
            const Icon &icon,
            u32 priority,
            const Shortcut &shortcut,
            const impl::MenuCallback &function,
            const impl::EnabledCallback& enabledCallback, View *view
        );

        /**
         * @brief Adds a new main menu entry
         * @param unlocalizedMainMenuNames The unlocalized names of the main menu entries
         * @param icon The icon to use for the entry
         * @param priority The priority of the entry. Lower values are displayed first
         * @param shortcut The shortcut to use for the entry
         * @param function The function to call when the entry is clicked
         * @param enabledCallback The function to call to determine if the entry is enabled
         * @param selectedCallback The function to call to determine if the entry is selected
         * @param view The view to use for the entry. If nullptr, the shortcut will work globally
         */
        void addMenuItem(
            const std::vector<UnlocalizedString> &unlocalizedMainMenuNames,
            const Icon &icon,
            u32 priority,
            const Shortcut &shortcut,
            const impl::MenuCallback &function,
            const impl::EnabledCallback& enabledCallback = []{ return true; },
            const impl::SelectedCallback &selectedCallback = []{ return false; },
            View *view = nullptr
        );

        /**
         * @brief Adds a new main menu entry
         * @param unlocalizedMainMenuNames The unlocalized names of the main menu entries
         * @param priority The priority of the entry. Lower values are displayed first
         * @param shortcut The shortcut to use for the entry
         * @param function The function to call when the entry is clicked
         * @param enabledCallback The function to call to determine if the entry is enabled
         * @param selectedCallback The function to call to determine if the entry is selected
         * @param view The view to use for the entry. If nullptr, the shortcut will work globally
         */
        void addMenuItem(
            const std::vector<UnlocalizedString> &unlocalizedMainMenuNames,
            u32 priority,
            const Shortcut &shortcut,
            const impl::MenuCallback &function,
            const impl::EnabledCallback& enabledCallback = []{ return true; },
            const impl::SelectedCallback &selectedCallback = []{ return false; },
            View *view = nullptr
        );

        /**
         * @brief Adds a new main menu sub-menu entry
         * @param unlocalizedMainMenuNames The unlocalized names of the main menu entries
         * @param priority The priority of the entry. Lower values are displayed first
         * @param function The function to call when the entry is clicked
         * @param enabledCallback The function to call to determine if the entry is enabled
         */
        void addMenuItemSubMenu(
            std::vector<UnlocalizedString> unlocalizedMainMenuNames,
            u32 priority,
            const impl::MenuCallback &function,
            const impl::EnabledCallback& enabledCallback = []{ return true; }
        );

        /**
         * @brief Adds a new main menu sub-menu entry
         * @param unlocalizedMainMenuNames The unlocalized names of the main menu entries
         * @param icon The icon to use for the entry
         * @param priority The priority of the entry. Lower values are displayed first
         * @param function The function to call when the entry is clicked
         * @param enabledCallback The function to call to determine if the entry is enabled
         */
        void addMenuItemSubMenu(
            std::vector<UnlocalizedString> unlocalizedMainMenuNames,
            const char *icon,
            u32 priority,
            const impl::MenuCallback &function,
            const impl::EnabledCallback& enabledCallback = []{ return true; }
        );


        /**
         * @brief Adds a new main menu separator
         * @param unlocalizedMainMenuNames The unlocalized names of the main menu entries
         * @param priority The priority of the entry. Lower values are displayed first
         */
        void addMenuItemSeparator(std::vector<UnlocalizedString> unlocalizedMainMenuNames, u32 priority);


        /**
         * @brief Adds a new welcome screen entry
         * @param function The function to call to draw the entry
         */
        void addWelcomeScreenEntry(const impl::DrawCallback &function);

        /**
         * @brief Adds a new footer item
         * @param function The function to call to draw the item
         */
        void addFooterItem(const impl::DrawCallback &function);

        /**
         * @brief Adds a new toolbar item
         * @param function The function to call to draw the item
         */
        void addToolbarItem(const impl::DrawCallback &function);

        /**
         * @brief Adds a menu item to the toolbar
         * @param unlocalizedName Unlocalized name of the menu item
         * @param color Color of the toolbar icon
         */
        void addMenuItemToToolbar(const UnlocalizedString &unlocalizedName, ImGuiCustomCol color);

        /**
         * @brief Reconstructs the toolbar items list after they have been modified
         */
        void updateToolbarItems();

        /**
         * @brief Adds a new sidebar item
         * @param icon The icon to use for the item
         * @param function The function to call to draw the item
         * @param enabledCallback The function
         */
        void addSidebarItem(
            const std::string &icon,
            const impl::DrawCallback &function,
            const impl::EnabledCallback &enabledCallback = []{ return true; }
        );

        /**
         * @brief Adds a new title bar button
         * @param icon The icon to use for the button
         * @param unlocalizedTooltip The unlocalized tooltip to use for the button
         * @param function The function to call when the button is clicked
         */
        void addTitleBarButton(
            const std::string &icon,
            const UnlocalizedString &unlocalizedTooltip,
            const impl::ClickCallback &function
        );

        void setSplashScreen(std::unique_ptr<SplashScreen> &&splashScreen);

        template<std::derived_from<SplashScreen> T>
        void setSplashScreen(auto && ... args) {
            setSplashScreen(std::make_unique<T>(std::forward<decltype(args)>(args)...));
        }

    }

}