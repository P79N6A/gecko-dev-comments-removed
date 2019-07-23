






































#ifndef __UMacUnicode__
#define __UMacUnicode__

#include "nsString.h"
#include "nsCOMPtr.h"

#include <UnicodeConverter.h>

class CPlatformUCSConversion {
public:
     CPlatformUCSConversion();
     virtual ~CPlatformUCSConversion(){};
     
     static CPlatformUCSConversion* GetInstance();
          
     NS_IMETHOD UCSToPlatform(const nsAString& aIn, nsACString& aOut);
     NS_IMETHOD UCSToPlatform(const nsAString& aIn, Str255& aOut);  
  
     NS_IMETHOD PlatformToUCS(const nsACString& ain, nsAString& aOut);  
     NS_IMETHOD PlatformToUCS(const Str255& aIn, nsAString& aOut);  

private:
     static CPlatformUCSConversion *mgInstance;
     static UnicodeToTextInfo sEncoderInfo;
     static TextToUnicodeInfo sDecoderInfo;
     
     nsresult PrepareEncoder();
     nsresult PrepareDecoder();
};

#endif 
