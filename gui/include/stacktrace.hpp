#pragma once

#include <fenestra.hpp>

#include <string>
#include <vector>

namespace fene::stacktrace {

    struct StackFrame {
        std::string file;
        std::string function;
        u32 line;
    };

    void initialize();

    struct StackTraceResult {
        std::vector<StackFrame> stackFrames;
        std::string implementationName;
    };

    StackTraceResult getStackTrace(); 

}