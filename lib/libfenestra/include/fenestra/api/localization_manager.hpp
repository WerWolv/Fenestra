#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

namespace fene {

    namespace LocalizationManager {

        class LanguageDefinition {
        public:
            explicit LanguageDefinition(std::map<std::string, std::string> &&entries);

            [[nodiscard]] const std::map<std::string, std::string> &getEntries() const;

        private:
            std::map<std::string, std::string> m_entries;
        };

        namespace impl {
            void setFallbackLanguage(const std::string &language);
            void resetLanguageStrings();
        }

        void loadLanguage(const std::string &language);
        std::string getLocalizedString(const std::string &unlocalizedString, const std::string &language = "");

        [[nodiscard]] const std::map<std::string, std::string> &getSupportedLanguages();
        [[nodiscard]] const std::string &getFallbackLanguage();
        [[nodiscard]] const std::string &getSelectedLanguage();

        void addLocalization(const nlohmann::json &data);

        namespace impl {

            const std::map<std::string, std::string>& getLanguages();
            const std::map<std::string, std::vector<LanguageDefinition>>& getLanguageDefinitions();

        }

    }

    struct UnlocalizedString;

    class Lang {
    public:
        explicit Lang(const char *unlocalizedString);
        explicit Lang(const std::string &unlocalizedString);
        explicit Lang(const UnlocalizedString &unlocalizedString);
        explicit Lang(std::string_view unlocalizedString);

        [[nodiscard]] operator std::string() const;
        [[nodiscard]] operator std::string_view() const;
        [[nodiscard]] operator const char *() const;

        [[nodiscard]] const std::string &get() const;

    private:
        std::string m_unlocalizedString;
    };

    [[nodiscard]] std::string operator+(const std::string &&left, const Lang &&right);
    [[nodiscard]] std::string operator+(const Lang &&left, const std::string &&right);
    [[nodiscard]] std::string operator+(const std::string_view &&left, const Lang &&right);
    [[nodiscard]] std::string operator+(const Lang &&left, const std::string_view &&right);
    [[nodiscard]] std::string operator+(const char *left, const Lang &&right);
    [[nodiscard]] std::string operator+(const Lang &&left, const char *right);
    [[nodiscard]] std::string operator+(const Lang &&left, const Lang &&right);

    [[nodiscard]] inline Lang operator""_lang(const char *string, size_t) {
        return Lang(string);
    }


    struct UnlocalizedString {
    public:
        UnlocalizedString() = default;
        UnlocalizedString(auto && arg) : m_unlocalizedString(std::forward<decltype(arg)>(arg)) {
            static_assert(!std::same_as<std::remove_cvref_t<decltype(arg)>, Lang>, "Expected a unlocalized name, got a localized one!");
        }

        [[nodiscard]] operator std::string() const {
            return m_unlocalizedString;
        }

        [[nodiscard]] operator std::string_view() const {
            return m_unlocalizedString;
        }

        [[nodiscard]] operator const char *() const {
            return m_unlocalizedString.c_str();
        }

        [[nodiscard]] const std::string &get() const {
            return m_unlocalizedString;
        }

        [[nodiscard]] bool empty() const {
            return m_unlocalizedString.empty();
        }

        auto operator<=>(const UnlocalizedString &) const = default;
        auto operator<=>(const std::string &other) const {
            return m_unlocalizedString <=> other;
        }

    private:
        std::string m_unlocalizedString;
    };

    // {fmt} formatter for fene::Lang
    inline auto format_as(const fene::Lang &entry) {
        return entry.get();
    }

}