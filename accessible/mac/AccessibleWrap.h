








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

  


  virtual void GetNativeInterface(void** aOutAccessible) override;

  




  virtual Class GetNativeType ();

  virtual void Shutdown () override;
  virtual void InvalidateChildren() override;

  virtual bool InsertChildAt(uint32_t aIdx, Accessible* aChild) override;
  virtual bool RemoveChild(Accessible* aAccessible) override;

  virtual nsresult HandleAccEvent(AccEvent* aEvent) override;

  




  bool IsIgnored();

  



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

#if defined(__OBJC__)
  void FireNativeEvent(mozAccessible* aNativeAcc, uint32_t aEventType);
#else
  void FireNativeEvent(id aNativeAcc, uint32_t aEventType);
#endif

Class GetTypeFromRole(roles::Role aRole);

} 
} 

#endif
