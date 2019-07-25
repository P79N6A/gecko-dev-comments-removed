









































#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include <objc/objc.h>

#include "nsAccessible.h"
#include "nsAccUtils.h"
#include "States.h"

#include "nsCOMPtr.h"
#include "nsRect.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

#if defined(__OBJC__)
@class mozAccessible;
#endif

class nsAccessibleWrap : public nsAccessible
{
public: 
  nsAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsAccessibleWrap();
    
  


  NS_IMETHOD GetNativeInterface (void** aOutAccessible);
  
  




  virtual Class GetNativeType ();

  virtual void Shutdown ();
  virtual void InvalidateChildren();

  virtual bool AppendChild(nsAccessible* aAccessible);
  virtual bool RemoveChild(nsAccessible* aAccessible);

  virtual nsresult HandleAccEvent(AccEvent* aEvent);

  




  bool IsIgnored();
  
  inline bool HasPopup () 
    { return (NativeState() & mozilla::a11y::states::HASPOPUP); }
  
  



  void GetUnignoredChildren(nsTArray<nsAccessible*>* aChildrenArray);
  nsAccessible* GetUnignoredParent() const;
    
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
