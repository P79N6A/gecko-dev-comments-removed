

















#define CODE_MAP_WIN(aCPPCodeName, aNativeKey)

#define CODE_MAP_MAC(aCPPCodeName, aNativeKey)

#define CODE_MAP_X11(aCPPCodeName, aNativeKey)

#define CODE_MAP_ANDROID(aCPPCodeName, aNativeKey)

#if defined(XP_WIN)
#undef CODE_MAP_WIN

#define CODE_MAP_WIN(aCPPCodeName, aNativeKey) \
  NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, \
                                       CODE_NAME_INDEX_##aCPPCodeName)
#elif defined(XP_MACOSX)
#undef CODE_MAP_MAC
#define CODE_MAP_MAC(aCPPCodeName, aNativeKey) \
  NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, \
                                       CODE_NAME_INDEX_##aCPPCodeName)
#elif defined(MOZ_WIDGET_GTK) || defined(MOZ_WIDGET_QT)
#undef CODE_MAP_X11
#define CODE_MAP_X11(aCPPCodeName, aNativeKey) \
  NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, \
                                       CODE_NAME_INDEX_##aCPPCodeName)
#elif defined(ANDROID)
#undef CODE_MAP_ANDROID
#define CODE_MAP_ANDROID(aCPPCodeName, aNativeKey) \
  NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, \
                                       CODE_NAME_INDEX_##aCPPCodeName)
#endif


CODE_MAP_WIN(Backquote,                 0x0029)

CODE_MAP_WIN(Backslash,                 0x002B)

CODE_MAP_WIN(Backspace,                 0x000E)

CODE_MAP_WIN(BracketLeft,               0x001A)

CODE_MAP_WIN(BracketRight,              0x001B)

CODE_MAP_WIN(Comma,                     0x0033)

CODE_MAP_WIN(Digit0,                    0x0002)

CODE_MAP_WIN(Digit1,                    0x0003)

CODE_MAP_WIN(Digit2,                    0x0004)

CODE_MAP_WIN(Digit3,                    0x0005)

CODE_MAP_WIN(Digit4,                    0x0006)

CODE_MAP_WIN(Digit5,                    0x0007)

CODE_MAP_WIN(Digit6,                    0x0008)

CODE_MAP_WIN(Digit7,                    0x0009)

CODE_MAP_WIN(Digit8,                    0x000A)

CODE_MAP_WIN(Digit9,                    0x000B)

CODE_MAP_WIN(Equal,                     0x000D)

CODE_MAP_WIN(IntlBackslash,             0x0056)



CODE_MAP_WIN(IntlRo,                    0x0073)

CODE_MAP_WIN(IntlYen,                   0x007D)

CODE_MAP_WIN(KeyA,                      0x001E)

CODE_MAP_WIN(KeyB,                      0x0030)

CODE_MAP_WIN(KeyC,                      0x002E)

CODE_MAP_WIN(KeyD,                      0x0020)

CODE_MAP_WIN(KeyE,                      0x0012)

CODE_MAP_WIN(KeyF,                      0x0021)

CODE_MAP_WIN(KeyG,                      0x0022)

CODE_MAP_WIN(KeyH,                      0x0023)

CODE_MAP_WIN(KeyI,                      0x0017)

CODE_MAP_WIN(KeyJ,                      0x0024)

CODE_MAP_WIN(KeyK,                      0x0025)

CODE_MAP_WIN(KeyL,                      0x0026)

CODE_MAP_WIN(KeyM,                      0x0032)

CODE_MAP_WIN(KeyN,                      0x0031)

CODE_MAP_WIN(KeyO,                      0x0018)

CODE_MAP_WIN(KeyP,                      0x0019)

CODE_MAP_WIN(KeyQ,                      0x0010)

CODE_MAP_WIN(KeyR,                      0x0013)

CODE_MAP_WIN(KeyS,                      0x001F)

CODE_MAP_WIN(KeyT,                      0x0014)

CODE_MAP_WIN(KeyU,                      0x0016)

CODE_MAP_WIN(KeyV,                      0x002F)

CODE_MAP_WIN(KeyW,                      0x0011)

CODE_MAP_WIN(KeyX,                      0x002D)

CODE_MAP_WIN(KeyY,                      0x0015)

CODE_MAP_WIN(KeyZ,                      0x002C)

CODE_MAP_WIN(Minus,                     0x000C)

CODE_MAP_WIN(Period,                    0x0034)

CODE_MAP_WIN(Quote,                     0x0028)

CODE_MAP_WIN(Semicolon,                 0x0027)

CODE_MAP_WIN(Slash,                     0x0035)


CODE_MAP_WIN(AltLeft,                   0x0038)

CODE_MAP_WIN(AltRight,                  0xE038)

CODE_MAP_WIN(CapsLock,                  0x003A)

CODE_MAP_WIN(ContextMenu,               0xE05D)

CODE_MAP_WIN(ControlLeft,               0x001D)

CODE_MAP_WIN(ControlRight,              0xE01D)

CODE_MAP_WIN(Enter,                     0x001C)

CODE_MAP_WIN(OSLeft,                    0xE05B)

CODE_MAP_WIN(OSRight,                   0xE05C)

CODE_MAP_WIN(ShiftLeft,                 0x002A)

CODE_MAP_WIN(ShiftRight,                0x0036)

CODE_MAP_WIN(Space,                     0x0039)

CODE_MAP_WIN(Tab,                       0x000F)


CODE_MAP_WIN(Convert,                   0x0079)

CODE_MAP_WIN(Lang1,                     0x0072) 
CODE_MAP_WIN(Lang1,                     0xE0F2) 

CODE_MAP_WIN(Lang2,                     0x0071) 
CODE_MAP_WIN(Lang2,                     0xE0F1) 

CODE_MAP_WIN(KanaMode,                  0x0070)

CODE_MAP_WIN(NonConvert,                0x007B)


CODE_MAP_WIN(Delete,                    0xE053)

CODE_MAP_WIN(End,                       0xE04F)

CODE_MAP_WIN(Home,                      0xE047)

CODE_MAP_WIN(Insert,                    0xE052)

CODE_MAP_WIN(PageDown,                  0xE051)

CODE_MAP_WIN(PageUp,                    0xE049)


CODE_MAP_WIN(ArrowDown,                 0xE050)

CODE_MAP_WIN(ArrowLeft,                 0xE04B)

CODE_MAP_WIN(ArrowRight,                0xE04D)

CODE_MAP_WIN(ArrowUp,                   0xE048)


CODE_MAP_WIN(NumLock,                   0xE045) 

CODE_MAP_WIN(Numpad0,                   0x0052)

CODE_MAP_WIN(Numpad1,                   0x004F)

CODE_MAP_WIN(Numpad2,                   0x0050)

CODE_MAP_WIN(Numpad3,                   0x0051)

CODE_MAP_WIN(Numpad4,                   0x004B)

CODE_MAP_WIN(Numpad5,                   0x004C)

CODE_MAP_WIN(Numpad6,                   0x004D)

CODE_MAP_WIN(Numpad7,                   0x0047)

CODE_MAP_WIN(Numpad8,                   0x0048)

CODE_MAP_WIN(Numpad9,                   0x0049)

CODE_MAP_WIN(NumpadAdd,                 0x004E)

CODE_MAP_WIN(NumpadComma,               0x007E)

CODE_MAP_WIN(NumpadDecimal,             0x0053)

CODE_MAP_WIN(NumpadDivide,              0xE035)

CODE_MAP_WIN(NumpadEnter,               0xE01C)

CODE_MAP_WIN(NumpadEqual,               0x0059)

CODE_MAP_WIN(NumpadMultiply,            0x0037)

CODE_MAP_WIN(NumpadSubtract,            0x004A)


CODE_MAP_WIN(Escape,                    0x0001)

CODE_MAP_WIN(F1,                        0x003B)

CODE_MAP_WIN(F2,                        0x003C)

CODE_MAP_WIN(F3,                        0x003D)

CODE_MAP_WIN(F4,                        0x003E)

CODE_MAP_WIN(F5,                        0x003F)

CODE_MAP_WIN(F6,                        0x0040)

CODE_MAP_WIN(F7,                        0x0041)

CODE_MAP_WIN(F8,                        0x0042)

CODE_MAP_WIN(F9,                        0x0043)

CODE_MAP_WIN(F10,                       0x0044)

CODE_MAP_WIN(F11,                       0x0057)

CODE_MAP_WIN(F12,                       0x0058)

CODE_MAP_WIN(F13,                       0x0064)

CODE_MAP_WIN(F14,                       0x0065)

CODE_MAP_WIN(F15,                       0x0066)

CODE_MAP_WIN(F16,                       0x0067)

CODE_MAP_WIN(F17,                       0x0068)

CODE_MAP_WIN(F18,                       0x0069)

CODE_MAP_WIN(F19,                       0x006A)

CODE_MAP_WIN(F20,                       0x006B)

CODE_MAP_WIN(F21,                       0x006C)

CODE_MAP_WIN(F22,                       0x006D)

CODE_MAP_WIN(F23,                       0x006E)

CODE_MAP_WIN(F24,                       0x0076)

CODE_MAP_WIN(PrintScreen,               0xE037)
CODE_MAP_WIN(PrintScreen,               0x0054) 

CODE_MAP_WIN(ScrollLock,                0x0046)

CODE_MAP_WIN(Pause,                     0x0045)
CODE_MAP_WIN(Pause,                     0xE046) 







































CODE_MAP_WIN(Power,                     0xE05E)











#undef CODE_MAP_WIN
#undef CODE_MAP_MAC
#undef CODE_MAP_X11
#undef CODE_MAP_ANDROID
