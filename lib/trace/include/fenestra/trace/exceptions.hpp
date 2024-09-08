#pragma once

#include <fenestra/trace/stacktrace.hpp>
#include <optional>

namespace fene::trace {

    std::optional<StackTrace> getLastExceptionStackTrace();

}