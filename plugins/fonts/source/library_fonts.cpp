#include <fenestra/plugin.hpp>

#include <fenestra/api/interface_registry.hpp>
#include <fenestra/helpers/logger.hpp>

#include <romfs/romfs.hpp>

namespace fene::fonts {
    void registerFonts();

    bool buildFontAtlas();
}

FENESTRA_LIBRARY_SETUP("Fonts") {
    fene::log::debug("Using romfs: '{}'", romfs::name());

    fene::FenestraApi::System::addStartupTask("Loading fonts", true, fene::fonts::buildFontAtlas);
    fene::fonts::registerFonts();
}