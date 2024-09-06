#pragma once

#include <fenestra.hpp>

#include <algorithm>
#include <functional>
#include <list>
#include <mutex>
#include <map>
#include <string_view>

#include <fenestra/helpers/logger.hpp>

#include <wolv/types/type_name.hpp>

#define EVENT_DEF_IMPL(event_name, event_name_string, should_log, ...)                                                                                                      \
    struct event_name final : public fene::impl::Event<__VA_ARGS__> {                                                                                                       \
        constexpr static auto Id = [] { return fene::impl::EventId(event_name_string); }();                                                                                 \
        constexpr static auto ShouldLog = (should_log);                                                                                                                     \
                                                                                                                                                                            \
        using Event::Event;                                                                                                                                                 \
                                                                                                                                                                            \
        static fene::EventManager::EventList::iterator subscribe(auto &&function) { return fene::EventManager::subscribe<event_name>(std::move(function)); }                \
        static void subscribe(void *token, auto &&function) { fene::EventManager::subscribe<event_name>(token, std::move(function)); }                                      \
        static void unsubscribe(const fene::EventManager::EventList::iterator &token) noexcept { fene::EventManager::unsubscribe(token); }                                  \
        static void unsubscribe(void *token) noexcept { fene::EventManager::unsubscribe<event_name>(token); }                                                               \
        static void post(auto &&...args) { fene::EventManager::post<event_name>(std::forward<decltype(args)>(args)...); }                                                   \
    }

#define EVENT_DEF(event_name, ...)          EVENT_DEF_IMPL(event_name, #event_name, true, __VA_ARGS__)
#define EVENT_DEF_NO_LOG(event_name, ...)   EVENT_DEF_IMPL(event_name, #event_name, false, __VA_ARGS__)


namespace fene {

    namespace impl {

        class EventId {
        public:
            explicit constexpr EventId(const char *eventName) {
                m_hash = 0x811C'9DC5;
                for (const char c : std::string_view(eventName)) {
                    m_hash = (m_hash >> 5) | (m_hash << 27);
                    m_hash ^= c;
                }
            }

            constexpr bool operator==(const EventId &other) const {
                return m_hash == other.m_hash;
            }

            constexpr auto operator<=>(const EventId &other) const {
                return m_hash <=> other.m_hash;
            }

        private:
            u32 m_hash;
        };

        struct EventBase {
            EventBase() noexcept = default;
            virtual ~EventBase() = default;
        };

        template<typename... Params>
        struct Event : EventBase {
            constexpr static bool HasParameters = sizeof...(Params) > 0;
            using Callback = std::function<void(Params...)>;
            using CallbackNoArgs = std::function<void()>;

            explicit Event(Callback func) noexcept : m_func(std::move(func)) { }
            explicit Event(CallbackNoArgs func) noexcept requires HasParameters : m_func([func = std::move(func)](auto &&...){ func(); }) { }

            template<typename E>
            void call(Params... params) const {
                try {
                    m_func(params...);
                } catch (const std::exception &e) {
                    log::error("An exception occurred while handling event {}: {}", wolv::type::getTypeName<E>(), e.what());
                    throw;
                }
            }

        private:
            Callback m_func;
        };

        template<typename T>
        concept EventType = std::derived_from<T, EventBase>;

    }


    /**
     * @brief The EventManager allows subscribing to and posting events to different parts of the program.
     * To create a new event, use the EVENT_DEF macro. This will create a new event type with the given name and parameters
     */
    class EventManager {
    public:
        using EventList = std::multimap<impl::EventId, std::unique_ptr<impl::EventBase>>;

        /**
         * @brief Subscribes to an event
         * @tparam E Event
         * @param function Function to call when the event is posted
         * @return Token to unsubscribe from the event
         */
        template<impl::EventType E>
        static EventList::iterator subscribe(typename E::Callback function) {
            std::scoped_lock lock(getEventMutex());

            auto &events = getEvents();
            return events.insert({ E::Id, std::make_unique<E>(function) });
        }

        /**
         * @brief Subscribes to an event
         * @tparam E Event
         * @param function Function to call when the event is posted
         * @return Token to unsubscribe from the event
         */
        template<impl::EventType E>
        static EventList::iterator subscribe(typename E::CallbackNoArgs function) requires E::HasParameters {
            std::scoped_lock lock(getEventMutex());

            auto &events = getEvents();
            return events.insert({ E::Id, std::make_unique<E>(function) });
        }

        /**
         * @brief Subscribes to an event
         * @tparam E Event
         * @param token Unique token to register the event to. Later required to unsubscribe again
         * @param function Function to call when the event is posted
         */
        template<impl::EventType E>
        static void subscribe(void *token, typename E::Callback function) {
            std::scoped_lock lock(getEventMutex());
            getTokenStore().insert({ token, subscribe<E>(function) });
        }

        /**
         * @brief Unsubscribes from an event
         * @param token Token returned by subscribe
         */
        static void unsubscribe(const EventList::iterator &token) noexcept {
            std::scoped_lock lock(getEventMutex());

            getEvents().erase(token);
        }

        /**
         * @brief Unsubscribes from an event
         * @tparam E Event
         * @param token Token passed to subscribe
         */
        template<impl::EventType E>
        static void unsubscribe(void *token) noexcept {
            std::scoped_lock lock(getEventMutex());

            auto &tokenStore = getTokenStore();
            auto iter = std::find_if(tokenStore.begin(), tokenStore.end(), [&](auto &item) {
                return item.first == token && item.second->first == E::Id;
            });

            if (iter != tokenStore.end()) {
                getEvents().erase(iter->second);
                tokenStore.erase(iter);
            }

        }

        /**
         * @brief Posts an event to all subscribers of it
         * @tparam E Event
         * @param args Arguments to pass to the event
         */
        template<impl::EventType E>
        static void post(auto && ...args) {
            std::scoped_lock lock(getEventMutex());

            auto [begin, end] = getEvents().equal_range(E::Id);
            for (auto it = begin; it != end; ++it) {
                const auto &[id, event] = *it;
                (*static_cast<E *const>(event.get())).template call<E>(std::forward<decltype(args)>(args)...);
            }

            #if defined (DEBUG)
                if constexpr (E::ShouldLog)
                    log::debug("Event posted: '{}({})'", wolv::type::getTypeName<E>(), log::impl::DebugFormattable(args)...);
#endif
        }

        /**
         * @brief Unsubscribe all subscribers from all events
         */
        static void clear() noexcept {
            std::scoped_lock lock(getEventMutex());

            getEvents().clear();
            getTokenStore().clear();
        }

    private:
        static std::multimap<void *, EventList::iterator>& getTokenStore();
        static EventList& getEvents();
        static std::recursive_mutex& getEventMutex();
    };

}