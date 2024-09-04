#pragma once

#include <fenestra.hpp>
#include <fenestra/api/event_manager.hpp>

EVENT_DEF(RequestAddInitTask, std::string, bool, std::function<bool()>);
EVENT_DEF(RequestAddExitTask, std::string, std::function<bool()>);

EVENT_DEF(RequestUpdateWindowTitle);
EVENT_DEF(RequestInitThemeHandlers);