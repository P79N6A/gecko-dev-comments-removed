



































#ifndef _nsos2uni_h
#define _nsos2uni_h

#define INCL_WIN
#include <os2.h>
#include <uconv.h>
#include "nsTArray.h"
#include "nsICharsetConverterManager.h"
#include "gfxCore.h"

enum ConverterRequest {
  eConv_Encoder,
  eConv_Decoder
};

class OS2Uni {
public:
  static nsISupports* GetUconvObject(int CodePage, ConverterRequest aReq);
  static void FreeUconvObjects();
private:
  static nsICharsetConverterManager* gCharsetManager;
};

#define CHAR_BUFFER_SIZE 1024
typedef nsAutoTArray<char, CHAR_BUFFER_SIZE> nsAutoCharBuffer;
typedef nsAutoTArray<PRUnichar, CHAR_BUFFER_SIZE> nsAutoChar16Buffer;

nsresult WideCharToMultiByte(int aCodePage, const PRUnichar* aSrc,
                             PRInt32 aSrcLength, nsAutoCharBuffer& aResult,
                             PRInt32& aResultLength);
nsresult MultiByteToWideChar(int aCodePage, const char* aSrc,
                             PRInt32 aSrcLength, nsAutoChar16Buffer& aResult,
                             PRInt32& aResultLength);

#endif
