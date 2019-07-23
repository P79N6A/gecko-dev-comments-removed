














































#ifndef  IPARSERFILTER
#define  IPARSERFILTER

#include "nsISupports.h"

class CToken;

#define NS_IPARSERFILTER_IID     \
  {0x14d6ff0,  0x0610,  0x11d2,  \
  {0x8c, 0x3f, 0x00,    0x80, 0x5f, 0x8a, 0x1d, 0xb7}}


class nsIParserFilter : public nsISupports {
  public:

   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPARSERFILTER_IID)
      
   NS_IMETHOD RawBuffer(const char * buffer, PRUint32 * buffer_length) = 0;

   NS_IMETHOD WillAddToken(CToken & token) = 0;

   NS_IMETHOD ProcessTokens(  void ) = 0;

   NS_IMETHOD Finish() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIParserFilter, NS_IPARSERFILTER_IID)


#endif

