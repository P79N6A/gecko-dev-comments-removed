

































  

#ifndef JNI_MD_H
#define JNI_MD_H

#include "jri_md.h"

#define JNICALL			JRI_CALLBACK

#ifdef XP_WIN
#define JNIEXPORT __declspec(dllexport)
#else
#define JNIEXPORT 
#endif

#endif 
