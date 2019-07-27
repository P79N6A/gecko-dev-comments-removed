








#ifndef _AccessibleWrap_H_
#define _AccessibleWrap_H_

#include <objc/objc.h>

#include "Accessible.h"
#include "States.h"

#include "nsCOMPtr.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

#if defined(__OBJC__)
@class mozAccessible;
#endif

namespace mozilla {
namespace a11y {

class AccessibleWrap : public Accessible
{
public: 
  AccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~AccessibleWrap();

  


  virtual void GetNativeInterface(void** aOutAccessible) MOZ_OVERRIDE;

  




  virtual Class GetNativeType ();

  virtual void Shutdown () MOZ_OVERRIDE;
  virtual void InvalidateChildren() MOZ_OVERRIDE;

  virtual bool InsertChildAt(uint32_t aIdx, Accessible* aChild) MOZ_OVERRIDE;
  virtual bool RemoveChild(Accessible* aAccessible) MOZ_OVERRIDE;

  virtual nsresult HandleAccEvent(AccEvent* aEvent) MOZ_OVERRIDE;

  




  bool IsIgnored();
  
  inline bool HasPopup () 
    { return (NativeState() & mozilla::a11y::states::HASPOPUP); }
  
  



  void GetUnignoredChildren(nsTArray<Accessible*>* aChildrenArray);
  Accessible* GetUnignoredParent() const;

protected:

  


  bool AncestorIsFlat();

  


#if defined(__OBJC__)
  mozAccessible* GetNativeObject();
#else
  id GetNativeObject();
#endif

private:

  




#if defined(__OBJC__)
  
  mozAccessible* mNativeObject;
#else
  id mNativeObject;
#endif

  




  bool mNativeInited;  
};

} 
} 

#endif
