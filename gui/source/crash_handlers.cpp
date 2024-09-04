#include <fenestra/api/task_manager.hpp>
#include <fenestra/api/fenestra_manager.hpp>

#include <fenestra/helpers/logger.hpp>
#include <fenestra/helpers/fs.hpp>

#include <fenestra/events/system_events.hpp>

#include <wolv/utils/string.hpp>

#include <window.hpp>
#include <init/tasks.hpp>
#include <stacktrace.hpp>

#include <nlohmann/json.hpp>

#include <csignal>
#include <exception>
#include <typeinfo>

#if defined (OS_MACOS)
    #include <sys/utsname.h>
#endif

namespace fene::crash {

    constexpr static auto Signals = { SIGSEGV, SIGILL, SIGABRT, SIGFPE };

    void resetCrashHandlers();
    
    static void sendNativeMessage(const std::string& message) {
        fene::nativeErrorMessage(fmt::format("Application crashed during initial setup!\nError: {}", message));
    }

    // Function that decides what should happen on a crash
    // (either sending a message or saving a crash file, depending on when the crash occurred)
    using CrashCallback = void (*) (const std::string&);
    static CrashCallback crashCallback = sendNativeMessage;

    static void printStackTrace() {
        auto stackTraceResult = stacktrace::getStackTrace();
        log::fatal("Printing stacktrace using implementation '{}'", stackTraceResult.implementationName);
        for (const auto &stackFrame : stackTraceResult.stackFrames) {
            if (stackFrame.line == 0)
                log::fatal("  ({}) | {}", stackFrame.file, stackFrame.function);
            else
                log::fatal("  ({}:{}) | {}",  stackFrame.file, stackFrame.line, stackFrame.function);
        }
    }

    extern "C" void triggerSafeShutdown(int signalNumber = 0) {
        // Trigger an event so that plugins can handle crashes
        EventAbnormalTermination::post(signalNumber);

        // Run exit tasks
        init::runExitTasks();

        // Terminate all asynchronous tasks
        TaskManager::exit();

        // Trigger a breakpoint if we're in a debug build or raise the signal again for the default handler to handle it
        #if defined(DEBUG)

            static bool breakpointTriggered = false;
            if (!breakpointTriggered) {
                breakpointTriggered = true;
                #if defined(OS_WINDOWS)
                    __debugbreak();
                #else
                    raise(SIGTRAP);
                #endif
            }

            std::exit(signalNumber);

        #else

            if (signalNumber == 0)
                std::abort();
            else
                std::exit(signalNumber);

        #endif
    }

    void handleCrash(const std::string &msg) {
        // Call the crash callback
        crashCallback(msg);

        // Print the stacktrace to the console or log file
        printStackTrace();

        // Flush all streams
        fflush(stdout);
        fflush(stderr);
    }

    // Custom signal handler to print various information and a stacktrace when the application crashes
    static void signalHandler(int signalNumber, const std::string &signalName) {
        #if !defined (DEBUG)
            if (signalNumber == SIGINT) {
                FenestraApi::System::closeApplication();
                return;
            }
        #endif

        // Reset crash handlers, so we can't have a recursion if this code crashes
        resetCrashHandlers();

        // Actually handle the crash
        handleCrash(fmt::format("Received signal '{}' ({})", signalName, signalNumber));

        // Detect if the crash was due to an uncaught exception
        if (std::uncaught_exceptions() > 0) {
            log::fatal("Uncaught exception thrown!");
        }

        triggerSafeShutdown(signalNumber);
    }

    static void uncaughtExceptionHandler() {
        // Reset crash handlers, so we can't have a recursion if this code crashes
        resetCrashHandlers();

        // Print the current exception info
        try {
            std::rethrow_exception(std::current_exception());
        } catch (std::exception &ex) {
            std::string exceptionStr = fmt::format("{}()::what() -> {}", typeid(ex).name(), ex.what());

            handleCrash(exceptionStr);
            log::fatal("Program terminated with uncaught exception: {}", exceptionStr);
        }

        triggerSafeShutdown();
    }

    // Setup functions to handle signals, uncaught exception, or similar stuff that will crash the application
    void setupCrashHandlers() {
        stacktrace::initialize();

        // Register signal handlers
        {
            #define HANDLE_SIGNAL(name)              \
            std::signal(name, [](int signalNumber) { \
                signalHandler(signalNumber, #name);  \
            })

            HANDLE_SIGNAL(SIGSEGV);
            HANDLE_SIGNAL(SIGILL);
            HANDLE_SIGNAL(SIGABRT);
            HANDLE_SIGNAL(SIGFPE);
            HANDLE_SIGNAL(SIGINT);

            #if defined (SIGBUS)
                HANDLE_SIGNAL(SIGBUS);
            #endif

            #undef HANDLE_SIGNAL
        }

        // Configure the uncaught exception handler
        std::set_terminate(uncaughtExceptionHandler);

        // Change the crash callback when the application has finished startup
        EventApplicationStartupFinished::subscribe([]{
            crashCallback = [](auto&){};
        });
    }

    void resetCrashHandlers() {
        std::set_terminate(nullptr);

        for (auto signal : Signals)
            std::signal(signal, SIG_DFL);
    }
}
