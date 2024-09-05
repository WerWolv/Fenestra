#include <fenestra/helpers/utils.hpp>

#include <fenestra/api/fenestra_manager.hpp>

#include <fenestra/helpers/fmt.hpp>
#include <fenestra/helpers/logger.hpp>

#include <imgui.h>

#if defined(OS_WINDOWS)
    #include <windows.h>
    #include <shellapi.h>

    #include <wolv/utils/guards.hpp>
#elif defined(OS_LINUX)
    #include <unistd.h>
    #include <dlfcn.h>
    #include <fenestra/helpers/utils_linux.hpp>
#elif defined(OS_MACOS)
    #include <unistd.h>
    #include <dlfcn.h>
    #include <fenestra/helpers/utils_macos.hpp>
#elif defined(OS_WEB)
    #include "emscripten.h"
#endif

namespace fene {

    float operator""_scaled(long double value) {
        return value * FenestraApi::System::getGlobalScale();
    }

    float operator""_scaled(unsigned long long value) {
        return value * FenestraApi::System::getGlobalScale();
    }

    ImVec2 scaled(const ImVec2 &vector) {
        return vector * FenestraApi::System::getGlobalScale();
    }

    ImVec2 scaled(float x, float y) {
        return ImVec2(x, y) * FenestraApi::System::getGlobalScale();
    }

    std::string to_string(u128 value) {
        char data[45] = { 0 };

        u8 index = sizeof(data) - 2;
        while (value != 0 && index != 0) {
            data[index] = '0' + value % 10;
            value /= 10;
            index--;
        }

        return { data + index + 1 };
    }

    std::string to_string(i128 value) {
        char data[45] = { 0 };

        u128 unsignedValue = value < 0 ? -value : value;

        u8 index = sizeof(data) - 2;
        while (unsignedValue != 0 && index != 0) {
            data[index] = '0' + unsignedValue % 10;
            unsignedValue /= 10;
            index--;
        }

        if (value < 0) {
            data[index] = '-';
            return { data + index };
        } else {
            return { data + index + 1 };
        }
    }

    std::string toLower(std::string string) {
        for (char &c : string)
            c = std::tolower(c);

        return string;
    }

    std::string toUpper(std::string string) {
        for (char &c : string)
            c = std::toupper(c);

        return string;
    }

    std::vector<std::string> splitString(const std::string &string, const std::string &delimiter) {
        size_t start = 0, end = 0;
        std::vector<std::string> res;

        while ((end = string.find(delimiter, start)) != std::string::npos) {
            size_t size = end - start;
            if (start + size > string.length())
                break;

            std::string token = string.substr(start, end - start);
            start = end + delimiter.length();
            res.push_back(token);
        }

        res.emplace_back(string.substr(start));
        return res;
    }

    std::string combineStrings(const std::vector<std::string> &strings, const std::string &delimiter) {
        std::string result;
        for (const auto &string : strings) {
            result += string;
            result += delimiter;
        }

        return result.substr(0, result.length() - delimiter.length());
    }

    std::string replaceStrings(std::string string, const std::string &search, const std::string &replace) {
        if (search.empty())
            return string;

        std::size_t pos;
        while ((pos = string.find(search)) != std::string::npos)
            string.replace(pos, search.size(), replace);

        return string;
    }

    void startProgram(const std::string &command) {

        #if defined(OS_WINDOWS)
            std::ignore = system(fmt::format("start {0}", command).c_str());
        #elif defined(OS_MACOS)
            std::ignore = system(fmt::format("open {0}", command).c_str());
        #elif defined(OS_LINUX)
            executeCmd({"xdg-open", command});
        #elif defined(OS_WEB)
            std::ignore = command;
        #endif
    }

    int executeCommand(const std::string &command) {
        return ::system(command.c_str());
    }

    void openWebpage(std::string url) {
        if (!url.contains("://"))
            url = "https://" + url;

        #if defined(OS_WINDOWS)
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        #elif defined(OS_MACOS)
            openWebpageMacos(url.c_str());
        #elif defined(OS_LINUX)
            executeCmd({"xdg-open", url});
        #elif defined(OS_WEB)
            EM_ASM({
                window.open(UTF8ToString($0), '_blank');
            }, url.c_str());
        #else
            #warning "Unknown OS, can't open webpages"
        #endif
    }

    std::optional<u8> hexCharToValue(char c) {
        if (std::isdigit(c))
            return c - '0';
        else if (std::isxdigit(c))
            return std::toupper(c) - 'A' + 0x0A;
        else
            return { };
    }

    std::wstring utf8ToUtf16(const std::string& utf8) {
        std::vector<u32> unicodes;

        for (size_t byteIndex = 0; byteIndex < utf8.size();) {
            u32 unicode = 0;
            size_t unicodeSize = 0;

            u8 ch = utf8[byteIndex];
            byteIndex += 1;

            if (ch <= 0x7F) {
                unicode = ch;
                unicodeSize = 0;
            } else if (ch <= 0xBF) {
                return { };
            } else if (ch <= 0xDF) {
                unicode = ch&0x1F;
                unicodeSize = 1;
            } else if (ch <= 0xEF) {
                unicode = ch&0x0F;
                unicodeSize = 2;
            } else if (ch <= 0xF7) {
                unicode = ch&0x07;
                unicodeSize = 3;
            } else {
                return { };
            }

            for (size_t unicodeByteIndex = 0; unicodeByteIndex < unicodeSize; unicodeByteIndex += 1) {
                if (byteIndex == utf8.size())
                    return { };

                u8 byte = utf8[byteIndex];
                if (byte < 0x80 || byte > 0xBF)
                    return { };

                unicode <<= 6;
                unicode += byte & 0x3F;

                byteIndex += 1;
            }

            if (unicode >= 0xD800 && unicode <= 0xDFFF)
                return { };
            if (unicode > 0x10FFFF)
                return { };

            unicodes.push_back(unicode);
        }

        std::wstring utf16;

        for (auto unicode : unicodes) {
            if (unicode <= 0xFFFF) {
                utf16 += static_cast<wchar_t>(unicode);
            } else {
                unicode -= 0x10000;
                utf16 += static_cast<wchar_t>(((unicode >> 10) + 0xD800));
                utf16 += static_cast<wchar_t>(((unicode & 0x3FF) + 0xDC00));
            }
        }
        return utf16;
    }

    std::string utf16ToUtf8(const std::wstring& utf16) {
        std::vector<u32> unicodes;

        for (size_t index = 0; index < utf16.size();) {
            u32 unicode = 0;

            wchar_t wch = utf16[index];
            index += 1;

            if (wch < 0xD800 || wch > 0xDFFF) {
                unicode = static_cast<u32>(wch);
            } else if (wch >= 0xD800 && wch <= 0xDBFF) {
                if (index == utf16.size())
                    return "";

                wchar_t nextWch = utf16[index];
                index += 1;

                if (nextWch < 0xDC00 || nextWch > 0xDFFF)
                    return "";

                unicode = static_cast<u32>(((wch - 0xD800) << 10) + (nextWch - 0xDC00) + 0x10000);
            } else {
                return "";
            }

            unicodes.push_back(unicode);
        }

        std::string utf8;

        for (auto unicode : unicodes) {
            if (unicode <= 0x7F) {
                utf8 += static_cast<char>(unicode);
            } else if (unicode <= 0x7FF) {
                utf8 += static_cast<char>(0xC0 | ((unicode >> 6) & 0x1F));
                utf8 += static_cast<char>(0x80 | (unicode & 0x3F));
            } else if (unicode <= 0xFFFF) {
                utf8 += static_cast<char>(0xE0 | ((unicode >> 12) & 0x0F));
                utf8 += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (unicode & 0x3F));
            } else if (unicode <= 0x10FFFF) {
                utf8 += static_cast<char>(0xF0 | ((unicode >> 18) & 0x07));
                utf8 += static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
                utf8 += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (unicode & 0x3F));
            } else {
                return "";
            }
        }

        return utf8;
    }

    float float16ToFloat32(u16 float16) {
        u32 sign     = float16 >> 15;
        u32 exponent = (float16 >> 10) & 0x1F;
        u32 mantissa = float16 & 0x3FF;

        u32 result = 0x00;

        if (exponent == 0) {
            if (mantissa == 0) {
                // +- Zero
                result = sign << 31;
            } else {
                // Subnormal value
                exponent = 0x7F - 14;

                while ((mantissa & (1 << 10)) == 0) {
                    exponent--;
                    mantissa <<= 1;
                }

                mantissa &= 0x3FF;
                result = (sign << 31) | (exponent << 23) | (mantissa << 13);
            }
        } else if (exponent == 0x1F) {
            // +-Inf or +-NaN
            result = (sign << 31) | (0xFF << 23) | (mantissa << 13);
        } else {
            // Normal value
            result = (sign << 31) | ((exponent + (0x7F - 15)) << 23) | (mantissa << 13);
        }

        float floatResult = 0;
        std::memcpy(&floatResult, &result, sizeof(float));

        return floatResult;
    }

    bool isProcessElevated() {
        #if defined(OS_WINDOWS)
            bool elevated = false;
            HANDLE token  = INVALID_HANDLE_VALUE;

            if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)) {
                TOKEN_ELEVATION elevation;
                DWORD elevationSize = sizeof(TOKEN_ELEVATION);

                if (::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &elevationSize))
                    elevated = elevation.TokenIsElevated;
            }

            if (token != INVALID_HANDLE_VALUE)
                ::CloseHandle(token);

            return elevated;
        #elif defined(OS_LINUX) || defined(OS_MACOS)
            return getuid() == 0 || getuid() != geteuid();
        #else
            return false;
        #endif
    }

    std::optional<std::string> getEnvironmentVariable(const std::string &env) {
        auto value = std::getenv(env.c_str());

        if (value == nullptr)
            return std::nullopt;
        else
            return value;
    }

    [[nodiscard]] std::string limitStringLength(const std::string &string, size_t maxLength) {
        // If the string is shorter than the max length, return it as is
        if (string.size() < maxLength)
            return string;

        // If the string is longer than the max length, find the last space before the max length
        auto it = string.begin() + maxLength / 2;
        while (it != string.begin() && !std::isspace(*it)) --it;

        // If there's no space before the max length, just cut the string
        if (it == string.begin()) {
            it = string.begin() + maxLength / 2;

            // Try to find a UTF-8 character boundary
            while (it != string.begin() && (*it & 0xC0) == 0x80) --it;
        }

        // If we still didn't find a valid boundary, just return the string as is
        if (it == string.begin())
            return string;

        auto result = std::string(string.begin(), it) + "…";

        // If the string is longer than the max length, find the last space before the max length
        it = string.end() - 1 - maxLength / 2;
        while (it != string.end() && !std::isspace(*it)) ++it;

        // If there's no space before the max length, just cut the string
        if (it == string.end()) {
            it = string.end() - 1 - maxLength / 2;

            // Try to find a UTF-8 character boundary
            while (it != string.end() && (*it & 0xC0) == 0x80) ++it;
        }

        return result + std::string(it, string.end());
    }

    static std::optional<std::fs::path> s_fileToOpen;
    extern "C" void openFile(const char *path) {
        log::info("Opening file: {0}", path);
        s_fileToOpen = path;
    }

    std::optional<std::fs::path> getInitialFilePath() {
        return s_fileToOpen;
    }

    static std::map<std::fs::path, std::string> s_fonts;
    extern "C" void registerFont(const char *fontName, const char *fontPath) {
        s_fonts[fontPath] = fontName;
    }

    const std::map<std::fs::path, std::string>& getFonts() {
        return s_fonts;
    }

    std::string formatSystemError(i32 error) {
        #if defined(OS_WINDOWS)
            wchar_t *message = nullptr;
            auto wLength = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (wchar_t*)&message, 0,
                nullptr
            );
            ON_SCOPE_EXIT { LocalFree(message); };

            auto length = ::WideCharToMultiByte(CP_UTF8, 0, message, wLength, nullptr, 0, nullptr, nullptr);
            std::string result(length, '\x00');
            ::WideCharToMultiByte(CP_UTF8, 0, message, wLength, result.data(), length, nullptr, nullptr);

            return result;
        #else
            return std::system_category().message(error);
        #endif
    }


    void* getContainingModule(void* symbol) {
        #if defined(OS_WINDOWS)
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(symbol, &mbi, sizeof(mbi)))
                return mbi.AllocationBase;

            return nullptr;
        #elif !defined(OS_WEB)
            Dl_info info = {};
            if (dladdr(symbol, &info) == 0)
                return nullptr;

            return dlopen(info.dli_fname, RTLD_LAZY);
        #else
            std::ignore = symbol;
            return nullptr;
        #endif
    }

}