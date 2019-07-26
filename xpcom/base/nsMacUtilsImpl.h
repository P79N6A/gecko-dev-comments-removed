




#ifndef nsMacUtilsImpl_h___
#define nsMacUtilsImpl_h___

#include "nsIMacUtils.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsMacUtilsImpl MOZ_FINAL : public nsIMacUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMACUTILS

  nsMacUtilsImpl() {}

private:
  ~nsMacUtilsImpl() {}

  nsresult GetArchString(nsAString& archString);

  
  
  nsString mBinaryArchs;
};



#define NS_MACUTILSIMPL_CID \
 {0x697BD3FD, 0x43E5, 0x41CE, {0xAD, 0x5E, 0xC3, 0x39, 0x17, 0x5C, 0x08, 0x18}}
#define NS_MACUTILSIMPL_CONTRACTID "@mozilla.org/xpcom/mac-utils;1"

#endif 
