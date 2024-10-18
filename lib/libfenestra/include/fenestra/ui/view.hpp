#pragma once

#include <fenestra.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <fenestra/ui/imgui_extensions.h>

#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/api/interface_registry.hpp>
#include <fenestra/api/shortcut_manager.hpp>
#include <fenestra/api/event_manager.hpp>
#include <fenestra/api/localization_manager.hpp>

#include <fenestra/helpers/utils.hpp>


#include <map>
#include <string>

namespace fene {

    class View {
        View() = default;
    public:
        virtual ~View() = default;

        void close();

        /**
         * @brief Draws the view
         * @note Do not override this method. Override drawContent() instead
         */
        virtual void draw() = 0;

        /**
         * @brief Draws the content of the view
         */
        virtual void drawContent() = 0;

        /**
         * @brief Whether or not the view window should be drawn
         * @return True if the view window should be drawn, false otherwise
         */
        [[nodiscard]] virtual bool shouldDraw() const;

        /**
         * @brief Whether or not the entire view should be processed
         * If this returns false, the view will not be drawn and no shortcuts will be handled. This includes things
         * drawn in the drawAlwaysVisibleContent() function.
         * @return True if the view should be processed, false otherwise
         */
        [[nodiscard]] virtual bool shouldProcess() const;

        /**
         * @brief Whether or not the view should have an entry in the view menu
         * @return True if the view should have an entry in the view menu, false otherwise
         */
        [[nodiscard]] virtual bool hasViewMenuItemEntry() const;

        /**
         * @brief Gets the minimum size of the view window
         * @return The minimum size of the view window
         */
        [[nodiscard]] virtual ImVec2 getMinSize() const;

        /**
         * @brief Gets the maximum size of the view window
         * @return The maximum size of the view window
         */
        [[nodiscard]] virtual ImVec2 getMaxSize() const;

        /**
         * @brief Gets additional window flags for the view window
         * @return Additional window flags for the view window
         */
        [[nodiscard]] virtual ImGuiWindowFlags getWindowFlags() const;

        [[nodiscard]] virtual bool shouldStoreWindowState() const { return true; }

        [[nodiscard]] const char *getIcon() const { return m_icon; }

        [[nodiscard]] const UnlocalizedString &getUnlocalizedName() const;
        [[nodiscard]] std::string getName() const;

        [[nodiscard]] bool didWindowJustOpen();
        void setWindowJustOpened(bool state);
        void setProperties(const char *icon, const UnlocalizedString &unlocalizedName, bool allowMultipleInstances);

        static void discardNavigationRequests();

    public:
        class Window;
        class Special;
        class Floating;
        class Modal;

    private:
        UnlocalizedString m_unlocalizedViewName;
        std::map<Shortcut, ShortcutManager::ShortcutEntry> m_shortcuts;
        bool m_windowJustOpened = false;
        const char *m_icon;
        bool m_allowMultipleInstances = false;

        friend class ShortcutManager;
    };


    /**
     * @brief A view that draws a regular window. This should be the default for most views
     */
    class View::Window : public View {
    public:
        Window() = default;

        void draw() final {
            if (this->shouldDraw()) {
                ImGui::SetNextWindowSizeConstraints(this->getMinSize(), this->getMaxSize());

                bool windowOpen = true;
                if (ImGui::Begin(this->getName().c_str(), &windowOpen, ImGuiWindowFlags_NoCollapse | this->getWindowFlags())) {
                    this->drawContent();
                }
                ImGui::End();

                if (!windowOpen) {
                    this->close();
                }
            }
        }
    };

    /**
     * @brief A view that doesn't handle any window creation and just draws its content.
     * This should be used if you intend to draw your own special window
     */
    class View::Special : public View {
    public:
        Special() = default;

        void draw() final {
            if (this->shouldDraw()) {
                ImGui::SetNextWindowSizeConstraints(this->getMinSize(), this->getMaxSize());
                this->drawContent();
            }
        }
    };

    /**
     * @brief A view that draws a floating window. These are the same as regular windows but cannot be docked
     */
    class View::Floating : public View::Window {
    public:
        Floating() = default;

        [[nodiscard]] ImGuiWindowFlags getWindowFlags() const override { return ImGuiWindowFlags_NoDocking; }
        [[nodiscard]] bool shouldStoreWindowState() const override { return false; }
    };

    /**
     * @brief A view that draws a modal window. The window will always be drawn on top and will block input to other windows
     */
    class View::Modal : public View {
    public:
        Modal() = default;

        void draw() final {
            if (this->shouldDraw()) {
                ImGui::OpenPopup(this->getName().c_str());

                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
                ImGui::SetNextWindowSizeConstraints(this->getMinSize(), this->getMaxSize());

                bool windowOpen = true;
                if (ImGui::BeginPopupModal(this->getName().c_str(), this->hasCloseButton() ? &windowOpen : nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | this->getWindowFlags())) {
                    this->drawContent();

                    ImGui::EndPopup();
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Escape) || !windowOpen)
                    this->close();
            }
        }

        [[nodiscard]] virtual bool hasCloseButton() const { return true; }
        [[nodiscard]] bool shouldStoreWindowState() const override { return false; }
    };

}
