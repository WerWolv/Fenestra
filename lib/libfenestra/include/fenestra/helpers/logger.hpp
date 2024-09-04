#pragma once

#include <fenestra.hpp>

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ranges.h>

#include <wolv/io/file.hpp>
#include <wolv/utils/guards.hpp>
#include <wolv/types/type_name.hpp>

namespace fene::log {

    namespace impl {

        FILE *getDestination();
        wolv::io::File& getFile();
        bool isRedirected();
        [[maybe_unused]] void redirectToFile();
        [[maybe_unused]] void enableColorPrinting();

        [[nodiscard]] bool isLoggingSuspended();
        [[nodiscard]] bool isDebugLoggingEnabled();

        void lockLoggerMutex();
        void unlockLoggerMutex();

        struct LogEntry {
            std::string project;
            std::string level;
            std::string message;
        };

        std::vector<LogEntry>& getLogEntries();
        void addLogEntry(std::string_view project, std::string_view level, std::string_view message);

        [[maybe_unused]] void printPrefix(FILE *dest, const fmt::text_style &ts, const std::string &level, const char *projectName);

        [[maybe_unused]] void print(const fmt::text_style &ts, const std::string &level, const std::string &fmt, auto && ... args) {
            if (isLoggingSuspended()) [[unlikely]]
                return;

            lockLoggerMutex();
            ON_SCOPE_EXIT { unlockLoggerMutex(); };

            auto dest = getDestination();
            try {
                auto message = fmt::format(fmt::runtime(fmt), args...);

                printPrefix(dest, ts, level, FENESTRA_PROJECT_NAME);
                fmt::print(dest, "{}\n", message);
                fflush(dest);

                addLogEntry(FENESTRA_PROJECT_NAME, level, std::move(message));
            } catch (const std::exception&) {
                fflush(dest);
            }
        }

        template<typename T>
        struct DebugFormattable {
            DebugFormattable(T& value) : value(value) { }

            T& value;
        };

        template<typename T>
        auto format_as(const DebugFormattable<T> &formattable) {
            if constexpr (std::convertible_to<T, std::string>)
                return fmt::format("{:?}", formattable.value);
            else if constexpr (fmt::is_formattable<T>::value)
                return fmt::format("{}", formattable.value);
            else if constexpr (std::is_enum_v<T>)
                return fmt::format("{}({})", wolv::type::getTypeName<T>(), std::to_underlying(formattable.value));
            else if constexpr (std::is_pointer_v<T>)
                return fmt::format("{}(0x{:016X})", wolv::type::getTypeName<T>(), std::uintptr_t(formattable.value));
            else
                return fmt::format("{} {{ ??? }}", wolv::type::getTypeName<T>());
        }

        namespace color {

            fmt::color debug();
            fmt::color info();
            fmt::color warn();
            fmt::color error();
            fmt::color fatal();

        }

    }

    void suspendLogging();
    void resumeLogging();
    void enableDebugLogging();

    [[maybe_unused]] void debug(const std::string &fmt, auto && ... args) {
        if (impl::isDebugLoggingEnabled()) [[unlikely]] {
            log::impl::print(fg(impl::color::debug()) | fmt::emphasis::bold, "[DEBUG]", fmt, args...);
        } else {
            impl::addLogEntry(FENESTRA_PROJECT_NAME, "[DEBUG]", fmt::format(fmt::runtime(fmt), args...));
        }
    }

    [[maybe_unused]] void info(const std::string &fmt, auto && ... args) {
        log::impl::print(fg(impl::color::info()) | fmt::emphasis::bold, "[INFO] ", fmt, args...);
    }

    [[maybe_unused]] void warn(const std::string &fmt, auto && ... args) {
        log::impl::print(fg(impl::color::warn()) | fmt::emphasis::bold, "[WARN] ", fmt, args...);
    }

    [[maybe_unused]] void error(const std::string &fmt, auto && ... args) {
        log::impl::print(fg(impl::color::error()) | fmt::emphasis::bold, "[ERROR]", fmt, args...);
    }

    [[maybe_unused]] void fatal(const std::string &fmt, auto && ... args) {
        log::impl::print(fg(impl::color::fatal()) | fmt::emphasis::bold, "[FATAL]", fmt, args...);
    }

    [[maybe_unused]] void print(const std::string &fmt, auto && ... args) {
        impl::lockLoggerMutex();
        ON_SCOPE_EXIT { impl::unlockLoggerMutex(); };

        try {
            auto dest = impl::getDestination();
            auto message = fmt::format(fmt::runtime(fmt), args...);
            fmt::print(dest, "{}", message);
            fflush(dest);
        } catch (const std::exception&) { }
    }

    [[maybe_unused]] void println(const std::string &fmt, auto && ... args) {
        impl::lockLoggerMutex();
        ON_SCOPE_EXIT { impl::unlockLoggerMutex(); };

        try {
            auto dest = impl::getDestination();
            auto message = fmt::format(fmt::runtime(fmt), args...);
            fmt::print(dest, "{}\n", message);
            fflush(dest);
        } catch (const std::exception&) { }
    }

}
