























#ifndef SK_USE_CORETEXT
    #if TARGET_RT_64_BIT || defined(SK_USE_MAC_CORE_TEXT)
        #define SK_USE_CORETEXT                                     1
    #else
        #define SK_USE_CORETEXT                                     0
    #endif
#endif

#if SK_USE_CORETEXT
    #include "SkFontHost_mac_coretext.cpp"
#else
    #include "SkFontHost_mac_atsui.cpp"
#endif


