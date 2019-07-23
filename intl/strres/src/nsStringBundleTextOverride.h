






































#ifndef nsStringBundleTextOverride_h__
#define nsStringBundleTextOverride_h__

#include "nsIStringBundleOverride.h"
#include "nsCOMPtr.h"
#include "nsIPersistentProperties2.h"


#define NS_STRINGBUNDLETEXTOVERRIDE_CID \
  { 0x6316c6ce, 0x12d3, 0x479e, \
  { 0x8f, 0x53, 0xe2, 0x89, 0x35, 0x14, 0x12, 0xb8 } }


#define NS_STRINGBUNDLETEXTOVERRIDE_CONTRACTID \
    "@mozilla.org/intl/stringbundle/text-override;1"



class nsStringBundleTextOverride : public nsIStringBundleOverride
{
 public:
    nsStringBundleTextOverride() { }
    virtual ~nsStringBundleTextOverride() {}

    nsresult Init();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTRINGBUNDLEOVERRIDE

private:
    nsCOMPtr<nsIPersistentProperties> mValues;
    
};

#endif
