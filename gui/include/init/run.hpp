#pragma once

#include <init/splash_window.hpp>

namespace fene::init {

    std::unique_ptr<WindowSplash> initializeApplication();
    void deinitializeApplication();

}
