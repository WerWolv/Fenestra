#pragma once

#include <fenestra.hpp>
#include <fenestra/api/localization_manager.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <SDL3/SDL_keycode.h>

struct ImGuiWindow;

namespace fene {

    class View;

    enum class Keys {
        Space          = SDLK_SPACE,
        Apostrophe     = SDLK_APOSTROPHE,
        Comma          = SDLK_COMMA,
        Minus          = SDLK_MINUS,
        Period         = SDLK_PERIOD,
        Slash          = SDLK_SLASH,
        Num0           = SDLK_0,
        Num1           = SDLK_1,
        Num2           = SDLK_2,
        Num3           = SDLK_3,
        Num4           = SDLK_4,
        Num5           = SDLK_5,
        Num6           = SDLK_6,
        Num7           = SDLK_7,
        Num8           = SDLK_8,
        Num9           = SDLK_9,
        Semicolon      = SDLK_SEMICOLON,
        Equals         = SDLK_EQUALS,
        A              = SDLK_A,
        B              = SDLK_B,
        C              = SDLK_C,
        D              = SDLK_D,
        E              = SDLK_E,
        F              = SDLK_F,
        G              = SDLK_G,
        H              = SDLK_H,
        I              = SDLK_I,
        J              = SDLK_J,
        K              = SDLK_K,
        L              = SDLK_L,
        M              = SDLK_M,
        N              = SDLK_N,
        O              = SDLK_O,
        P              = SDLK_P,
        Q              = SDLK_Q,
        R              = SDLK_R,
        S              = SDLK_S,
        T              = SDLK_T,
        U              = SDLK_U,
        V              = SDLK_V,
        W              = SDLK_W,
        X              = SDLK_X,
        Y              = SDLK_Y,
        Z              = SDLK_Z,
        LeftBracket    = SDLK_LEFTBRACKET,
        Backslash      = SDLK_BACKSLASH,
        RightBracket   = SDLK_RIGHTBRACKET,
        GraveAccent    = SDLK_GRAVE,
        Escape         = SDLK_ESCAPE,
        Enter          = SDLK_RETURN,
        Tab            = SDLK_TAB,
        Backspace      = SDLK_BACKSPACE,
        Insert         = SDLK_INSERT,
        Delete         = SDLK_DELETE,
        Right          = SDLK_RIGHT,
        Left           = SDLK_LEFT,
        Down           = SDLK_DOWN,
        Up             = SDLK_UP,
        PageUp         = SDLK_PAGEUP,
        PageDown       = SDLK_PAGEDOWN,
        Home           = SDLK_HOME,
        End            = SDLK_END,
        CapsLock       = SDLK_CAPSLOCK,
        ScrollLock     = SDLK_SCROLLLOCK,
        NumLock        = SDLK_NUMLOCKCLEAR,
        PrintScreen    = SDLK_PRINTSCREEN,
        Pause          = SDLK_PAUSE,
        F1             = SDLK_F1,
        F2             = SDLK_F2,
        F3             = SDLK_F3,
        F4             = SDLK_F4,
        F5             = SDLK_F5,
        F6             = SDLK_F6,
        F7             = SDLK_F7,
        F8             = SDLK_F8,
        F9             = SDLK_F9,
        F10            = SDLK_F10,
        F11            = SDLK_F11,
        F12            = SDLK_F12,
        F13            = SDLK_F13,
        F14            = SDLK_F14,
        F15            = SDLK_F15,
        F16            = SDLK_F16,
        F17            = SDLK_F17,
        F18            = SDLK_F18,
        F19            = SDLK_F19,
        F20            = SDLK_F20,
        F21            = SDLK_F21,
        F22            = SDLK_F22,
        F23            = SDLK_F23,
        F24            = SDLK_F24,
        KeyPad0        = SDLK_KP_0,
        KeyPad1        = SDLK_KP_1,
        KeyPad2        = SDLK_KP_2,
        KeyPad3        = SDLK_KP_3,
        KeyPad4        = SDLK_KP_4,
        KeyPad5        = SDLK_KP_5,
        KeyPad6        = SDLK_KP_6,
        KeyPad7        = SDLK_KP_7,
        KeyPad8        = SDLK_KP_8,
        KeyPad9        = SDLK_KP_9,
        KeyPadDecimal  = SDLK_KP_DECIMAL,
        KeyPadDivide   = SDLK_KP_DIVIDE,
        KeyPadMultiply = SDLK_KP_MULTIPLY,
        KeyPadSubtract = SDLK_KP_MINUS,
        KeyPadAdd      = SDLK_KP_PLUS,
        KeyPadEnter    = SDLK_KP_ENTER,
        KeyPadEqual    = SDLK_KP_EQUALS,
        Menu           = SDLK_MENU,
    };


    class Key {
    public:
        constexpr Key() = default;
        constexpr Key(Keys key) : m_key(static_cast<u32>(key)) { }

        std::strong_ordering operator<=>(const Key &) const = default;

        [[nodiscard]] constexpr u32 getKeyCode() const { return m_key; }
    private:
        u32 m_key = 0;
    };


    constexpr static auto CTRL              = Key(static_cast<Keys>(0x0100'0000));
    constexpr static auto ALT               = Key(static_cast<Keys>(0x0200'0000));
    constexpr static auto SHIFT             = Key(static_cast<Keys>(0x0400'0000));
    constexpr static auto SUPER             = Key(static_cast<Keys>(0x0800'0000));
    constexpr static auto CurrentView       = Key(static_cast<Keys>(0x1000'0000));
    constexpr static auto AllowWhileTyping  = Key(static_cast<Keys>(0x2000'0000));

    #if defined (OS_MACOS)
        constexpr static auto CTRLCMD = SUPER;
    #else
        constexpr static auto CTRLCMD = CTRL;
    #endif

    class Shortcut {
    public:
        Shortcut() = default;
        Shortcut(Keys key) : m_keys({ key }) { }
        explicit Shortcut(std::set<Key> keys) : m_keys(std::move(keys)) { }
        Shortcut(const Shortcut &other) = default;
        Shortcut(Shortcut &&) noexcept = default;

        Shortcut& operator=(const Shortcut &other) = default;

        Shortcut& operator=(Shortcut &&) noexcept = default;

        constexpr static inline auto None = Keys(0);

        Shortcut operator+(const Key &other) const {
            Shortcut result = *this;
            result.m_keys.insert(other);

            return result;
        }

        Shortcut &operator+=(const Key &other) {
            m_keys.insert(other);

            return *this;
        }

        bool operator<(const Shortcut &other) const {
            return m_keys < other.m_keys;
        }

        bool operator==(const Shortcut &other) const {
            auto thisKeys = m_keys;
            auto otherKeys = other.m_keys;

            thisKeys.erase(CurrentView);
            thisKeys.erase(AllowWhileTyping);
            otherKeys.erase(CurrentView);
            otherKeys.erase(AllowWhileTyping);

            return thisKeys == otherKeys;
        }

        bool isLocal() const {
            return m_keys.contains(CurrentView);
        }

        std::string toString() const {
            std::string result;

            #if defined(OS_MACOS)
                constexpr static auto CTRL_NAME    = "CTRL";
                constexpr static auto ALT_NAME     = "OPT";
                constexpr static auto SHIFT_NAME   = "SHIFT";
                constexpr static auto SUPER_NAME   = "CMD";
            #else
                constexpr static auto CTRL_NAME    = "CTRL";
                constexpr static auto ALT_NAME     = "ALT";
                constexpr static auto SHIFT_NAME   = "SHIFT";
                constexpr static auto SUPER_NAME   = "SUPER";
            #endif

            constexpr static auto Concatination = " + ";

            auto keys = m_keys;
            if (keys.erase(CTRL) > 0) {
                result += CTRL_NAME;
                result += Concatination;
            }
            if (keys.erase(ALT) > 0) {
                result += ALT_NAME;
                result += Concatination;
            }
            if (keys.erase(SHIFT) > 0) {
                result += SHIFT_NAME;
                result += Concatination;
            }
            if (keys.erase(SUPER) > 0) {
                result += SUPER_NAME;
                result += Concatination;
            }
            keys.erase(CurrentView);

            for (const auto &key : keys) {
                switch (Keys(key.getKeyCode())) {
                    case Keys::Space:           result += "SPACE";          break;
                    case Keys::Apostrophe:      result += "'";              break;
                    case Keys::Comma:           result += ",";              break;
                    case Keys::Minus:           result += "-";              break;
                    case Keys::Period:          result += ".";              break;
                    case Keys::Slash:           result += "/";              break;
                    case Keys::Num0:            result += "0";              break;
                    case Keys::Num1:            result += "1";              break;
                    case Keys::Num2:            result += "2";              break;
                    case Keys::Num3:            result += "3";              break;
                    case Keys::Num4:            result += "4";              break;
                    case Keys::Num5:            result += "5";              break;
                    case Keys::Num6:            result += "6";              break;
                    case Keys::Num7:            result += "7";              break;
                    case Keys::Num8:            result += "8";              break;
                    case Keys::Num9:            result += "9";              break;
                    case Keys::Semicolon:       result += ";";              break;
                    case Keys::Equals:          result += "=";              break;
                    case Keys::A:               result += "A";              break;
                    case Keys::B:               result += "B";              break;
                    case Keys::C:               result += "C";              break;
                    case Keys::D:               result += "D";              break;
                    case Keys::E:               result += "E";              break;
                    case Keys::F:               result += "F";              break;
                    case Keys::G:               result += "G";              break;
                    case Keys::H:               result += "H";              break;
                    case Keys::I:               result += "I";              break;
                    case Keys::J:               result += "J";              break;
                    case Keys::K:               result += "K";              break;
                    case Keys::L:               result += "L";              break;
                    case Keys::M:               result += "M";              break;
                    case Keys::N:               result += "N";              break;
                    case Keys::O:               result += "O";              break;
                    case Keys::P:               result += "P";              break;
                    case Keys::Q:               result += "Q";              break;
                    case Keys::R:               result += "R";              break;
                    case Keys::S:               result += "S";              break;
                    case Keys::T:               result += "T";              break;
                    case Keys::U:               result += "U";              break;
                    case Keys::V:               result += "V";              break;
                    case Keys::W:               result += "W";              break;
                    case Keys::X:               result += "X";              break;
                    case Keys::Y:               result += "Y";              break;
                    case Keys::Z:               result += "Z";              break;
                    case Keys::LeftBracket:     result += "[";              break;
                    case Keys::Backslash:       result += "\\";             break;
                    case Keys::RightBracket:    result += "]";              break;
                    case Keys::GraveAccent:     result += "`";              break;
                    case Keys::Escape:          result += "ESC";            break;
                    case Keys::Enter:           result += "ENTER";          break;
                    case Keys::Tab:             result += "TAB";            break;
                    case Keys::Backspace:       result += "BACKSPACE";      break;
                    case Keys::Insert:          result += "INSERT";         break;
                    case Keys::Delete:          result += "DELETE";         break;
                    case Keys::Right:           result += "RIGHT";          break;
                    case Keys::Left:            result += "LEFT";           break;
                    case Keys::Down:            result += "DOWN";           break;
                    case Keys::Up:              result += "UP";             break;
                    case Keys::PageUp:          result += "PAGEUP";         break;
                    case Keys::PageDown:        result += "PAGEDOWN";       break;
                    case Keys::Home:            result += "HOME";           break;
                    case Keys::End:             result += "END";            break;
                    case Keys::CapsLock:        result += "CAPSLOCK";       break;
                    case Keys::ScrollLock:      result += "SCROLLLOCK";     break;
                    case Keys::NumLock:         result += "NUMLOCK";        break;
                    case Keys::PrintScreen:     result += "PRINTSCREEN";    break;
                    case Keys::Pause:           result += "PAUSE";          break;
                    case Keys::F1:              result += "F1";             break;
                    case Keys::F2:              result += "F2";             break;
                    case Keys::F3:              result += "F3";             break;
                    case Keys::F4:              result += "F4";             break;
                    case Keys::F5:              result += "F5";             break;
                    case Keys::F6:              result += "F6";             break;
                    case Keys::F7:              result += "F7";             break;
                    case Keys::F8:              result += "F8";             break;
                    case Keys::F9:              result += "F9";             break;
                    case Keys::F10:             result += "F10";            break;
                    case Keys::F11:             result += "F11";            break;
                    case Keys::F12:             result += "F12";            break;
                    case Keys::F13:             result += "F13";            break;
                    case Keys::F14:             result += "F14";            break;
                    case Keys::F15:             result += "F15";            break;
                    case Keys::F16:             result += "F16";            break;
                    case Keys::F17:             result += "F17";            break;
                    case Keys::F18:             result += "F18";            break;
                    case Keys::F19:             result += "F19";            break;
                    case Keys::F20:             result += "F20";            break;
                    case Keys::F21:             result += "F21";            break;
                    case Keys::F22:             result += "F22";            break;
                    case Keys::F23:             result += "F23";            break;
                    case Keys::F24:             result += "F24";            break;
                    case Keys::KeyPad0:         result += "KP0";            break;
                    case Keys::KeyPad1:         result += "KP1";            break;
                    case Keys::KeyPad2:         result += "KP2";            break;
                    case Keys::KeyPad3:         result += "KP3";            break;
                    case Keys::KeyPad4:         result += "KP4";            break;
                    case Keys::KeyPad5:         result += "KP5";            break;
                    case Keys::KeyPad6:         result += "KP6";            break;
                    case Keys::KeyPad7:         result += "KP7";            break;
                    case Keys::KeyPad8:         result += "KP8";            break;
                    case Keys::KeyPad9:         result += "KP9";            break;
                    case Keys::KeyPadDecimal:   result += "KPDECIMAL";      break;
                    case Keys::KeyPadDivide:    result += "KPDIVIDE";       break;
                    case Keys::KeyPadMultiply:  result += "KPMULTIPLY";     break;
                    case Keys::KeyPadSubtract:  result += "KPSUBTRACT";     break;
                    case Keys::KeyPadAdd:       result += "KPADD";          break;
                    case Keys::KeyPadEnter:     result += "KPENTER";        break;
                    case Keys::KeyPadEqual:     result += "KPEQUAL";        break;
                    case Keys::Menu:            result += "MENU";           break;
                    default:
                        continue;
                }

                result += " + ";
            }

            if (result.ends_with(" + "))
                result = result.substr(0, result.size() - 3);

            return result;
        }

        const std::set<Key>& getKeys() const { return m_keys; }

    private:
        friend Shortcut operator+(const Key &lhs, const Key &rhs);

        std::set<Key> m_keys;
    };

    inline Shortcut operator+(const Key &lhs, const Key &rhs) {
        Shortcut result;
        result.m_keys = { lhs, rhs };

        return result;
    }

    /**
     * @brief The ShortcutManager handles global and view-specific shortcuts.
     * New shortcuts can be constructed using the + operator on Key objects. For example: CTRL + ALT + Keys::A
     */
    class ShortcutManager {
    public:
        using Callback = std::function<void()>;
        struct ShortcutEntry {
            Shortcut shortcut;
            UnlocalizedString unlocalizedName;
            Callback callback;
        };

        /**
         * @brief Add a global shortcut. Global shortcuts can be triggered regardless of what view is currently focused
         * @param shortcut The shortcut to add.
         * @param unlocalizedName The unlocalized name of the shortcut
         * @param callback The callback to call when the shortcut is triggered.
         */
        static void addGlobalShortcut(const Shortcut &shortcut, const UnlocalizedString &unlocalizedName, const Callback &callback);

        /**
         * @brief Add a view-specific shortcut. View-specific shortcuts can only be triggered when the specified view is focused.
         * @param view The view to add the shortcut to.
         * @param shortcut The shortcut to add.
         * @param unlocalizedName The unlocalized name of the shortcut
         * @param callback The callback to call when the shortcut is triggered.
         */
        static void addShortcut(View *view, const Shortcut &shortcut, const UnlocalizedString &unlocalizedName, const Callback &callback);


        /**
         * @brief Process a key event. This should be called from the main loop.
         * @param currentView Current view to process
         * @param ctrl Whether the CTRL key is pressed
         * @param alt Whether the ALT key is pressed
         * @param shift Whether the SHIFT key is pressed
         * @param super Whether the SUPER key is pressed
         * @param focused Whether the current view is focused
         * @param keyCode The key code of the key that was pressed
         */
        static void process(const View *currentView, bool ctrl, bool alt, bool shift, bool super, bool focused, u32 keyCode);

        /**
         * @brief Process a key event. This should be called from the main loop.
         * @param ctrl Whether the CTRL key is pressed
         * @param alt Whether the ALT key is pressed
         * @param shift Whether the SHIFT key is pressed
         * @param super Whether the SUPER key is pressed
         * @param keyCode The key code of the key that was pressed
         */
        static void processGlobals(bool ctrl, bool alt, bool shift, bool super, u32 keyCode);

        /**
         * @brief Clear all shortcuts
         */
        static void clearShortcuts();

        static void resumeShortcuts();
        static void pauseShortcuts();

        [[nodiscard]] static std::optional<Shortcut> getPreviousShortcut();

        [[nodiscard]] static std::vector<ShortcutEntry> getGlobalShortcuts();
        [[nodiscard]] static std::vector<ShortcutEntry> getViewShortcuts(const View *view);

        [[nodiscard]] static bool updateShortcut(const Shortcut &oldShortcut, const Shortcut &newShortcut, View *view = nullptr);
    };

}