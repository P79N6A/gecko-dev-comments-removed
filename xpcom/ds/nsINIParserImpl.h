





#ifndef nsINIParserImpl_h__
#define nsINIParserImpl_h__

#include "nsIINIParser.h"
#include "nsIFactory.h"
#include "mozilla/Attributes.h"

#define NS_INIPARSERFACTORY_CID \
{ 0xdfac10a9, 0xdd24, 0x43cf, \
  { 0xa0, 0x95, 0x6f, 0xfa, 0x2e, 0x4b, 0x6a, 0x6c } }

#define NS_INIPARSERFACTORY_CONTRACTID \
  "@mozilla.org/xpcom/ini-parser-factory;1"

class nsINIParserFactory final
  : public nsIINIParserFactory
  , public nsIFactory
{
  ~nsINIParserFactory() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINIPARSERFACTORY
  NS_DECL_NSIFACTORY
};

#endif
