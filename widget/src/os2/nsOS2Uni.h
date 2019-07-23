



































#ifndef _nsos2uni_h
#define _nsos2uni_h

#include <os2.h>
#include <uconv.h>
#include "nsHashTable.h"


class OS2Uni {
public:
  static UconvObject GetUconvObject(int CodePage);
  static void FreeUconvObjects();
private:
  static nsHashtable gUconvObjects;
};

int WideCharToMultiByte( int CodePage, const PRUnichar *pText, ULONG ulLength, char* szBuffer, ULONG ulSize );
int MultiByteToWideChar( int CodePage, const char*pText, ULONG ulLength, PRUnichar *szBuffer, ULONG ulSize );

#endif
