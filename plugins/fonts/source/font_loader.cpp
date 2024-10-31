#include <imgui.h>
#include <imgui_freetype.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <list>

#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/api/interface_registry.hpp>
#include <fenestra/api/event_manager.hpp>
#include <fenestra/events/system_events.hpp>

#include <fenestra/helpers/logger.hpp>
#include <fenestra/helpers/default_paths.hpp>

#include <romfs/romfs.hpp>

#include <wolv/io/file.hpp>
#include <wolv/utils/string.hpp>

namespace fene::fonts {

    namespace {

        class Font {
        public:
            Font() = default;

            [[nodiscard]] float getDescent() const {
                return m_font->Descent;
            }

        private:
            explicit Font(ImFont *font) : m_font(font) { }

        private:
            friend class FontAtlas;

            ImFont *m_font;
        };

        class FontAtlas {
        public:
            FontAtlas() : m_fontAtlas(IM_NEW(ImFontAtlas)) {
                enableUnicodeCharacters(false);

                // Set the default configuration for the font atlas
                m_config.OversampleH = m_config.OversampleV = 1;
                m_config.PixelSnapH = true;
                m_config.MergeMode = false;

                // Make sure the font atlas doesn't get too large, otherwise weaker GPUs might reject it
                m_fontAtlas->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
                m_fontAtlas->TexDesiredWidth = 4096;

                setAntiAliasing(false);
                setItalic(false);
                setBold(false);
            }

            ~FontAtlas() {
                IM_DELETE(m_fontAtlas);
            }

            Font addDefaultFont() {
                ImFontConfig config = m_config;
                config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
                config.SizePixels = std::floor(float(FenestraManager::System::getGlobalScale())) * 13.0F;

                #if defined(OS_MACOS)
                    config.SizePixels *= 2.0F;
                #endif

                auto font = m_fontAtlas->AddFontDefault(&config);
                m_fontSizes.emplace_back(false, config.SizePixels);

                m_config.MergeMode = true;

                return Font(font);
            }

            Font addFontFromMemory(const std::vector<u8> &fontData, float fontSize, bool scalable, ImVec2 offset, const ImVector<ImWchar> &glyphRange = {}) {
                auto &storedFontData = m_fontData.emplace_back(fontData);

                ImFontConfig config = m_config;
                config.FontBuilderFlags &= ~(ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting);
                config.FontDataOwnedByAtlas = false;

                config.GlyphOffset = { offset.x, offset.y };
                auto font = m_fontAtlas->AddFontFromMemoryTTF(storedFontData.data(), int(storedFontData.size()), fontSize, &config, !glyphRange.empty() ? glyphRange.Data : m_glyphRange.Data);
                m_fontSizes.emplace_back(scalable, fontSize);

                m_config.MergeMode = true;

                return Font(font);
            }

            Font addFontFromRomFs(const std::fs::path &path, float fontSize, bool scalable, ImVec2 offset, const ImVector<ImWchar> &glyphRange = {}) {
                auto data = romfs::get(path).span<u8>();
                return addFontFromMemory({ data.begin(), data.end() }, fontSize, scalable, offset, glyphRange);
            }

            Font addFontFromFile(const std::fs::path &path, float fontSize, bool scalable, ImVec2 offset, const ImVector<ImWchar> &glyphRange = {}) {
                wolv::io::File file(path, wolv::io::File::Mode::Read);

                auto data = file.readVector();
                return addFontFromMemory(data, fontSize, scalable, offset, glyphRange);
            }

            void setBold(bool enabled) {
                if (enabled)
                    m_config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Bold;
                else
                    m_config.FontBuilderFlags &= ~ImGuiFreeTypeBuilderFlags_Bold;
            }

            void setItalic(bool enabled) {
                if (enabled)
                    m_config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Oblique;
                else
                    m_config.FontBuilderFlags &= ~ImGuiFreeTypeBuilderFlags_Oblique;
            }

            void setAntiAliasing(bool enabled) {
                if (enabled)
                    m_config.FontBuilderFlags &= ~ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
                else
                    m_config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
            }

            void enableUnicodeCharacters(bool enabled) {
                ImFontGlyphRangesBuilder glyphRangesBuilder;

                {
                    constexpr static std::array<ImWchar, 3> controlCodeRange    = { 0x0001, 0x001F, 0 };
                    constexpr static std::array<ImWchar, 3> extendedAsciiRange  = { 0x007F, 0x00FF, 0 };
                    constexpr static std::array<ImWchar, 3> latinExtendedARange = { 0x0100, 0x017F, 0 };

                    glyphRangesBuilder.AddRanges(controlCodeRange.data());
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesDefault());
                    glyphRangesBuilder.AddRanges(extendedAsciiRange.data());
                    glyphRangesBuilder.AddRanges(latinExtendedARange.data());
                }

                if (enabled) {
                    constexpr static std::array<ImWchar, 3> fullRange = { 0x0180, 0xFFEF, 0 };

                    glyphRangesBuilder.AddRanges(fullRange.data());
                } else {
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesJapanese());
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesChineseFull());
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesCyrillic());
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesKorean());
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesThai());
                    glyphRangesBuilder.AddRanges(m_fontAtlas->GetGlyphRangesVietnamese());
                }

                m_glyphRange.clear();
                glyphRangesBuilder.BuildRanges(&m_glyphRange);
            }

            [[nodiscard]] bool build() const {
                return m_fontAtlas->Build();
            }

            [[nodiscard]] ImFontAtlas* getAtlas() {
                auto result = m_fontAtlas;

                return result;
            }

            [[nodiscard]] float calculateFontDescend(const FenestraManager::Fonts::Font &font, float fontSize) const {
                auto atlas = std::make_unique<ImFontAtlas>();
                auto cfg = m_config;

                // Calculate the expected font size
                float size = fontSize;
                if (font.defaultSize.has_value())
                    size = float(font.defaultSize.value()) * std::max(1.0F, std::floor(FenestraManager::Fonts::getFontSize() / FenestraManager::Fonts::DefaultFontSize));
                else
                    size = std::max(1.0F, std::floor(size / FenestraManager::Fonts::DefaultFontSize)) * FenestraManager::Fonts::DefaultFontSize;

                cfg.MergeMode = false;
                cfg.SizePixels = size;
                cfg.FontDataOwnedByAtlas = false;

                // Construct a range that only contains the first glyph of the font
                ImVector<ImWchar> queryRange;
                {
                    auto firstGlyph = font.glyphRanges.empty() ? m_glyphRange.front() : font.glyphRanges.front().begin;
                    queryRange.push_back(firstGlyph);
                    queryRange.push_back(firstGlyph);
                }
                queryRange.push_back(0x00);

                // Build the font atlas with the query range
                auto newFont = atlas->AddFontFromMemoryTTF(const_cast<u8 *>(font.fontData.data()), int(font.fontData.size()), 0, &cfg, queryRange.Data);
                atlas->Build();

                return newFont->Descent;
            }

            void reset() {
                IM_DELETE(m_fontAtlas);
                m_fontAtlas = IM_NEW(ImFontAtlas);

                m_fontData.clear();
                m_config.MergeMode = false;
            }

            void updateFontScaling(float newScaling) {
                for (int i = 0; i < m_fontAtlas->ConfigData.size(); i += 1) {
                    const auto &[scalable, fontSize] = m_fontSizes[i];
                    auto &configData = m_fontAtlas->ConfigData[i];

                    if (!scalable) {
                        configData.SizePixels = fontSize * std::floor(newScaling);
                    } else {
                        configData.SizePixels = fontSize * newScaling;
                    }
                }
            }

        private:
            ImFontAtlas* m_fontAtlas;
            std::vector<std::pair<bool, float>> m_fontSizes;
            ImFontConfig m_config;
            ImVector<ImWchar> m_glyphRange;

            std::list<std::vector<u8>> m_fontData;
        };

        std::fs::path findCustomFontPath() {
            // Find the custom font file specified in the settings
            auto fontFile = FenestraManager::Fonts::getCustomFont().value_or(FenestraManager::Fonts::CustomFont { }).fontPath;
            if (!fontFile.empty()) {
                if (!wolv::io::fs::exists(fontFile) || !wolv::io::fs::isRegularFile(fontFile)) {
                    log::warn("Custom font file {} not found! Falling back to default font.", wolv::util::toUTF8String(fontFile));
                    fontFile.clear();
                }

                log::info("Loading custom font from {}", wolv::util::toUTF8String(fontFile));
            }

            // If no custom font has been specified, search for a file called "font.ttf" in one of the resource folders
            if (fontFile.empty()) {
                for (const auto &dir : paths::Resources.read()) {
                    auto path = dir / "font.ttf";
                    if (wolv::io::fs::exists(path)) {
                        log::info("Loading custom font from {}", wolv::util::toUTF8String(path));

                        fontFile = path;
                        break;
                    }
                }
            }

            return fontFile;
        }

        float getFontSize() {
            float fontSize = FenestraManager::Fonts::DefaultFontSize;

            if (auto scaling = FenestraManager::System::getGlobalScale(); u32(scaling) * 10 == u32(scaling * 10))
                fontSize *= scaling;
            else
                fontSize *= scaling * 0.75F;

            // Fall back to the default font if the global scale is 0
            if (fontSize == 0.0F)
                fontSize = FenestraManager::Fonts::DefaultFontSize;

            // If a custom font is used, adjust the font size
            if (!FenestraManager::Fonts::getCustomFont().has_value()) {
                fontSize = FenestraManager::Fonts::getFontSize() * FenestraManager::System::getGlobalScale();
            }

            return fontSize;
        }

    }

    bool buildFontAtlasImpl(bool loadUnicodeCharacters) {
        static FontAtlas fontAtlas;
        fontAtlas.reset();

        // Check if Unicode support is enabled in the settings and that the user doesn't use the No GPU version on Windows
        // The Mesa3D software renderer on Windows identifies itself as "VMware, Inc."
        bool shouldLoadUnicode =
                FenestraManager::Fonts::shouldLoadAllUnicodeCharacters() &&
                FenestraManager::System::getGPUVendor() != "VMware, Inc.";

        if (!loadUnicodeCharacters)
            shouldLoadUnicode = false;

        fontAtlas.enableUnicodeCharacters(shouldLoadUnicode);

        auto customFont = FenestraManager::Fonts::getCustomFont();

        // If a custom font is set in the settings, load the rest of the settings as well
        if (customFont.has_value()) {
            fontAtlas.setBold(customFont->bold);
            fontAtlas.setItalic(customFont->italic);
            fontAtlas.setAntiAliasing(customFont->antiAliased);

            FenestraManager::Fonts::impl::setCustomFontPath(findCustomFontPath());
        }
        FenestraManager::Fonts::impl::setFontSize(getFontSize());


        const auto fontSize = FenestraManager::Fonts::getFontSize();
        const auto &customFontPath = customFont.value_or(FenestraManager::Fonts::CustomFont { }).fontPath;

        // Try to load the custom font if one was set
        std::optional<Font> defaultFont;
        if (!customFontPath.empty()) {
            defaultFont = fontAtlas.addFontFromFile(customFontPath, fontSize, true, ImVec2());
            if (!fontAtlas.build()) {
                log::error("Failed to load custom font '{}'! Falling back to default font", wolv::util::toUTF8String(customFontPath));
                defaultFont.reset();
            }
        }

        // If there's no custom font set, or it failed to load, fall back to the default font
        if (!defaultFont.has_value()) {
            auto selectedFont = FenestraManager::Fonts::getSelectedFont();

            if (selectedFont == FenestraManager::Fonts::SelectedFont::BuiltinPixelPerfect)
                defaultFont = fontAtlas.addDefaultFont();
            else if (selectedFont == FenestraManager::Fonts::SelectedFont::BuiltinSmooth)
                defaultFont = fontAtlas.addFontFromRomFs("fonts/firacode.ttf", fontSize * 1.1, true, ImVec2());

            if (!fontAtlas.build()) {
                log::fatal("Failed to load default font!");
                return false;
            }
        }


        // Add all the built-in fonts
        {
            static std::list<ImVector<ImWchar>> glyphRanges;
            glyphRanges.clear();

            for (auto &font : FenestraManager::Fonts::impl::getFonts()) {
                // Construct the glyph range for the font
                ImVector<ImWchar> glyphRange;
                if (!font.glyphRanges.empty()) {
                    for (const auto &range : font.glyphRanges) {
                        glyphRange.push_back(range.begin);
                        glyphRange.push_back(range.end);
                    }
                    glyphRange.push_back(0x00);
                }
                glyphRanges.push_back(glyphRange);

                // Calculate the glyph offset for the font
                ImVec2 offset = { font.offset.x, font.offset.y - (defaultFont->getDescent() - fontAtlas.calculateFontDescend(font, fontSize)) };

                // Load the font
                fontAtlas.addFontFromMemory(font.fontData, font.defaultSize.value_or(fontSize), !font.defaultSize.has_value(), offset, glyphRanges.back());
            }
        }

        EventDPIChanged::subscribe([](float, float newScaling) {
            fontAtlas.updateFontScaling(newScaling);

            if (fontAtlas.build()) {
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                FenestraManager::Fonts::impl::setFontAtlas(fontAtlas.getAtlas());
            } else {
                log::error("Failed to build font atlas!");
            }
        });

        // Build the font atlas
        if (fontAtlas.build()) {
            // Set the font atlas if the build was successful
            FenestraManager::Fonts::impl::setFontAtlas(fontAtlas.getAtlas());
            return true;
        }

        // If the build wasn't successful and Unicode characters are enabled, try again without them
        // If they were disabled already, something went wrong, and we can't recover from it
        if (!shouldLoadUnicode) {
            // Reset Unicode loading and scaling factor settings back to default to make sure the user can still use the application
            FenestraManager::Fonts::impl::setLoadAllUnicodeCharacters(false);

            return false;
        } else {
            return buildFontAtlasImpl(false);
        }
    }

    bool buildFontAtlas() {
        return buildFontAtlasImpl(true);
    }

}