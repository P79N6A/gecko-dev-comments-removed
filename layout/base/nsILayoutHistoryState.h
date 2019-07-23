










































#ifndef _nsILayoutHistoryState_h
#define _nsILayoutHistoryState_h

#include "nsISupports.h"
#include "nsStringFwd.h"

class nsPresState;

#define NS_ILAYOUTHISTORYSTATE_IID \
{0xe6abfb7c, 0x6624, 0x4b4d, \
{0x9d, 0xfe, 0xea, 0x62, 0xae, 0xfe, 0x03, 0x31}}

class nsILayoutHistoryState : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILAYOUTHISTORYSTATE_IID)

  





  NS_IMETHOD AddState(const nsCString& aKey, nsPresState* aState) = 0;

  


  NS_IMETHOD GetState(const nsCString& aKey, nsPresState** aState) = 0;

  


  NS_IMETHOD RemoveState(const nsCString& aKey) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILayoutHistoryState,
                              NS_ILAYOUTHISTORYSTATE_IID)

nsresult
NS_NewLayoutHistoryState(nsILayoutHistoryState** aState);

#endif 

