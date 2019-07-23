




































#ifndef INSTALLCLEANUPDEFINES_H
#define INSTALLCLEANUPDEFINES_H


#ifndef NS_LITERAL_CSTRING
#define NS_LITERAL_CSTRING(x) (x)
#endif

#define CLEANUP_MESSAGE_FILENAME  NS_LITERAL_CSTRING("cmessage.txt")

#define CLEANUP_REGISTRY          NS_LITERAL_CSTRING("xpicleanup.dat")

#if defined (XP_WIN)
#define CLEANUP_UTIL              NS_LITERAL_CSTRING("xpicleanup.exe")

#elif defined (XP_OS2)
#define CLEANUP_UTIL              NS_LITERAL_CSTRING("xpicleanup.exe")

#else
#define CLEANUP_UTIL              NS_LITERAL_CSTRING("xpicleanup")

#endif


#endif
