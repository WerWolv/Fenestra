#include <fenestra/plugin.hpp>
#include <fenestra/api/interface_registry.hpp>
#include <fenestra/events/ui_events.hpp>

namespace fene::plugin::bundled {

    void addWindowDecoration();

    void registerThemeHandlers();
    void registerStyleHandlers();
    void registerThemes();

}

FENESTRA_PLUGIN_SETUP("Bundled", "Fenestra", "Integrated Fenestra functionality") {
    fene::plugin::bundled::addWindowDecoration();

    fene::plugin::bundled::registerThemeHandlers();
    fene::plugin::bundled::registerStyleHandlers();
    fene::plugin::bundled::registerThemes();
}