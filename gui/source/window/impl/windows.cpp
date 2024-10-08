
#include <fenestra/api/theme_manager.hpp>

#include "window.hpp"


#if defined(OS_WINDOWS)

    #include "messaging.hpp"

    #include <fenestra/helpers/utils.hpp>
    #include <fenestra/api/fenestra_manager.hpp>
    #include <fenestra/helpers/logger.hpp>
    #include <fenestra/helpers/default_paths.hpp>
    #include <fenestra/events/system_events.hpp>
    #include <fenestra/events/ui_events.hpp>

    #include <imgui.h>
    #include <imgui_internal.h>

    #include <windows.h>
    #include <winbase.h>
    #include <winuser.h>
    #include <dwmapi.h>
    #include <windowsx.h>
    #include <shobjidl.h>
    #include <wrl/client.h>
    #include <fcntl.h>
    #include <shellapi.h>
    #include <timeapi.h>

    #include <cstdio>

namespace fene {

    template<typename T>
    using WinUniquePtr = std::unique_ptr<std::remove_pointer_t<T>, BOOL(*)(T)>;

    static LONG_PTR s_oldWndProc;
    static float s_titleBarHeight;
    static Microsoft::WRL::ComPtr<ITaskbarList4> s_taskbarList;

    void nativeErrorMessage(const std::string &message) {
        log::fatal(message);
        MessageBoxA(nullptr, message.c_str(), "Error", MB_ICONERROR | MB_OK);
    }

    // Custom Window procedure for receiving OS events
    static LRESULT commonWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_COPYDATA: {
                // Handle opening files in existing instance

                auto message = reinterpret_cast<COPYDATASTRUCT *>(lParam);
                if (message == nullptr) break;

                ssize_t nullIndex = -1;

                auto messageData = static_cast<const char*>(message->lpData);
                size_t messageSize = message->cbData;

                for (size_t i = 0; i < messageSize; i++) {
                    if (messageData[i] == '\0') {
                        nullIndex = i;
                        break;
                    }
                }

                if (nullIndex == -1) {
                    log::warn("Received invalid forwarded event");
                    break;
                }

                std::string eventName(messageData, nullIndex);

                std::vector<u8> eventData(messageData + nullIndex + 1, messageData + messageSize);

                messaging::messageReceived(eventName, eventData);
                break;
            }
            default:
                break;
        }

        return CallWindowProc((WNDPROC)s_oldWndProc, hwnd, uMsg, wParam, lParam);
    }

    // Custom window procedure for borderless window
    static LRESULT borderlessWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_MOUSELAST:
                break;
            case WM_NCACTIVATE:
            case WM_NCPAINT:
                // Handle Windows Aero Snap
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            case WM_NCCALCSIZE: {
                // Handle window resizing

                RECT &rect  = *reinterpret_cast<RECT *>(lParam);
                RECT client = rect;

                CallWindowProc((WNDPROC)s_oldWndProc, hwnd, uMsg, wParam, lParam);

                if (IsMaximized(hwnd)) {
                    WINDOWINFO windowInfo = { };
                    windowInfo.cbSize = sizeof(WINDOWINFO);
                    GetWindowInfo(hwnd, &windowInfo);

                    rect = RECT {
                        .left   = static_cast<LONG>(client.left + windowInfo.cyWindowBorders),
                        .top    = static_cast<LONG>(client.top + windowInfo.cyWindowBorders),
                        .right  = static_cast<LONG>(client.right - windowInfo.cyWindowBorders),
                        .bottom = static_cast<LONG>(client.bottom - windowInfo.cyWindowBorders) + 1
                    };
                } else {
                    rect = client;
                }

                // This code tries to avoid DWM flickering when resizing the window
                // It's not perfect, but it's really the best we can do.

                LARGE_INTEGER performanceFrequency = {};
                QueryPerformanceFrequency(&performanceFrequency);
                TIMECAPS tc = {};
                timeGetDevCaps(&tc, sizeof(tc));

                const auto granularity = tc.wPeriodMin;
                timeBeginPeriod(tc.wPeriodMin);

                DWM_TIMING_INFO dti = {};
                dti.cbSize = sizeof(dti);
                ::DwmGetCompositionTimingInfo(nullptr, &dti);

                LARGE_INTEGER end = {};
                QueryPerformanceCounter(&end);

                const auto period = dti.qpcRefreshPeriod;
                const i64 delta = dti.qpcVBlank - end.QuadPart;

                i64 sleepTicks = 0;
                i64 sleepMilliSeconds = 0;
                if (delta >= 0) {
                    sleepTicks = delta / period;
                } else {

                    sleepTicks = -1 + delta / period;
                }

                sleepMilliSeconds = delta - (period * sleepTicks);
                const double sleepTime = (1000.0 * double(sleepMilliSeconds) / double(performanceFrequency.QuadPart));
                Sleep(DWORD(std::round(sleepTime)));
                timeEndPeriod(granularity);

                return WVR_REDRAW;
            }
            case WM_ERASEBKGND:
                return 1;
            case WM_WINDOWPOSCHANGING: {
                // Make sure that windows discards the entire client area when resizing to avoid flickering
                const auto windowPos = reinterpret_cast<LPWINDOWPOS>(lParam);
                windowPos->flags |= SWP_NOCOPYBITS;
                break;
            }
            case WM_NCHITTEST: {
                // Handle window resizing and moving

                POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

                const POINT border {
                    static_cast<LONG>((::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)) * FenestraManager::System::getGlobalScale()),
                    static_cast<LONG>((::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)) * FenestraManager::System::getGlobalScale())
                };

                if (SDL_GetDisplayForWindow(static_cast<SDL_Window*>(FenestraManager::System::getMainWindowHandle())) == 0) {
                    return HTCLIENT;
                }

                RECT window;
                if (!::GetWindowRect(hwnd, &window)) {
                    return HTNOWHERE;
                }

                constexpr static auto RegionClient = 0b0000;
                constexpr static auto RegionLeft   = 0b0001;
                constexpr static auto RegionRight  = 0b0010;
                constexpr static auto RegionTop    = 0b0100;
                constexpr static auto RegionBottom = 0b1000;

                const auto result =
                    RegionLeft * (cursor.x < (window.left + border.x)) |
                    RegionRight * (cursor.x >= (window.right - border.x)) |
                    RegionTop * (cursor.y < (window.top + border.y)) |
                    RegionBottom * (cursor.y >= (window.bottom - border.y));

                if (result != 0 && (ImGui::IsAnyItemHovered() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))) {
                    break;
                }

                std::string_view hoveredWindowName = GImGui->HoveredWindow == nullptr ? "" : GImGui->HoveredWindow->Name;

                if (!FenestraManager::System::impl::isWindowResizable()) {
                    if (result != RegionClient) {
                        return HTCAPTION;
                    }
                }

                switch (result) {
                    case RegionLeft:
                        return HTLEFT;
                    case RegionRight:
                        return HTRIGHT;
                    case RegionTop:
                        return HTTOP;
                    case RegionBottom:
                        return HTBOTTOM;
                    case RegionTop | RegionLeft:
                        return HTTOPLEFT;
                    case RegionTop | RegionRight:
                        return HTTOPRIGHT;
                    case RegionBottom | RegionLeft:
                        return HTBOTTOMLEFT;
                    case RegionBottom | RegionRight:
                        return HTBOTTOMRIGHT;
                    case RegionClient:
                    default:
                        if (cursor.y < (window.top + s_titleBarHeight * 2)) {
                            if (hoveredWindowName == "##MainMenuBar" || hoveredWindowName == "FenestraDockSpace") {
                                if (!ImGui::IsAnyItemHovered()) {
                                    return HTCAPTION;
                                }
                            }
                        }

                        break;
                }
                break;
            }
            default:
                break;
        }

        return commonWindowProc(hwnd, uMsg, wParam, lParam);
    }


    [[maybe_unused]]
    static void reopenConsoleHandle(u32 stdHandleNumber, i32 stdFileDescriptor, FILE *stdStream) {
        // Get the Windows handle for the standard stream
        HANDLE handle = ::GetStdHandle(stdHandleNumber);

        // Redirect the standard stream to the relevant console stream
        if (stdFileDescriptor == STDIN_FILENO)
            freopen("CONIN$", "rt", stdStream);
        else
            freopen("CONOUT$", "wt", stdStream);

        // Disable buffering
        setvbuf(stdStream, nullptr, _IONBF, 0);

        // Reopen the standard stream as a file descriptor
        auto unboundFd = [stdFileDescriptor, handle]{
            if (stdFileDescriptor == STDIN_FILENO)
                return _open_osfhandle(intptr_t(handle), _O_RDONLY);
            else
                return _open_osfhandle(intptr_t(handle), _O_WRONLY);
        }();

        // Duplicate the file descriptor to the standard stream
        dup2(unboundFd, stdFileDescriptor);
    }

    void enumerateFonts() {
        constexpr static auto FontRegistryPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

        static const std::array RegistryLocations = {
            HKEY_LOCAL_MACHINE,
            HKEY_CURRENT_USER
        };

        for (const auto location : RegistryLocations) {
            HKEY key;
            if (RegOpenKeyExW(location, FontRegistryPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
                continue;
            }

            DWORD index = 0;
            std::wstring valueName(0xFFF, L'\0');
            DWORD valueNameSize = valueName.size() * sizeof(wchar_t);
            std::wstring valueData(0xFFF, L'\0');
            DWORD valueDataSize = valueData.size() * sizeof(wchar_t);
            DWORD valueType;

            while (RegEnumValueW(key, index, valueName.data(), &valueNameSize, nullptr, &valueType, reinterpret_cast<BYTE *>(valueData.data()), &valueDataSize) == ERROR_SUCCESS) {
                if (valueType == REG_SZ) {
                    auto fontName = utf16ToUtf8(valueName.c_str());
                    auto fontPath = std::fs::path(valueData);
                    if (fontPath.is_relative())
                        fontPath = std::fs::path("C:\\Windows\\Fonts") / fontPath;

                    registerFont(fontName.c_str(), wolv::util::toUTF8String(fontPath).c_str());
                }

                valueNameSize = valueName.size();
                valueDataSize = valueData.size();
                index++;
            }

            RegCloseKey(key);
        }
    }

    void Window::configureBackend() {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        // Windows versions before Windows 10 have issues with transparent framebuffers
        // causing the entire window to be slightly transparent ignoring all configurations
        OSVERSIONINFOA versionInfo = { };
        if (::GetVersionExA(&versionInfo) && versionInfo.dwMajorVersion >= 10) {
            //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        } else {
            //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
        }
    }


    void Window::initNative() {
        // Setup DPI Awareness
        {
            using SetProcessDpiAwarenessContextFunc = HRESULT(WINAPI *)(DPI_AWARENESS_CONTEXT);

            SetProcessDpiAwarenessContextFunc SetProcessDpiAwarenessContext =
                (SetProcessDpiAwarenessContextFunc)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext");

            if (SetProcessDpiAwarenessContext != nullptr) {
                SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            }
        }

        if (FenestraManager::System::isDebugBuild()) {
            // If the application is running in debug mode, Fenestra applications run under the CONSOLE subsystem,
            // so we don't need to do anything besides enabling ANSI colors
            log::impl::enableColorPrinting();
        } else if (getEnvironmentVariable("__FENESTRA_FORWARD_CONSOLE__") == "1") {
            // Check for the __FENESTRA_FORWARD_CONSOLE__ environment variable that was set by the forwarder application

            // If it's present, attach to its console window
            ::AttachConsole(ATTACH_PARENT_PROCESS);

            // Reopen stdin, stdout and stderr to the console if not in debug mode
            reopenConsoleHandle(STD_INPUT_HANDLE,  STDIN_FILENO,  stdin);
            reopenConsoleHandle(STD_OUTPUT_HANDLE, STDOUT_FILENO, stdout);

            // Explicitly don't forward stderr because some libraries like to write to it
            // with no way to disable it (e.g., libmagic)
            /*
                reopenConsoleHandle(STD_ERROR_HANDLE,  STDERR_FILENO, stderr);
            */

            // Enable ANSI colors in the console
            log::impl::enableColorPrinting();
        } else {
            log::impl::redirectToFile();
        }

        // Add plugin library folders to dll search path
        for (const auto &path : paths::Libraries.read())  {
            if (std::fs::exists(path))
                AddDllDirectory(path.c_str());
        }

        enumerateFonts();
    }

    void Window::setupNativeWindow() {
        // Setup borderless window
        auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));

        CoInitialize(nullptr);
        OleInitialize(nullptr);

        bool borderlessWindowMode = FenestraManager::System::isBorderlessWindowModeEnabled();

        // Set up the correct window procedure based on the borderless window mode state
        if (borderlessWindowMode) {
            s_oldWndProc = ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)borderlessWindowProc);

            MARGINS borderless = { 1, 1, 1, 1 };
            ::DwmExtendFrameIntoClientArea(hwnd, &borderless);

            DWORD attribute = DWMNCRP_ENABLED;
            ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &attribute, sizeof(attribute));

            ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE);
        } else {
            s_oldWndProc = ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)commonWindowProc);
        }

        // Set up a taskbar progress handler
        {
            if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
                CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList4, &s_taskbarList);
            }

            EventSetTaskBarIconState::subscribe([hwnd](u32 state, u32 type, u32 progress){
                using enum FenestraManager::System::TaskProgressState;
                switch (FenestraManager::System::TaskProgressState(state)) {
                    case Reset:
                        s_taskbarList->SetProgressState(hwnd, TBPF_NOPROGRESS);
                        s_taskbarList->SetProgressValue(hwnd, 0, 0);
                        break;
                    case Flash:
                        FlashWindow(hwnd, true);
                        break;
                    case Progress:
                        s_taskbarList->SetProgressState(hwnd, TBPF_INDETERMINATE);
                        s_taskbarList->SetProgressValue(hwnd, progress, 100);
                        break;
                }

                using enum FenestraManager::System::TaskProgressType;
                switch (FenestraManager::System::TaskProgressType(type)) {
                    case Normal:
                        s_taskbarList->SetProgressState(hwnd, TBPF_NORMAL);
                        break;
                    case Warning:
                        s_taskbarList->SetProgressState(hwnd, TBPF_PAUSED);
                        break;
                    case Error:
                        s_taskbarList->SetProgressState(hwnd, TBPF_ERROR);
                        break;
                }
            });
        }

        struct ACCENTPOLICY {
            u32 accentState;
            u32 accentFlags;
            u32 gradientColor;
            u32 animationId;
        };
        struct WINCOMPATTRDATA {
            int attribute;
            PVOID pData;
            ULONG dataSize;
        };

        EventThemeChanged::subscribe([this, hwnd]{
            static auto user32Dll = WinUniquePtr<HMODULE>(LoadLibraryA("user32.dll"), FreeLibrary);
            if (user32Dll != nullptr) {
                using SetWindowCompositionAttributeFunc = BOOL(WINAPI*)(HWND, WINCOMPATTRDATA*);

                const auto SetWindowCompositionAttribute =
                        (SetWindowCompositionAttributeFunc)
                        (void*)
                        GetProcAddress(user32Dll.get(), "SetWindowCompositionAttribute");

                if (SetWindowCompositionAttribute != nullptr) {
                    ACCENTPOLICY policy = { ImGuiExt::GetCustomStyle().WindowBlur > 0.5F ? 4U : 0U, 0, ImGuiExt::GetCustomColorU32(ImGuiCustomCol_BlurBackground), 0 };
                    WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) };
                    SetWindowCompositionAttribute(hwnd, &data);
                }
            }
        });
        RequestChangeTheme::subscribe([this](const std::string &theme) {
            const int immersiveDarkMode = 20;
            auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));

            BOOL value = theme == "Dark" ? TRUE : FALSE;

            // Using the C++ "bool" type seems to not work correctly.
            DwmSetWindowAttribute(hwnd, immersiveDarkMode, &value, sizeof(value));
        });

        ImGui::GetIO().ConfigDebugIsDebuggerPresent = ::IsDebuggerPresent();

        DwmEnableMMCSS(TRUE);
        
        {
            constexpr BOOL value = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_ENABLED, &value, sizeof(value));
        }
        {
            constexpr DWMNCRENDERINGPOLICY value = DWMNCRP_ENABLED;
            DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &value, sizeof(value));
        }

        // Remove WS_POPUP style from the window to make various window management tools work
        ::SetWindowLong(hwnd, GWL_STYLE, (GetWindowLong(hwnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
        ::SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_COMPOSITED | WS_EX_LAYERED);

        EventFrameBegin::subscribe(this, [this] {
            ImGui::Begin("FenestraDockSpace");
            s_titleBarHeight = ImGui::GetCurrentWindowRead()->MenuBarHeight;
            ImGui::End();
        });
    }

}

#endif