#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>

#include <wolv/io/fs.hpp>

namespace fene::fs {

    void openFileExternal(const std::fs::path &filePath);
    void openFolderExternal(const std::fs::path &dirPath);
    void openFolderWithSelectionExternal(const std::fs::path &selectedFilePath);

    bool isPathWritable(const std::fs::path &path);

}