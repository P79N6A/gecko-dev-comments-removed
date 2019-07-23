





































#ifndef __NS_SVGVALUE_H__
#define __NS_SVGVALUE_H__

#include "nscore.h"
#include "nsISVGValue.h"
#include "nsAutoPtr.h"
#include "nsVoidArray.h"
#include "nsISVGValueObserver.h"

class nsSVGValue : public nsISVGValue
{
protected:
  nsSVGValue();
  virtual ~nsSVGValue();

  
  
  void WillModify(modificationType aModType = mod_other);
  void DidModify(modificationType aModType = mod_other);

  friend class nsSVGValueAutoNotifier;
  
public:
  
  NS_IMETHOD AddObserver(nsISVGValueObserver* observer);
  NS_IMETHOD RemoveObserver(nsISVGValueObserver* observer);
  NS_IMETHOD BeginBatchUpdate();
  NS_IMETHOD EndBatchUpdate();

  typedef
  NS_STDCALL_FUNCPROTO(nsresult,
                       SVGObserverNotifyFunction,
                       nsISVGValueObserver, DidModifySVGObservable,
                       (nsISVGValue*, nsISVGValue::modificationType));

protected:
  
  void ReleaseObservers();
  void NotifyObservers(SVGObserverNotifyFunction f,
                       modificationType aModType);
  PRInt32 GetModifyNestCount() { return mModifyNestCount; }
private:
  virtual void OnDidModify(){}; 
  
  nsSmallVoidArray mObservers;
  PRInt32 mModifyNestCount;
};




class nsSVGValueAutoNotifier
{
public:
  nsSVGValueAutoNotifier(nsSVGValue* aVal,
                         nsISVGValue::modificationType aModType =
                         nsISVGValue::mod_other)
    : mVal(aVal)
    , mModType(aModType)
  {
    mVal->WillModify(mModType);
  }

  ~nsSVGValueAutoNotifier()
  {
    mVal->DidModify(mModType);
  }

private:
  nsRefPtr<nsSVGValue> mVal;
  nsISVGValue::modificationType mModType;
};

#endif 
