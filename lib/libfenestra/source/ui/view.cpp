#include <fenestra/ui/view.hpp>

#include <fenestra/api/interface_registry.hpp>

#include <string>

namespace fene {

    void View::close() {
        InterfaceRegistry::closeView(this);
    }

    bool View::shouldDraw() const {
        return true;
    }

    bool View::shouldProcess() const {
        const auto &welcomeScreen = InterfaceRegistry::impl::getWelcomeScreen();
        if (welcomeScreen != nullptr && !welcomeScreen->isVisible())
            return false;

        return this->shouldDraw();
    }

    bool View::hasViewMenuItemEntry() const {
        return true;
    }

    ImVec2 View::getMinSize() const {
        return scaled({ 300, 400 });
    }

    ImVec2 View::getMaxSize() const {
        return { FLT_MAX, FLT_MAX };
    }

    ImGuiWindowFlags View::getWindowFlags() const {
        return ImGuiWindowFlags_None;
    }

    const UnlocalizedString &View::getUnlocalizedName() const {
        return m_unlocalizedViewName;
    }

    std::string View::getName() const {
        return fmt::format("{}###{}_{}", Lang(m_unlocalizedViewName), m_unlocalizedViewName.get(), std::uintptr_t(this));
    }

    bool View::didWindowJustOpen() {
        return std::exchange(m_windowJustOpened, false);
    }

    void View::setWindowJustOpened(bool state) {
        m_windowJustOpened = state;
    }

    void View::setProperties(const char *icon, const UnlocalizedString &unlocalizedName, bool allowMultipleInstances) {
        m_icon = icon;
        m_unlocalizedViewName = unlocalizedName;
        m_allowMultipleInstances = allowMultipleInstances;
    }


    void View::discardNavigationRequests() {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    }

}
