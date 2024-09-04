#if defined(OS_WEB)

#include <stdexcept>

#include <fenestra/helpers/intrinsics.hpp>
#include <fenestra/helpers/logger.hpp>

#include "messaging.hpp"

namespace fene::messaging {

    void sendToOtherInstance(const std::string &eventName, const std::vector<u8> &args) {
        hex::unused(eventName);
        hex::unused(args);
        log::error("Unimplemented function 'sendToOtherInstance()' called");
    }
    
    // Not implemented, so lets say we are the main instance every time so events are forwarded to ourselves
    bool setupNative() {
        return true;
    }
}

#endif
