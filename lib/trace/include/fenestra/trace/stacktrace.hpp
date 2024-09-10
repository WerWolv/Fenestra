#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace fene::trace {

    struct StackFrame {
        std::string file;
        std::string function;
        std::uint32_t line;
    };

    void initialize();

    struct StackTrace {
        std::vector<StackFrame> stackFrames;
        std::string implementationName;
    };

    StackTrace getStackTrace();

}