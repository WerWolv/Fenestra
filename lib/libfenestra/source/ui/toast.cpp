#include <fenestra/ui/toast.hpp>
#include <fenestra/helpers/auto_reset.hpp>

namespace fene::impl {

    [[nodiscard]] std::list<std::unique_ptr<ToastBase>> &ToastBase::getQueuedToasts() {
        static AutoReset<std::list<std::unique_ptr<ToastBase>>> queuedToasts;

        return queuedToasts;
    }

    std::mutex& ToastBase::getMutex() {
        static std::mutex mutex;

        return mutex;
    }

}