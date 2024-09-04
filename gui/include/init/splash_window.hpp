#pragma once

#include <fenestra.hpp>
#include <functional>
#include <future>
#include <list>
#include <ranges>
#include <string>

#include <mutex>

#include <imgui.h>
#include <fenestra/ui/imgui_extensions.h>

#include <SDL3/SDL.h>

namespace fene::init {

    using TaskFunction = std::function<bool()>;

    struct Task {
        std::string name;
        std::function<bool()> callback;
        bool async;
    };

    enum FrameResult{ Success, Failure, Running };

    struct Highlight {
        ImVec2 start;
        size_t count;
        ImColor color;
    };

    class WindowSplash {
    public:
        WindowSplash();
        ~WindowSplash();

        bool loop();

        FrameResult fullFrame();
        void startStartupTasks();

        void createTask(const Task &task);

        void addStartupTask(const std::string &taskName, const TaskFunction &function, bool async) {
            std::scoped_lock lock(m_tasksMutex);

            m_tasks.emplace_back(taskName, function, async);
        }

    private:
        SDL_Window *m_window;
        SDL_GLContext m_glContext;
        std::mutex m_progressMutex;
        std::atomic<float> m_progress = 0;
        std::list<std::string> m_currTaskNames;

        void initBackend();
        void initImGui();

        void exitBackend() const;
        void exitImGui() const;

        std::future<bool> processTasksAsync();

        std::atomic<u32> m_totalTaskCount, m_completedTaskCount;
        std::atomic<bool> m_taskStatus = true;
        std::list<Task> m_tasks;
        std::mutex m_tasksMutex;

        std::string m_gpuVendor;

        std::future<bool> m_tasksSucceeded;
        float m_progressLerp = 0.0F;
    };

}
