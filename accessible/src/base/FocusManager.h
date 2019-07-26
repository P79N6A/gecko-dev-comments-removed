



#ifndef mozilla_a11y_FocusManager_h_
#define mozilla_a11y_FocusManager_h_

#include "nsAutoPtr.h"
#include "mozilla/dom/Element.h"

class AccEvent;
class Accessible;
class DocAccessible;

namespace mozilla {
namespace a11y {




class FocusManager
{
public:
  virtual ~FocusManager();

  


  Accessible* FocusedAccessible() const;

  


  bool IsFocused(const Accessible* aAccessible) const;

  



  inline bool IsActiveItem(const Accessible* aAccessible)
    { return aAccessible == mActiveItem; }

  


  inline bool HasDOMFocus(const nsINode* aNode) const
    { return aNode == FocusedDOMNode(); }

  


  bool IsFocusWithin(const Accessible* aContainer) const;

  



  enum FocusDisposition {
    eNone,
    eFocused,
    eContainsFocus,
    eContainedByFocus
  };
  FocusDisposition IsInOrContainsFocus(const Accessible* aAccessible) const;

  
  

  


  void NotifyOfDOMFocus(nsISupports* aTarget);

  


  void NotifyOfDOMBlur(nsISupports* aTarget);

  



  void ActiveItemChanged(Accessible* aItem, bool aCheckIfActive = true);

  


  void ForceFocusEvent();

  


  void DispatchFocusEvent(DocAccessible* aDocument, Accessible* aTarget);

  


  void ProcessDOMFocus(nsINode* aTarget);

  



  void ProcessFocusEvent(AccEvent* aEvent);

protected:
  FocusManager();

private:
  FocusManager(const FocusManager&);
  FocusManager& operator =(const FocusManager&);

  


  nsINode* FocusedDOMNode() const;

  


  nsIDocument* FocusedDOMDocument() const;

private:
  nsRefPtr<Accessible> mActiveItem;
  nsRefPtr<Accessible> mActiveARIAMenubar;
};

} 
} 

#endif
