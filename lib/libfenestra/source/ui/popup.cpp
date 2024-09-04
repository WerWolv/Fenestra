#include <fenestra/ui/popup.hpp>
#include <fenestra/helpers/auto_reset.hpp>

namespace fene::impl {


    [[nodiscard]] std::vector<std::unique_ptr<PopupBase>> &PopupBase::getOpenPopups() {
        static AutoReset<std::vector<std::unique_ptr<PopupBase>>> openPopups;

        return openPopups;
    }

    std::mutex& PopupBase::getMutex() {
        static std::mutex mutex;

        return mutex;
    }



}