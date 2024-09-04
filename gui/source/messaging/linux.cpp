#if defined(OS_LINUX)

#include <stdexcept>

#include <fenestra/helpers/logger.hpp>

#include "messaging.hpp"

namespace fene::messaging {

    void sendToOtherInstance(const std::string &eventName, const std::vector<u8> &args) {
        std::ignore = eventName;
        std::ignore = args;
        log::error("Unimplemented function 'sendToOtherInstance()' called");
    }
    
    // Not implemented, so lets say we are the main instance every time so events are forwarded to ourselves
    bool setupNative() {
        return true;
    }
}

#endif
