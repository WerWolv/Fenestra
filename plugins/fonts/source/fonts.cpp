#include <fenestra/api/fenestra_manager.hpp>

#include <romfs/romfs.hpp>

#include <fenestra/helpers/utils.hpp>

#include <fonts/codicons_font.h>
#include <fonts/blendericons_font.h>

namespace fene::fonts {

    void registerFonts() {
        /**
         *  !!IMPORTANT!!
         *  Always load the font files in decreasing size of their glyphs. This will make the rasterize be more
         *  efficient when packing the glyphs into the font atlas and therefor make the atlas much smaller.
         */

        FenestraManager::Fonts::loadFont("Blender Icons",  romfs::get("fonts/blendericons.ttf").span<u8>(),{ { ICON_MIN_BI, ICON_MAX_BI } }, { -1_scaled, -1_scaled });

        FenestraManager::Fonts::loadFont("VS Codicons", romfs::get("fonts/codicons.ttf").span<u8>(),
            {
                { ICON_MIN_VS, ICON_MAX_VS }
            },
            { -1_scaled, -1_scaled });

        FenestraManager::Fonts::loadFont("Unifont", romfs::get("fonts/unifont.otf").span<u8>(), {}, {}, 0, 16);
    }

}
