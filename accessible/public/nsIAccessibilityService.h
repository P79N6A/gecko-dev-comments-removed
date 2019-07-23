







































#ifndef _nsIAccessibilityService_h_
#define _nsIAccessibilityService_h_

#include "nsISupports.h"
#include "nsIAccessibleRetrieval.h"

class nsIDocument;
class nsIFrame;
class nsObjectFrame;
class nsIContent;

#define NS_IACCESSIBILITYSERVICE_IID \
{0x33fa2a8d, 0x72e5, 0x4b8b,         \
  {0xbb, 0x17, 0x6b, 0x22, 0x79, 0x05, 0x5c, 0x6c} }

class nsIAccessibilityService : public nsIAccessibleRetrieval
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IACCESSIBILITYSERVICE_IID)

  


  virtual nsresult CreateOuterDocAccessible(nsIDOMNode *aNode,
                                            nsIAccessible **aAccessible) = 0;

  virtual nsresult CreateHTML4ButtonAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHyperTextAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLBRAccessible(nsIFrame *aFrame,
                                          nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLButtonAccessible(nsIFrame *aFrame,
                                              nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLLIAccessible(nsIFrame *aFrame,
                                          nsIFrame *aBulletFrame,
                                          const nsAString& aBulletText,
                                          nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLCheckboxAccessible(nsIFrame *aFrame,
                                                nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLComboboxAccessible(nsIDOMNode *aNode,
                                                nsIWeakReference *aPresShell,
                                                nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLGenericAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLGroupboxAccessible(nsIFrame *aFrame,
                                                nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLHRAccessible(nsIFrame *aFrame,
                                          nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLImageAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLLabelAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLListboxAccessible(nsIDOMNode *aNode,
                                               nsIWeakReference *aPresShell,
                                               nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLMediaAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLObjectFrameAccessible(nsObjectFrame *aFrame,
                                                   nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLRadioButtonAccessible(nsIFrame *aFrame,
                                                   nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLSelectOptionAccessible(nsIDOMNode *aNode,
                                                    nsIAccessible *aAccParent,
                                                    nsIWeakReference *aPresShell,
                                                    nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLTableAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLTableCellAccessible(nsIFrame *aFrame,
                                                 nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLTextAccessible(nsIFrame *aFrame,
                                            nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLTextFieldAccessible(nsIFrame *aFrame,
                                                 nsIAccessible **aAccessible) = 0;
  virtual nsresult CreateHTMLCaptionAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible) = 0;

  



  virtual nsresult AddNativeRootAccessible(void *aAtkAccessible,
                                           nsIAccessible **aAccessible) = 0;
  virtual nsresult
    RemoveNativeRootAccessible(nsIAccessible *aRootAccessible) = 0;

  


  enum {
    NODE_APPEND = 0x01,
    NODE_REMOVE = 0x02,
    NODE_SIGNIFICANT_CHANGE = 0x03,
    FRAME_SHOW = 0x04,
    FRAME_HIDE = 0x05,
    FRAME_SIGNIFICANT_CHANGE = 0x06
  };

  






  virtual nsresult InvalidateSubtreeFor(nsIPresShell *aPresShell,
                                        nsIContent *aContent,
                                        PRUint32 aChangeType) = 0;

  



  virtual nsresult NotifyOfAnchorJumpTo(nsIContent *aTarget) = 0;

  





  virtual nsresult FireAccessibleEvent(PRUint32 aEvent,
                                       nsIAccessible *aTarget) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAccessibilityService,
                              NS_IACCESSIBILITYSERVICE_IID)



#define NS_ACCESSIBILITY_SERVICE_CID \
{ 0xde401c37, 0x9a7f, 0x4278, { 0xa6, 0xf8, 0x3d, 0xe2, 0x83, 0x39, 0x89, 0xef } }

extern nsresult
NS_GetAccessibilityService(nsIAccessibilityService** aResult);

#endif
