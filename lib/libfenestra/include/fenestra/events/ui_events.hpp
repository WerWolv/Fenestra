#pragma once

#include <fenestra.hpp>
#include <fenestra/api/event_manager.hpp>
#include <fenestra/ui/view.hpp>

#include <imgui.h>

EVENT_DEF_NO_LOG(EventImGuiElementRendered, ImGuiID, std::array<float, 4>);
EVENT_DEF(EventSearchBoxClicked, ImGuiMouseButton);

EVENT_DEF_NO_LOG(EventFrameBegin);
EVENT_DEF_NO_LOG(EventFrameEnd);

EVENT_DEF(EventViewOpened, fene::View*);

EVENT_DEF(RequestOpenPopup, std::string);
