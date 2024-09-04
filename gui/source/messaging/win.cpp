#if defined(OS_WINDOWS)

#include "messaging.hpp"

#include <fenestra/helpers/logger.hpp>

#include <windows.h>

namespace fene::messaging {

    std::optional<HWND> getApplicationWindow() {
        HWND fenestraWindow = nullptr;
        
        ::EnumWindows([](HWND hWnd, LPARAM ret) -> BOOL {
            // Get the window name
            auto length = ::GetWindowTextLength(hWnd);
            std::string windowName(length + 1, '\x00');
            ::GetWindowTextA(hWnd, windowName.data(), windowName.size());

            // Check if the window is visible and if it's another window of this application
            if (::IsWindowVisible(hWnd) == TRUE && length != 0) {
                if (windowName.starts_with(FENESTRA_APPLICATION_NAME)) {
                    // It's our window, return it and stop iteration
                    *reinterpret_cast<HWND*>(ret) = hWnd;
                    return FALSE;
                }
            }

            // Continue iteration
            return TRUE;

        }, reinterpret_cast<LPARAM>(&fenestraWindow));

        if (fenestraWindow == nullptr)
            return { };
        else
            return fenestraWindow;
    }

    void sendToOtherInstance(const std::string &eventName, const std::vector<u8> &args) {
        log::debug("Sending event {} to another instance (not us)", eventName);

        const auto window = getApplicationWindow();
        if (!window.has_value()) {
            log::error("Cannot get window handle of other instance");
            return;
        }

        // Get the window we want to send it to
        HWND fenestraWindow = *window;

        // Create the message
        std::vector<u8> fulleventData(eventName.begin(), eventName.end());
        fulleventData.push_back('\0');

        fulleventData.insert(fulleventData.end(), args.begin(), args.end());
        
        u8 *data = &fulleventData[0];
        DWORD dataSize = fulleventData.size();

        COPYDATASTRUCT message = {
            .dwData = 0,
            .cbData = dataSize,
            .lpData = data
        };

        // Send the message
        SendMessage(fenestraWindow, WM_COPYDATA, reinterpret_cast<WPARAM>(fenestraWindow), reinterpret_cast<LPARAM>(&message));
    }

    bool setupNative() {
        constexpr static auto UniqueMutexId = WOLV_TOKEN_CONCAT(L,  FENESTRA_APPLICATION_NAME) L"/a477ea68-e334-4d07-a439-4f159c683763";

        // Check if an application instance is already running by opening a global mutex
        HANDLE globalMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, UniqueMutexId);
        if (globalMutex == nullptr) {
            // If no application instance is running, create a new global mutex
            globalMutex = CreateMutexW(nullptr, FALSE, UniqueMutexId);
            return true;
        } else {
            return false;
        }
    }
}

#endif
