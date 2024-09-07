#include <fenestra/plugin.hpp>
#include <fenestra/api/interface_registry.hpp>
#include <fenestra/events/ui_events.hpp>

namespace fene::plugin::bundled {

    void addWindowDecoration();

}

FENESTRA_PLUGIN_SETUP("Bundled", "Fenestra", "Integrated Fenestra functionality") {
    fene::plugin::bundled::addWindowDecoration();

    EventFrameBegin::subscribe([] {
        ImGui::ShowDemoWindow();
    });

    class WelcomeScreen : public fene::InterfaceRegistry::WelcomeScreen {
        void draw() final {
            ImGui::Text("Welcome to Fenestra!");
        }

        bool isVisible() final {
            return true;
        }
    };

    fene::InterfaceRegistry::setWelcomeScreen<WelcomeScreen>();

}