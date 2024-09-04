#include <fenestra/helpers/fs.hpp>

#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/helpers/logger.hpp>
#include <fenestra/helpers/fmt.hpp>
#include <fenestra/helpers/utils_linux.hpp>
#include <fenestra/helpers/auto_reset.hpp>

#if defined(OS_WINDOWS)
    #include <windows.h>
    #include <shlobj.h>
    #include <shellapi.h>
#elif defined(OS_LINUX) || defined(OS_WEB)
    #include <xdg.hpp>
# if defined(OS_FREEBSD)
    #include <sys/syslimits.h>
# else
    #include <limits.h>
# endif
#endif

#if defined(OS_WEB)
    #include <emscripten.h>
#endif

#include <filesystem>

#include <wolv/io/file.hpp>
#include <wolv/io/fs.hpp>
#include <wolv/utils/string.hpp>

#include <fmt/format.h>
#include <fmt/xchar.h>

namespace fene::fs {

    static AutoReset<std::function<void(const std::string&)>> s_fileBrowserErrorCallback;
    void setFileBrowserErrorCallback(const std::function<void(const std::string&)> &callback) {
        s_fileBrowserErrorCallback = callback;
    }

    // With help from https://github.com/owncloud/client/blob/cba22aa34b3677406e0499aadd126ce1d94637a2/src/gui/openfilemanager.cpp
    void openFileExternal(const std::fs::path &filePath) {
        // Make sure the file exists before trying to open it
        if (!wolv::io::fs::exists(filePath)) {
            return;
        }

        #if defined(OS_WINDOWS)
            std::ignore = ShellExecuteW(nullptr, L"open", filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        #elif defined(OS_MACOS)
            std::ignore = system(
                fmt::format("open {}", wolv::util::toUTF8String(filePath)).c_str()
            );
        #elif defined(OS_LINUX)
            executeCmd({"xdg-open", wolv::util::toUTF8String(filePath)});
        #endif
    }

    void openFolderExternal(const std::fs::path &dirPath) {
        // Make sure the folder exists before trying to open it
        if (!wolv::io::fs::exists(dirPath)) {
            return;
        }

        #if defined(OS_WINDOWS)
            auto args = fmt::format(L"\"{}\"", dirPath.c_str());
            ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
        #elif defined(OS_MACOS)
            std::ignore = system(
                fmt::format("open {}", wolv::util::toUTF8String(dirPath)).c_str()
            );
        #elif defined(OS_LINUX)
            executeCmd({"xdg-open", wolv::util::toUTF8String(dirPath)});
        #endif
    }

    void openFolderWithSelectionExternal(const std::fs::path &selectedFilePath) {
        // Make sure the file exists before trying to open it
        if (!wolv::io::fs::exists(selectedFilePath)) {
            return;
        }

        #if defined(OS_WINDOWS)
            auto args = fmt::format(L"/select,\"{}\"", selectedFilePath.c_str());
            ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
        #elif defined(OS_MACOS)
            std::ignore = system(
                fmt::format(
                    R"(osascript -e 'tell application "Finder" to reveal POSIX file "{}"')",
                    wolv::util::toUTF8String(selectedFilePath)
                ).c_str()
            );
            system(R"(osascript -e 'tell application "Finder" to activate')");
        #elif defined(OS_LINUX)
            // Fallback to only opening the folder for now
            // TODO actually select the file
            executeCmd({"xdg-open", wolv::util::toUTF8String(selectedFilePath.parent_path())});
        #endif
    }

    bool isPathWritable(const std::fs::path &path) {
        constexpr static auto TestFileName = "__fenestra__tmp__";

        // Try to open the __fenestra__tmp__ file in the given path
        // If one does exist already, try to delete it
        {
            wolv::io::File file(path / TestFileName, wolv::io::File::Mode::Read);
            if (file.isValid()) {
                if (!file.remove())
                    return false;
            }
        }

        // Try to create a new file in the given path
        // If that fails, or the file cannot be deleted anymore afterward; the path is not writable
        wolv::io::File file(path / TestFileName, wolv::io::File::Mode::Create);
        const bool result = file.isValid();
        if (!file.remove())
            return false;

        return result;
    }

    std::fs::path toShortPath(const std::fs::path &path) {
        #if defined(OS_WINDOWS)
            // Get the size of the short path
            size_t size = GetShortPathNameW(path.c_str(), nullptr, 0);
            if (size == 0)
                return path;

            // Get the short path
            std::wstring newPath(size, 0x00);
            GetShortPathNameW(path.c_str(), newPath.data(), newPath.size());
            newPath.pop_back();

            return newPath;
        #else
            // Other supported platforms don't have short paths
            return path;
        #endif
    }


}
