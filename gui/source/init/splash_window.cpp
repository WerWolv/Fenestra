#include "window.hpp"
#include "init/splash_window.hpp"

#include <fenestra/api/interface_registry.hpp>
#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/api/task_manager.hpp>
#include <fenestra/events/fenestra_events.hpp>

#include <fenestra/helpers/utils.hpp>
#include <fenestra/helpers/logger.hpp>
#include <fmt/chrono.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <opengl_support.h>

#include <wolv/utils/guards.hpp>

#include <future>


using namespace std::literals::chrono_literals;

namespace fene::init {

    WindowSplash::WindowSplash() : m_window(nullptr), m_glContext(nullptr) {
        this->initBackend();
        this->initImGui();

        {
            auto glVendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
            auto glRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
            auto glVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
            auto glShadingLanguageVersion = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

            log::debug("OpenGL Vendor: '{}'", glVendor);
            log::debug("OpenGL Renderer: '{}'", glRenderer);
            log::debug("OpenGL Version: '{}'", glVersion);
            log::debug("OpenGL Shading Language Version: '{}'", glShadingLanguageVersion);

            FenestraManager::System::impl::setGPUVendor(glVendor);
            FenestraManager::System::impl::setGLRenderer(glRenderer);
        }

        RequestAddInitTask::subscribe([this](const std::string& name, bool async, const TaskFunction &function){
            m_tasks.push_back(Task{ name, function, async });
        });
    }

    WindowSplash::~WindowSplash() {
        this->exitImGui();
        this->exitBackend();
    }

    void WindowSplash::createTask(const Task& task) {
        auto runTask = [&, task] {
            try {
                // Save an iterator to the current task name
                decltype(m_currTaskNames)::iterator taskNameIter;
                {
                    std::lock_guard guard(m_progressMutex);
                    m_currTaskNames.push_back(task.name + "...");
                    taskNameIter = std::prev(m_currTaskNames.end());
                }

                // When the task finished, increment the progress bar
                ON_SCOPE_EXIT {
                    m_completedTaskCount += 1;
                    m_progress = float(m_completedTaskCount) / float(m_totalTaskCount);
                };

                // Execute the actual task and track the amount of time it took to run
                auto startTime = std::chrono::high_resolution_clock::now();
                bool taskStatus = task.callback();
                auto endTime = std::chrono::high_resolution_clock::now();

                auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

                if (taskStatus)
                    log::info("Task '{}' finished successfully in {} ms", task.name, milliseconds);
                else
                    log::warn("Task '{}' finished unsuccessfully in {} ms", task.name, milliseconds);

                // Track the overall status of the tasks
                m_taskStatus = m_taskStatus && taskStatus;

                // Erase the task name from the list of running tasks
                {
                    std::lock_guard guard(m_progressMutex);
                    m_currTaskNames.erase(taskNameIter);
                }
            } catch (const std::exception &e) {
                log::error("Init task '{}' threw an exception: {}", task.name, e.what());
                m_taskStatus = false;
            } catch (...) {
                log::error("Init task '{}' threw an unidentifiable exception", task.name);
                m_taskStatus = false;
            }
        };

        m_totalTaskCount += 1;

        // If the task can be run asynchronously, run it in a separate thread
        // otherwise run it in this thread and wait for it to finish
        if (task.async) {
            std::thread([name = task.name, runTask = std::move(runTask)] {
                TaskManager::setCurrentThreadName(name);
                runTask();
            }).detach();
        } else {
            runTask();
        }
    }

    std::future<bool> WindowSplash::processTasksAsync() {
        return std::async(std::launch::async, [this] {
            TaskManager::setCurrentThreadName("Init Tasks");

            auto startTime = std::chrono::high_resolution_clock::now();

            // Loop over all registered init tasks
            for (auto &task : m_tasks) {
                // Construct a new task callback
                this->createTask(task);
            }

            // Check every 100ms if all tasks have run
            while (true) {
                {
                    std::scoped_lock lock(m_tasksMutex);
                    if (m_completedTaskCount >= m_totalTaskCount)
                        break;
                }

                std::this_thread::sleep_for(100ms);
            }

            auto endTime = std::chrono::high_resolution_clock::now();

            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            log::info("Application fully started in {}ms", milliseconds);

            // Small extra delay so the last progress step is visible
            std::this_thread::sleep_for(100ms);

            return m_taskStatus.load();
        });
    }


    FrameResult WindowSplash::fullFrame() {
        const auto &splashScreen = InterfaceRegistry::impl::getSplashScreen();
        if (splashScreen != nullptr) {
            {
                SDL_ShowWindow(m_window);

                auto size = splashScreen->getSize();
                SDL_SetWindowSize(m_window, size.x, size.y);
                SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            }

            SDL_Event event;
            SDL_PollEvent(&event);

            // Start new ImGui Frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            {
                std::lock_guard guard(m_progressMutex);

                // Draw the splash screen
                splashScreen->draw(m_completedTaskCount, m_tasks.size());
            }

            // Render the frame
            ImGui::Render();
            int displayWidth, displayHeight;
            SDL_GetWindowSizeInPixels(m_window, &displayWidth, &displayHeight);
            glViewport(0, 0, displayWidth, displayHeight);
            glClearColor(0.00F, 0.00F, 0.00F, 0.00F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(m_window);
        }

        // Check if all background tasks have finished so the splash screen can be closed
        if (this->m_tasksSucceeded.wait_for(0s) == std::future_status::ready) {
            if (this->m_tasksSucceeded.get()) {
                log::debug("All tasks finished successfully!");
                return FrameResult::Success;
            } else {
                log::warn("All tasks finished, but some failed");
                return FrameResult::Failure;
            }
        }

        return FrameResult::Running;
    }

    bool WindowSplash::loop() {
        // Splash window rendering loop
        while (true) {
            auto startTime = SDL_GetTicks();
            auto frameResult = this->fullFrame();
            auto endTime = SDL_GetTicks();

            if (frameResult == FrameResult::Success)
                return true;
            else if (frameResult == FrameResult::Failure)
                return false;
            else {
                if (endTime - startTime < 1000) {
                    SDL_Delay((1000.0F / 60.0F) - (endTime - startTime));
                }
            }
        }
    }

    void WindowSplash::initBackend() {
        if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
            log::fatal("Failed to initialize SDL3!");
            log::fatal("Error: {}", SDL_GetError());
            std::exit(EXIT_FAILURE);
        }

        // Configure used OpenGL version
    #if defined(OS_MACOS)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #endif

        SDL_SetHint(SDL_HINT_APP_ID, FENESTRA_APPLICATION_NAME_LOWER);

        // Create the splash screen window
        m_window = SDL_CreateWindow("Starting Application...", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP);
        if (m_window == nullptr) {
            fene::nativeErrorMessage(fmt::format(
                "Failed to create GLFW window: {}.\n"
                "You may not have a renderer available.\n"
                "The most common cause of this is using a virtual machine\n"
                "You may want to try a release artifact ending with 'NoGPU'",
                SDL_GetError()
            ));
            std::exit(EXIT_FAILURE);
        }

        m_glContext = SDL_GL_CreateContext(m_window);

        SDL_GL_MakeCurrent(m_window, m_glContext);

        // Force window to be fully opaque by default
        SDL_SetWindowOpacity(m_window, 1.0F);

        // Calculate native scale factor for hidpi displays
        {
            auto scale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(m_window));

            if (scale <= 0.0F)
                scale = 1.0F;

            #if defined(OS_WEB)
                scale = 1.0F;
            #endif

            FenestraManager::System::impl::setGlobalScale(scale);
            FenestraManager::System::impl::setNativeScale(scale);

            log::info("Native scaling set to: {:.1f}", scale);
        }

        SDL_GL_MakeCurrent(m_window, m_glContext);
        SDL_GL_SwapWindow(m_window);
    }

    void WindowSplash::initImGui() {
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        GImGui = ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);

        #if defined(OS_MACOS)
            ImGui_ImplOpenGL3_Init("#version 150");
        #elif defined(OS_WEB)
            ImGui_ImplOpenGL3_Init();
        #else
            ImGui_ImplOpenGL3_Init("#version 130");
        #endif

        auto &io = ImGui::GetIO();

        ImGui::GetStyle().ScaleAllSizes(FenestraManager::System::getGlobalScale());

        // Load fonts necessary for the splash screen
        {
            io.Fonts->Clear();

            ImFontConfig cfg;
            cfg.OversampleH = cfg.OversampleV = 1, cfg.PixelSnapH = true;
            cfg.SizePixels = FenestraManager::Fonts::DefaultFontSize;
            io.Fonts->AddFontDefault(&cfg);

            std::uint8_t *px;
            int w, h;
            io.Fonts->GetTexDataAsAlpha8(&px, &w, &h);

            // Create new font atlas
            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, px);
            io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(tex));
        }

        // Don't save window settings for the splash screen
        io.IniFilename = nullptr;
    }

    void WindowSplash::startStartupTasks() {
        // Launch init tasks in the background
        this->m_tasksSucceeded = processTasksAsync();
    }

    void WindowSplash::exitBackend() const {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void WindowSplash::exitImGui() const {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

}
