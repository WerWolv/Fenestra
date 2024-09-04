#include <fenestra/api/localization_manager.hpp>
#include <fenestra/helpers/auto_reset.hpp>

#include <nlohmann/json.hpp>

namespace fene {

    namespace LocalizationManager {

        namespace {

            AutoReset<std::string> s_fallbackLanguage;
            AutoReset<std::string> s_selectedLanguage;
            AutoReset<std::map<std::string, std::string>> s_currStrings;

        }

        namespace impl {

            void resetLanguageStrings() {
                s_currStrings->clear();
                s_selectedLanguage->clear();
            }

            void setFallbackLanguage(const std::string &language) {
                s_fallbackLanguage = language;
            }

        }

        LanguageDefinition::LanguageDefinition(std::map<std::string, std::string> &&entries) {
            for (const auto &[key, value] : entries) {
                if (value.empty())
                    continue;

                m_entries.insert({ key, value });
            }

        }

        const std::map<std::string, std::string> &LanguageDefinition::getEntries() const {
            return m_entries;
        }

        void loadLanguage(const std::string &language) {
            auto &definitions = impl::getLanguageDefinitions();

            if (!definitions.contains(language))
                return;

            s_currStrings->clear();

            for (const auto &definition : definitions.at(language))
                s_currStrings->insert(definition.getEntries().begin(), definition.getEntries().end());

            const auto& fallbackLanguage = getFallbackLanguage();
            if (language != fallbackLanguage && definitions.contains(fallbackLanguage)) {
                for (const auto &definition : definitions.at(fallbackLanguage))
                    s_currStrings->insert(definition.getEntries().begin(), definition.getEntries().end());
            }

            s_selectedLanguage = language;
        }

        std::string getLocalizedString(const std::string& unlocalizedString, const std::string& language) {
            if (language.empty())
                return getLocalizedString(unlocalizedString, getSelectedLanguage());

            auto &languageDefinitions = impl::getLanguageDefinitions();
            if (!languageDefinitions.contains(language))
                return "";

            std::string localizedString;
            for (const auto &definition : languageDefinitions.at(language)) {
                if (definition.getEntries().contains(unlocalizedString)) {
                    localizedString = definition.getEntries().at(unlocalizedString);
                    break;
                }
            }

            if (localizedString.empty())
                return getLocalizedString(unlocalizedString, getFallbackLanguage());

            return localizedString;
        }


        const std::map<std::string, std::string> &getSupportedLanguages() {
            return impl::getLanguages();
        }

        const std::string &getFallbackLanguage() {
            return s_fallbackLanguage;
        }

        const std::string &getSelectedLanguage() {
            return s_selectedLanguage;
        }

        namespace impl {

            static AutoReset<std::map<std::string, std::string>> s_languages;
            const std::map<std::string, std::string>& getLanguages() {
                return *s_languages;
            }

            static AutoReset<std::map<std::string, std::vector<LocalizationManager::LanguageDefinition>>> s_definitions;
            const std::map<std::string, std::vector<LocalizationManager::LanguageDefinition>>& getLanguageDefinitions() {
                return *s_definitions;
            }

        }

        void addLocalization(const nlohmann::json &data) {
            if (!data.is_object())
                return;

            if (!data.contains("code") || !data.contains("country") || !data.contains("language") || !data.contains("translations")) {
                log::error("Localization data is missing required fields!");
                return;
            }

            const auto &code            = data["code"];
            const auto &country         = data["country"];
            const auto &language        = data["language"];
            const auto &translations    = data["translations"];

            if (!code.is_string() || !country.is_string() || !language.is_string() || !translations.is_object()) {
                log::error("Localization data has invalid fields!");
                return;
            }

            if (data.contains("fallback")) {
                const auto &fallback = data["fallback"];

                if (fallback.is_boolean() && fallback.get<bool>())
                    LocalizationManager::impl::setFallbackLanguage(code.get<std::string>());
            }

            impl::s_languages->insert({ code.get<std::string>(), fmt::format("{} ({})", language.get<std::string>(), country.get<std::string>()) });

            std::map<std::string, std::string> translationDefinitions;
            for (auto &[key, value] : translations.items()) {
                if (!value.is_string()) {
                    log::error("Localization data has invalid fields!");
                    continue;
                }

                translationDefinitions[key] = value.get<std::string>();
            }

            (*impl::s_definitions)[code.get<std::string>()].emplace_back(std::move(translationDefinitions));
        }

    }

    Lang::Lang(const char *unlocalizedString) : m_unlocalizedString(unlocalizedString) { }
    Lang::Lang(const std::string &unlocalizedString) : m_unlocalizedString(unlocalizedString) { }
    Lang::Lang(const UnlocalizedString &unlocalizedString) : m_unlocalizedString(unlocalizedString.get()) { }
    Lang::Lang(std::string_view unlocalizedString) : m_unlocalizedString(unlocalizedString) { }


    Lang::operator std::string() const {
        return get();
    }

    Lang::operator std::string_view() const {
        return get();
    }

    Lang::operator const char *() const {
        return get().c_str();
    }

    std::string operator+(const std::string &&left, const Lang &&right) {
        return left + static_cast<std::string>(right);
    }

    std::string operator+(const Lang &&left, const std::string &&right) {
        return static_cast<std::string>(left) + right;
    }

    std::string operator+(const Lang &&left, const Lang &&right) {
        return static_cast<std::string>(left) + static_cast<std::string>(right);
    }

    std::string operator+(const std::string_view &&left, const Lang &&right) {
        return std::string(left) + static_cast<std::string>(right);
    }

    std::string operator+(const Lang &&left, const std::string_view &&right) {
        return static_cast<std::string>(left) + std::string(right);
    }

    std::string operator+(const char *left, const Lang &&right) {
        return left + static_cast<std::string>(right);
    }

    std::string operator+(const Lang &&left, const char *right) {
        return static_cast<std::string>(left) + right;
    }

    const std::string &Lang::get() const {
        auto &lang = LocalizationManager::s_currStrings;
        if (lang->contains(m_unlocalizedString))
            return lang->at(m_unlocalizedString);
        else
            return m_unlocalizedString;
    }

}