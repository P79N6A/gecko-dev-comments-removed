






































#include "nsWindow.h"
#include "nsQtKeyUtils.h"

#include "nsGUIEvent.h"

struct nsKeyConverter
{
    int vkCode; 
    int keysym; 
};

static struct nsKeyConverter nsKeycodes[] =
{

    { NS_VK_BACK,          Qt::Key_Backspace },
    { NS_VK_TAB,           Qt::Key_Tab },
    { NS_VK_TAB,           Qt::Key_Backtab },

    { NS_VK_RETURN,        Qt::Key_Return },
    { NS_VK_RETURN,        Qt::Key_Enter },
    { NS_VK_SHIFT,         Qt::Key_Shift },
    { NS_VK_CONTROL,       Qt::Key_Control },
    { NS_VK_ALT,           Qt::Key_Alt },
    { NS_VK_PAUSE,         Qt::Key_Pause },
    { NS_VK_CAPS_LOCK,     Qt::Key_CapsLock },
    { NS_VK_ESCAPE,        Qt::Key_Escape },
    { NS_VK_SPACE,         Qt::Key_Space },
    { NS_VK_PAGE_UP,       Qt::Key_PageUp },
    { NS_VK_PAGE_DOWN,     Qt::Key_PageDown },
    { NS_VK_END,           Qt::Key_End },
    { NS_VK_HOME,          Qt::Key_Home },
    { NS_VK_LEFT,          Qt::Key_Left },
    { NS_VK_UP,            Qt::Key_Up },
    { NS_VK_RIGHT,         Qt::Key_Right },
    { NS_VK_DOWN,          Qt::Key_Down },
    { NS_VK_PRINTSCREEN,   Qt::Key_Print },
    { NS_VK_INSERT,        Qt::Key_Insert },
    { NS_VK_DELETE,        Qt::Key_Delete },

    { NS_VK_0,             Qt::Key_0 },
    { NS_VK_1,             Qt::Key_1 },
    { NS_VK_2,             Qt::Key_2 },
    { NS_VK_3,             Qt::Key_3 },
    { NS_VK_4,             Qt::Key_4 },
    { NS_VK_5,             Qt::Key_5 },
    { NS_VK_6,             Qt::Key_6 },
    { NS_VK_7,             Qt::Key_7 },
    { NS_VK_8,             Qt::Key_8 },
    { NS_VK_9,             Qt::Key_9 },

    { NS_VK_SEMICOLON,     Qt::Key_Semicolon },
    { NS_VK_EQUALS,        Qt::Key_Equal },

    { NS_VK_A,             Qt::Key_A },
    { NS_VK_B,             Qt::Key_B },
    { NS_VK_C,             Qt::Key_C },
    { NS_VK_D,             Qt::Key_D },
    { NS_VK_E,             Qt::Key_E },
    { NS_VK_F,             Qt::Key_F },
    { NS_VK_G,             Qt::Key_G },
    { NS_VK_H,             Qt::Key_H },
    { NS_VK_I,             Qt::Key_I },
    { NS_VK_J,             Qt::Key_J },
    { NS_VK_K,             Qt::Key_K },
    { NS_VK_L,             Qt::Key_L },
    { NS_VK_M,             Qt::Key_M },
    { NS_VK_N,             Qt::Key_N },
    { NS_VK_O,             Qt::Key_O },
    { NS_VK_P,             Qt::Key_P },
    { NS_VK_Q,             Qt::Key_Q },
    { NS_VK_R,             Qt::Key_R },
    { NS_VK_S,             Qt::Key_S },
    { NS_VK_T,             Qt::Key_T },
    { NS_VK_U,             Qt::Key_U },
    { NS_VK_V,             Qt::Key_V },
    { NS_VK_W,             Qt::Key_W },
    { NS_VK_X,             Qt::Key_X },
    { NS_VK_Y,             Qt::Key_Y },
    { NS_VK_Z,             Qt::Key_Z },

    { NS_VK_NUMPAD0,       Qt::Key_0 },
    { NS_VK_NUMPAD1,       Qt::Key_1 },
    { NS_VK_NUMPAD2,       Qt::Key_2 },
    { NS_VK_NUMPAD3,       Qt::Key_3 },
    { NS_VK_NUMPAD4,       Qt::Key_4 },
    { NS_VK_NUMPAD5,       Qt::Key_5 },
    { NS_VK_NUMPAD6,       Qt::Key_6 },
    { NS_VK_NUMPAD7,       Qt::Key_7 },
    { NS_VK_NUMPAD8,       Qt::Key_8 },
    { NS_VK_NUMPAD9,       Qt::Key_9 },
    { NS_VK_MULTIPLY,      Qt::Key_Asterisk },
    { NS_VK_ADD,           Qt::Key_Plus },

    { NS_VK_SUBTRACT,      Qt::Key_Minus },
    { NS_VK_DECIMAL,       Qt::Key_Period },
    { NS_VK_DIVIDE,        Qt::Key_Slash },
    { NS_VK_F1,            Qt::Key_F1 },
    { NS_VK_F2,            Qt::Key_F2 },
    { NS_VK_F3,            Qt::Key_F3 },
    { NS_VK_F4,            Qt::Key_F4 },
    { NS_VK_F5,            Qt::Key_F5 },
    { NS_VK_F6,            Qt::Key_F6 },
    { NS_VK_F7,            Qt::Key_F7 },
    { NS_VK_F8,            Qt::Key_F8 },
    { NS_VK_F9,            Qt::Key_F9 },
    { NS_VK_F10,           Qt::Key_F10 },
    { NS_VK_F11,           Qt::Key_F11 },
    { NS_VK_F12,           Qt::Key_F12 },
    { NS_VK_F13,           Qt::Key_F13 },
    { NS_VK_F14,           Qt::Key_F14 },
    { NS_VK_F15,           Qt::Key_F15 },
    { NS_VK_F16,           Qt::Key_F16 },
    { NS_VK_F17,           Qt::Key_F17 },
    { NS_VK_F18,           Qt::Key_F18 },
    { NS_VK_F19,           Qt::Key_F19 },
    { NS_VK_F20,           Qt::Key_F20 },
    { NS_VK_F21,           Qt::Key_F21 },
    { NS_VK_F22,           Qt::Key_F22 },
    { NS_VK_F23,           Qt::Key_F23 },
    { NS_VK_F24,           Qt::Key_F24 },

    { NS_VK_NUM_LOCK,      Qt::Key_NumLock },
    { NS_VK_SCROLL_LOCK,   Qt::Key_ScrollLock },

    { NS_VK_COMMA,         Qt::Key_Comma },
    { NS_VK_PERIOD,        Qt::Key_Period },
    { NS_VK_SLASH,         Qt::Key_Slash },
    { NS_VK_BACK_QUOTE,    Qt::Key_QuoteLeft },
    { NS_VK_OPEN_BRACKET,  Qt::Key_ParenLeft },
    { NS_VK_CLOSE_BRACKET, Qt::Key_ParenRight },
    { NS_VK_QUOTE,         Qt::Key_QuoteDbl },

    { NS_VK_META,          Qt::Key_Meta }
};


#define IS_XSUN_XSERVER(dpy) \
    (strstr(XServerVendor(dpy), "Sun Microsystems") != NULL)


struct nsKeyConverter nsSunKeycodes[] = {
    {NS_VK_ESCAPE, Qt::Key_F11 }, 
    {NS_VK_F1, Qt::Key_Help }, 
    {NS_VK_F11, 0x1005ff10 }, 
    {NS_VK_F12, 0x1005ff11 }, 
    {NS_VK_PAGE_UP,    Qt::Key_F29 }, 
    {NS_VK_PAGE_DOWN,  Qt::Key_F35 }, 
    {NS_VK_HOME,       Qt::Key_F27 }, 
    {NS_VK_END,        Qt::Key_F33 }, 
};

int
QtKeyCodeToDOMKeyCode(int aKeysym)
{
    int i, length = 0;

    
    
    

    
    
    if (aKeysym >= Qt::Key_A && aKeysym <= Qt::Key_Z)
        return aKeysym - Qt::Key_A + NS_VK_A;

    
    if (aKeysym >= Qt::Key_0 && aKeysym <= Qt::Key_9)
        return aKeysym - Qt::Key_0 + NS_VK_0;

    



    








    
    length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);
    for (i = 0; i < length; i++) {
        if (nsKeycodes[i].keysym == aKeysym)
            return(nsKeycodes[i].vkCode);
    }

    
    if (aKeysym >= Qt::Key_F1 && aKeysym <= Qt::Key_F24)
        return aKeysym - Qt::Key_F1 + NS_VK_F1;

    return((int)0);
}

int
DOMKeyCodeToQtKeyCode(int aKeysym)
{
    int i, length = 0;

    
    
    

    if (aKeysym >= NS_VK_A && aKeysym <= NS_VK_Z)
      
      return aKeysym;

    
    if (aKeysym >= NS_VK_0 && aKeysym <= NS_VK_9)
      
      return aKeysym - Qt::Key_0 + NS_VK_0;

    
    if (aKeysym >= NS_VK_NUMPAD0 && aKeysym <= NS_VK_NUMPAD9) {
      qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
      
      return 0;
    }

    
    length = NS_ARRAY_LENGTH(nsKeycodes);
    for (i = 0; i < length; ++i) {
      if (nsKeycodes[i].vkCode == aKeysym) {
        return nsKeycodes[i].keysym;
      }
    }

    
    if (aKeysym >= NS_VK_F1 && aKeysym <= NS_VK_F9)
      return aKeysym - NS_VK_F1 + Qt::Key_F1;

    return 0;
}
