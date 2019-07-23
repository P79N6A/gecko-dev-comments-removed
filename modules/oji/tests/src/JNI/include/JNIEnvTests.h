



































#ifndef JNIEnvTests_h___
#define JNIEnvTests_h___

#ifdef XP_WIN
#include <windows.h>
#endif
#include "nsIServiceManager.h"
#include "nsIJVMManager.h"
#include "nsJVMManager.h"



#include "ojiapitests.h"

#define SecENV TRUE

static NS_DEFINE_CID(kJVMManagerCID,NS_JVMMANAGER_CID);
static NS_DEFINE_IID(kIJVMManagerIID, NS_IJVMMANAGER_IID);

extern nsresult GetJNI(JNIEnv **env);

typedef unsigned char byte;

#ifdef XP_UNIX
#define MAX_JLONG jlong_MAXINT  // from jri_md.h
#define MIN_JLONG jlong_MININT
#else
#define MAX_JLONG  9223372036854775807
#define MIN_JLONG -9223372036854775808
#endif

#define MAX_JINT  2147483647
#define MIN_JINT -2147483648
#define MAX_JDOUBLE  1.7976931348623157E308
#define MIN_JDOUBLE  4.9E-324
#define MAX_JBYTE  127
#define MIN_JBYTE -128
#define MAX_JFLOAT  3.4028235E38F
#define MIN_JFLOAT  1.4E-45F
#define MAX_JSHORT  32767
#define MIN_JSHORT -32768


#endif 
