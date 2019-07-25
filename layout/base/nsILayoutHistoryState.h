










































#ifndef _nsILayoutHistoryState_h
#define _nsILayoutHistoryState_h

#include "nsISupports.h"
#include "nsStringFwd.h"

class nsPresState;

#define NS_ILAYOUTHISTORYSTATE_IID \
{ 0x003919e2, 0x5e6b, 0x4d76, \
 { 0xa9, 0x4f, 0xbc, 0x5d, 0x15, 0x5b, 0x1c, 0x67 } }

class nsILayoutHistoryState : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILAYOUTHISTORYSTATE_IID)

  





  NS_IMETHOD AddState(const nsCString& aKey, nsPresState* aState) = 0;

  


  NS_IMETHOD GetState(const nsCString& aKey, nsPresState** aState) = 0;

  


  NS_IMETHOD RemoveState(const nsCString& aKey) = 0;

  


  NS_IMETHOD_(bool) HasStates() const = 0;

  



   NS_IMETHOD SetScrollPositionOnly(const bool aFlag) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILayoutHistoryState,
                              NS_ILAYOUTHISTORYSTATE_IID)

nsresult
NS_NewLayoutHistoryState(nsILayoutHistoryState** aState);

#endif 

