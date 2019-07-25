








#ifndef _AccessibleWrap_H_
#define _AccessibleWrap_H_

#include <objc/objc.h>

#include "Accessible.h"
#include "nsAccUtils.h"
#include "States.h"

#include "nsCOMPtr.h"
#include "nsRect.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

#if defined(__OBJC__)
@class mozAccessible;
#endif

class AccessibleWrap : public Accessible
{
public: 
  AccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~AccessibleWrap();
    
  


  NS_IMETHOD GetNativeInterface (void** aOutAccessible);
  
  




  virtual Class GetNativeType ();

  virtual void Shutdown ();
  virtual void InvalidateChildren();

  virtual bool AppendChild(Accessible* aAccessible);
  virtual bool RemoveChild(Accessible* aAccessible);

  virtual nsresult HandleAccEvent(AccEvent* aEvent);

  




  bool IsIgnored();
  
  inline bool HasPopup () 
    { return (NativeState() & mozilla::a11y::states::HASPOPUP); }
  
  



  void GetUnignoredChildren(nsTArray<Accessible*>* aChildrenArray);
  Accessible* GetUnignoredParent() const;

protected:

  virtual nsresult FirePlatformEvent(AccEvent* aEvent);

  


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


typedef class nsHTMLTableCellAccessible    nsHTMLTableCellAccessibleWrap;
typedef class nsHTMLTableAccessible        nsHTMLTableAccessibleWrap;

#endif
