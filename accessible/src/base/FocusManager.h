



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




#ifdef A11YDEBUG_FOCUS


#define A11YDEBUG_FOCUS_STARTBLOCK                                             \
  printf("  {\n    ");

#define A11YDEBUG_FOCUS_ENDBLOCK                                               \
  printf("\n  }\n");

#define A11YDEBUG_FOCUS_BLOCKOFFSET                                            \
  printf("    ");

#define A11YDEBUG_FOCUS_LOG_TIME                                               \
  {                                                                            \
    PRIntervalTime time = PR_IntervalNow();                                    \
    uint32_t mins = (PR_IntervalToSeconds(time) / 60) % 60;                    \
    uint32_t secs = PR_IntervalToSeconds(time) % 60;                           \
    uint32_t msecs = PR_IntervalToMilliseconds(time) % 1000;                   \
    printf("Time: %2d:%2d.%3d\n", mins, secs, msecs);                          \
  }

#define A11YDEBUG_FOCUS_LOG_DOMNODE(aNode)                                     \
  if (aNode) {                                                                 \
    if (aNode->IsElement()) {                                                  \
      dom::Element* targetElm = aNode->AsElement();                            \
      nsCAutoString tag;                                                       \
      targetElm->Tag()->ToUTF8String(tag);                                     \
      nsCAutoString id;                                                        \
      nsIAtom* atomid = targetElm->GetID();                                    \
      if (atomid)                                                              \
        atomid->ToUTF8String(id);                                              \
      printf("element %s@id='%s': %p", tag.get(), id.get(), (void*)aNode);     \
    } else if (aNode->IsNodeOfType(nsINode::eDOCUMENT)) {                      \
      nsCOMPtr<nsIDocument> document = do_QueryInterface(aNode);               \
      nsIURI* uri = document->GetDocumentURI();                                \
      nsCAutoString spec;                                                      \
      uri->GetSpec(spec);                                                      \
      printf("document: %p; uri: %s", (void*)aNode, spec.get());               \
    }                                                                          \
  }

#define A11YDEBUG_FOCUS_LOG_ACCESSIBLE(aAccessible)                            \
  printf("accessible: %p; ", (void*)aAccessible);                              \
  if (aAccessible) {                                                           \
    nsAutoString role;                                                         \
    GetAccService()->GetStringRole(aAccessible->Role(), role);                 \
    nsAutoString name;                                                         \
    aAccessible->Name(name);                                                   \
    printf(" role: %s, name: %s; ", NS_ConvertUTF16toUTF8(role).get(),         \
           NS_ConvertUTF16toUTF8(name).get());                                 \
    A11YDEBUG_FOCUS_LOG_DOMNODE(aAccessible->GetNode())                        \
  }


#define A11YDEBUG_FOCUS_LOG_DOMTARGET(aMsg, aTarget)                           \
  A11YDEBUG_FOCUS_STARTBLOCK                                                   \
  printf(aMsg "\n");                                                           \
  if (aTarget) {                                                               \
    A11YDEBUG_FOCUS_BLOCKOFFSET                                                \
    A11YDEBUG_FOCUS_LOG_DOMNODE(aTarget)                                       \
  }                                                                            \
  A11YDEBUG_FOCUS_ENDBLOCK

#define A11YDEBUG_FOCUS_LOG_ACCTARGET(aMsg, aTarget)                           \
  A11YDEBUG_FOCUS_STARTBLOCK                                                   \
  printf(aMsg "\n");                                                           \
  A11YDEBUG_FOCUS_BLOCKOFFSET                                                  \
  A11YDEBUG_FOCUS_LOG_ACCESSIBLE(aTarget)                                      \
  A11YDEBUG_FOCUS_ENDBLOCK

#define A11YDEBUG_FOCUS_LOG_WIDGET(aMsg, aWidget)                              \
  A11YDEBUG_FOCUS_STARTBLOCK                                                   \
  printf(aMsg "\n");                                                           \
  A11YDEBUG_FOCUS_BLOCKOFFSET                                                  \
  A11YDEBUG_FOCUS_LOG_ACCESSIBLE(aWidget)                                      \
  printf("; widget is active: %s, has operable items: %s",                     \
         (aWidget && aWidget->IsActiveWidget() ? "true" : "false"),            \
         (aWidget && aWidget->AreItemsOperable() ? "true" : "false"));         \
  A11YDEBUG_FOCUS_ENDBLOCK

#define A11YDEBUG_FOCUS_NOTIFICATION_SUPPORTSTARGET(aMsg, aTargetMsg, aTarget) \
  printf("\nA11Y FOCUS: " aMsg ". ");                                          \
  A11YDEBUG_FOCUS_LOG_TIME                                                     \
  if (aTarget) {                                                               \
    A11YDEBUG_FOCUS_STARTBLOCK                                                 \
    printf(aTargetMsg "\n");                                                   \
    A11YDEBUG_FOCUS_BLOCKOFFSET                                                \
    nsCOMPtr<nsINode> targetNode(do_QueryInterface(aTarget));                  \
    if (targetNode) {                                                          \
      A11YDEBUG_FOCUS_LOG_DOMNODE(targetNode)                                  \
    } else {                                                                   \
      printf("window: %p", (void*)aTarget);                                    \
    }                                                                          \
    A11YDEBUG_FOCUS_ENDBLOCK                                                   \
  }

#define A11YDEBUG_FOCUS_NOTIFICATION_DOMTARGET(aMsg, aTargetMsg, aTarget)      \
  printf("\nA11Y FOCUS: " aMsg ". ");                                          \
  A11YDEBUG_FOCUS_LOG_TIME                                                     \
  A11YDEBUG_FOCUS_LOG_DOMTARGET(aTargetMsg, aTarget)

#define A11YDEBUG_FOCUS_NOTIFICATION_ACCTARGET(aMsg, aTargetMsg, aTarget)      \
  printf("\nA11Y FOCUS: " aMsg ". ");                                          \
  A11YDEBUG_FOCUS_LOG_TIME                                                     \
  A11YDEBUG_FOCUS_LOG_ACCTARGET(aTargetMsg, aTarget)

#define A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE(aMsg, aTarget)                  \
  A11YDEBUG_FOCUS_LOG_ACCTARGET("Caused by: " aMsg, aTarget)

#else
#define A11YDEBUG_FOCUS_LOG_DOMTARGET(aMsg, aTarget)
#define A11YDEBUG_FOCUS_LOG_ACCTARGET(aMsg, aTarget)
#define A11YDEBUG_FOCUS_LOG_WIDGET(aMsg, aWidget)
#define A11YDEBUG_FOCUS_NOTIFICATION_SUPPORTSTARGET(aMsg, aTargetMsg, aTarget)
#define A11YDEBUG_FOCUS_NOTIFICATION_DOMTARGET(aMsg, aTargetMsg, aTarget)
#define A11YDEBUG_FOCUS_NOTIFICATION_ACCTARGET(aMsg, aTargetMsg, aTarget)
#define A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE(aMsg, aTarget)
#endif

#endif
