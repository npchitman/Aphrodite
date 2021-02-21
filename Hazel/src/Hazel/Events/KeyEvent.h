//
// Created by Npchitman on 2021/1/18.
//

#ifndef HAZELENGINE_KEYEVENT_H
#define HAZELENGINE_KEYEVENT_H

#include "Event.h"

namespace Hazel {
    class HAZEL_API KeyEvent : public Event {
    public:
        inline int GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        explicit KeyEvent(int keycode) : m_KeyCode(keycode) {}

        int m_KeyCode;
    };

    class HAZEL_API KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(int keyCode, int repeatCount)
                : KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

        inline int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)

    private:
        int m_RepeatCount;
    };


    class HAZEL_API KeyReleaseEvent : public KeyEvent {
    public:
        explicit KeyReleaseEvent(int keycode) : KeyEvent(keycode) {}

        std::string ToString() const override {
            std::stringstream ss;
            ss << "KeyReleaseEvent: " << m_KeyCode;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    class HAZEL_API KeyTypedEvent : public KeyEvent {
    public:
        explicit KeyTypedEvent(int keyCode)
                : KeyEvent(keyCode) {}

        std::string ToString() const override {
            std::stringstream ss;
            ss << "KeyTypedEvent: " << m_KeyCode;
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)
    };
}


#endif //HAZELENGINE_KEYEVENT_H
