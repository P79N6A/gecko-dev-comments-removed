






































#ifndef __NS_ISVGVALUE_H__
#define __NS_ISVGVALUE_H__

#include "nsISupports.h"
#include "nsString.h"

class nsISVGValueObserver;














#define NS_ISVGVALUE_IID \
{ 0xd8299a5e, 0xaf9a, 0x4bad, { 0x98, 0x45, 0xfb, 0x1b, 0x6e, 0x2e, 0xed, 0x19 } }


class nsISVGValue : public nsISupports
{
public:
  enum modificationType {
    mod_other = 0,
    mod_context,
    mod_die
  };

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGVALUE_IID)

  NS_IMETHOD SetValueString(const nsAString& aValue)=0;
  NS_IMETHOD GetValueString(nsAString& aValue)=0;

  NS_IMETHOD AddObserver(nsISVGValueObserver* observer)=0;
  NS_IMETHOD RemoveObserver(nsISVGValueObserver* observer)=0;

  NS_IMETHOD BeginBatchUpdate()=0;
  NS_IMETHOD EndBatchUpdate()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGValue, NS_ISVGVALUE_IID)

nsresult
NS_CreateSVGGenericStringValue(const nsAString& aValue, nsISVGValue** aResult);

nsresult
NS_CreateSVGStringProxyValue(nsISVGValue* proxiedValue, nsISVGValue** aResult);

#endif

