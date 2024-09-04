#pragma once

#include <fenestra/helpers/fs.hpp>

#include <vector>

namespace fene::paths {

    namespace impl {

        class DefaultPath {
        protected:
            constexpr DefaultPath() = default;
            virtual ~DefaultPath() = default;

        public:
            DefaultPath(const DefaultPath&) = delete;
            DefaultPath(DefaultPath&&) = delete;
            DefaultPath& operator=(const DefaultPath&) = delete;
            DefaultPath& operator=(DefaultPath&&) = delete;

            virtual std::vector<std::fs::path> all() const = 0;
            virtual std::vector<std::fs::path> read() const;
            virtual std::vector<std::fs::path> write() const;
        };

        class ConfigPath : public DefaultPath {
        public:
            explicit ConfigPath(std::fs::path postfix) : m_postfix(std::move(postfix)) {}

            std::vector<std::fs::path> all() const override;

        private:
            std::fs::path m_postfix;
        };

        class DataPath : public DefaultPath {
        public:
            explicit DataPath(std::fs::path postfix) : m_postfix(std::move(postfix)) {}

            std::vector<std::fs::path> all() const override;
            std::vector<std::fs::path> write() const override;

        private:
            std::fs::path m_postfix;
        };

        class PluginPath : public DefaultPath {
        public:
            explicit PluginPath(std::fs::path postfix) : m_postfix(std::move(postfix)) {}

            std::vector<std::fs::path> all() const override;

        private:
            std::fs::path m_postfix;
        };

    }

    std::vector<std::fs::path> getDataPaths(bool includeSystemFolders);
    std::vector<std::fs::path> getConfigPaths(bool includeSystemFolders);

    const static inline impl::ConfigPath Config("config");

    const static inline impl::PluginPath Libraries("lib");
    const static inline impl::PluginPath Plugins("plugins");

    const static inline impl::DataPath Resources("resources");
    const static inline impl::DataPath Logs("logs");
    const static inline impl::DataPath Themes("themes");
    const static inline impl::DataPath Layouts("layouts");
    const static inline impl::DataPath Workspaces("workspaces");

    constexpr static inline std::array<const impl::DefaultPath*, 20> All = {
        &Config,

        &Libraries,
        &Plugins,

        &Resources,
        &Logs,
        &Themes,
        &Layouts,
        &Workspaces,
    };

}