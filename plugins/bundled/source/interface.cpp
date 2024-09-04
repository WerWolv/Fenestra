#include <fonts/codicons_font.h>
#include <fenestra/plugin.hpp>
#include <fenestra/api/interface_registry.hpp>
#include <fenestra/events/ui_events.hpp>

namespace fene::plugin::bundled {

    void addWindowDecoration();

}

FENESTRA_PLUGIN_SETUP("Bundled", "Fenestra", "Integrated Fenestra functionality") {
    fene::plugin::bundled::addWindowDecoration();
}