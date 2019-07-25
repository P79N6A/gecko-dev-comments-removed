







































#ifndef _nsIAccessibilityService_h_
#define _nsIAccessibilityService_h_

#include "nsIAccessibleRetrieval.h"
#include "nsIAccessibleEvent.h"

#include "nsAutoPtr.h"

class nsAccessible;
class nsINode;
class nsIContent;
class nsIDocument;
class nsIFrame;
class nsIPresShell;
class nsObjectFrame;


#define NS_IACCESSIBILITYSERVICE_IID \
{ 0x10ff6dca, 0xb219, 0x4b64, \
 { 0x9a, 0x4c, 0x67, 0xa6, 0x2b, 0x86, 0xed, 0xce } }

class nsIAccessibilityService : public nsIAccessibleRetrieval
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IACCESSIBILITYSERVICE_IID)

  






  virtual nsAccessible* GetAccessibleInShell(nsINode* aNode,
                                             nsIPresShell* aPresShell) = 0;

  







  virtual nsAccessible* GetRootDocumentAccessible(nsIPresShell* aPresShell,
                                                  bool aCanCreate) = 0;

  


  virtual already_AddRefed<nsAccessible>
    CreateHTMLBRAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTML4ButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLCaptionAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLCheckboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLComboboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLGroupboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLHRAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLImageAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLLabelAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLLIAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLListboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLMediaAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLObjectFrameAccessible(nsObjectFrame* aFrame, nsIContent* aContent,
                                    nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLRadioButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLTableAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLTableCellAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLTextAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHTMLTextFieldAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateHyperTextAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;
  virtual already_AddRefed<nsAccessible>
    CreateOuterDocAccessible(nsIContent* aContent, nsIPresShell* aPresShell) = 0;

  



  virtual nsAccessible* AddNativeRootAccessible(void* aAtkAccessible) = 0;
  virtual void RemoveNativeRootAccessible(nsAccessible* aRootAccessible) = 0;

  



  virtual void ContentRangeInserted(nsIPresShell* aPresShell,
                                    nsIContent* aContainer,
                                    nsIContent* aStartChild,
                                    nsIContent* aEndChild) = 0;

  


  virtual void ContentRemoved(nsIPresShell* aPresShell, nsIContent* aContainer,
                              nsIContent* aChild) = 0;

  



  virtual void NotifyOfAnchorJumpTo(nsIContent *aTarget) = 0;

  



  virtual void PresShellDestroyed(nsIPresShell *aPresShell) = 0;

  


  virtual void RecreateAccessible(nsIPresShell* aPresShell,
                                  nsIContent* aContent) = 0;

  





  virtual void FireAccessibleEvent(PRUint32 aEvent, nsAccessible* aTarget) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAccessibilityService,
                              NS_IACCESSIBILITYSERVICE_IID)



#define NS_ACCESSIBILITY_SERVICE_CID \
{ 0xde401c37, 0x9a7f, 0x4278, { 0xa6, 0xf8, 0x3d, 0xe2, 0x83, 0x39, 0x89, 0xef } }

extern nsresult
NS_GetAccessibilityService(nsIAccessibilityService** aResult);

#endif
