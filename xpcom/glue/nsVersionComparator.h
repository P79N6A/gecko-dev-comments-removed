




































#ifndef nsVersionComparator_h__
#define nsVersionComparator_h__

#include "nscore.h"

#ifdef XP_WIN





PRInt32 NS_COM_GLUE
NS_CompareVersions(const PRUnichar *A, const PRUnichar *B);
#endif






PRInt32 NS_COM_GLUE
NS_CompareVersions(const char *A, const char *B);

#endif 
