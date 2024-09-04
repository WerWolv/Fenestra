#include <optional>

#include <fenestra/api/fenestra_manager.hpp>
#include <fenestra/api/event_manager.hpp>
#include <fenestra/helpers/logger.hpp>

#include <fenestra/events/system_events.hpp>

#include "messaging.hpp"

namespace fene::messaging {

    void messageReceived(const std::string &eventName, const std::vector<u8> &args) {
        log::debug("Received event '{}' with size {}", eventName, args.size());
        FenestraApi::Messaging::impl::runHandler(eventName, args);
    }

    void setupEvents() {
        RequestSendMessageToMainInstance::subscribe([](const std::string &eventName, const std::vector<u8> &eventData) {
            if (FenestraApi::System::isMainInstance()) {
                log::debug("Executing message '{}' in current instance", eventName);
                EventApplicationStartupFinished::subscribe([eventName, eventData]{
                    FenestraApi::Messaging::impl::runHandler(eventName, eventData);
                });
            } else {
                log::debug("Forwarding message '{}' to existing instance", eventName);
                sendToOtherInstance(eventName, eventData);
            }
        });
    }

    void setupMessaging() {
        FenestraApi::System::impl::setMainInstanceStatus(setupNative());
        setupEvents();
    }
}
