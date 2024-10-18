#pragma once

#include <fenestra.hpp>
#include <fenestra/api/event_manager.hpp>

#include <vector>
#include <string>

union SDL_Event;

EVENT_DEF(EventAbnormalTermination, int);
EVENT_DEF(EventApplicationClosing);
EVENT_DEF(EventApplicationStartupFinished);
EVENT_DEF(RequestSendMessageToMainInstance, const std::string &, const std::vector<fene::u8> &);
EVENT_DEF(EventThemeChanged);
EVENT_DEF(EventWindowFocused, bool);
EVENT_DEF(EventWindowClosing, void *, bool*);
EVENT_DEF(EventWindowInitialized);
EVENT_DEF(EventWindowDeinitializing, void*);
EVENT_DEF(EventDPIChanged, float, float);
EVENT_DEF(EventCrashRecovered, std::exception);
EVENT_DEF(EventOSThemeChanged);
EVENT_DEF(EventFileDragged, bool);
EVENT_DEF(EventFileDropped, std::fs::path);
EVENT_DEF_NO_LOG(EventSetTaskBarIconState, fene::u32, fene::u32, fene::u32);
EVENT_DEF_NO_LOG(EventBackendEventFired, const SDL_Event*);

EVENT_DEF(RequestChangeTheme, std::string);
EVENT_DEF(RequestCloseApplication, bool);
EVENT_DEF(RequestRestartApplication);
