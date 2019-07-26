








#ifndef SkUserConfig_DEFINED
#define SkUserConfig_DEFINED

































































































































#define SK_ALLOW_OVER_32K_BITMAPS












#ifdef SK_DEBUG

#endif















#ifdef SK_SAMPLES_FOR_X
        #define SK_R32_SHIFT    16
        #define SK_G32_SHIFT    8
        #define SK_B32_SHIFT    0
        #define SK_A32_SHIFT    24
#endif









#ifdef USE_SKIA_GPU
    #define SK_SUPPORT_GPU 1
#else
    #define SK_SUPPORT_GPU 0
#endif



#define SK_DISABLE_DITHER_32BIT_GRADIENT



#ifdef SK_BUILD_FOR_WIN32 
    #define SK_IGNORE_STDINT_DOT_H 
#endif 

#endif
