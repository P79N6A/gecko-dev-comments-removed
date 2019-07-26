









#ifndef _nsILayoutHistoryState_h
#define _nsILayoutHistoryState_h

#include "nsISupports.h"
#include "nsStringFwd.h"

class nsPresState;
template<typename> struct already_AddRefed;

#define NS_ILAYOUTHISTORYSTATE_IID \
{ 0x5208993e, 0xd812, 0x431e, \
  { 0x95, 0x9c, 0xc3, 0x84, 0x5b, 0x6e, 0x5a, 0xce } }

class nsILayoutHistoryState : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILAYOUTHISTORYSTATE_IID)

  





  virtual void AddState(const nsCString& aKey, nsPresState* aState) = 0;

  


  virtual nsPresState* GetState(const nsCString& aKey) = 0;

  


  virtual void RemoveState(const nsCString& aKey) = 0;

  


  virtual bool HasStates() const = 0;

  



  virtual void SetScrollPositionOnly(const bool aFlag) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILayoutHistoryState,
                              NS_ILAYOUTHISTORYSTATE_IID)

already_AddRefed<nsILayoutHistoryState>
NS_NewLayoutHistoryState();

#endif 

