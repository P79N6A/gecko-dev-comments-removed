
































































































#ifndef _mozISanitizingSerializer_h__
#define _mozISanitizingSerializer_h__

#include "nsISupports.h"

class nsAString;

#define MOZ_SANITIZINGHTMLSERIALIZER_CONTRACTID "@mozilla.org/layout/htmlsanitizer;1"


#define MOZ_ISANITIZINGHTMLSERIALIZER_IID_STR "feca3c34-205e-4ae5-bd1c-03c686ff012b"

#define MOZ_ISANITIZINGHTMLSERIALIZER_IID \
  {0xfeca3c34, 0x205e, 0x4ae5, \
    { 0xbd, 0x1c, 0x03, 0xc6, 0x86, 0xff, 0x01, 0x2b }}

class mozISanitizingHTMLSerializer : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(MOZ_ISANITIZINGHTMLSERIALIZER_IID)

  NS_IMETHOD Initialize(nsAString* aOutString,
                        PRUint32 aFlags,
                        const nsAString& allowedTags) = 0;
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(mozISanitizingHTMLSerializer,
                              MOZ_ISANITIZINGHTMLSERIALIZER_IID)

#endif
