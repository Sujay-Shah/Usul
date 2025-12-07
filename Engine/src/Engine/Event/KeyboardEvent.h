#ifndef __KEYBOARDEVENT_H__
#define __KEYBOARDEVENT_H__

#include "Event.h"

namespace Engine
{
    class KeyboardEvent : public Event
    {
        protected:
            KeyboardEvent(int keyCode)
            :
            m_keyCode(keyCode)
            {}

        public:
            unsigned int GetKeyCode() const { return m_keyCode; }
            EVENT_CATEGORY(INPUT | KEYBOARD);

        private:
            unsigned int m_keyCode;
    };

    class KeyPressedEvent : public KeyboardEvent
    {
        public:
            KeyPressedEvent(int key)
            :
            KeyboardEvent(key)
            {}

            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyPressedEvent: " << GetKeyCode() << " (repeat = " << m_isRepeat << ")";
                return ss.str();
            }

            EVENT_TYPE(KeyPressed);

            inline bool IsRepeat() const { return m_isRepeat; }

        private:
            bool m_isRepeat;
    };

    class KeyReleasedEvent : public KeyboardEvent
    {
        public:
            KeyReleasedEvent(int key)
            :
            KeyboardEvent(key)
            {}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyReleasedEvent: " << GetKeyCode();
                return ss.str();
            }

            EVENT_TYPE(KeyReleased);
    }; 

    class KeyTypedEvent : public KeyboardEvent
    {
        public:
            KeyTypedEvent(int key)
            :
            KeyboardEvent(key)
            {}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyTypedEvent: " << GetKeyCode();
                return ss.str();
            }

            EVENT_TYPE(KeyTyped);
    }; 
}

#endif