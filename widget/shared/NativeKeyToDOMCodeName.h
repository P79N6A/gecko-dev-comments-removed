

















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



#undef CODE_MAP_WIN
#undef CODE_MAP_MAC
#undef CODE_MAP_X11
#undef CODE_MAP_ANDROID
