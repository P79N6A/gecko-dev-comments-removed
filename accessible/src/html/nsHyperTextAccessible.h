






































#ifndef _nsHyperTextAccessible_H_
#define _nsHyperTextAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"
#include "nsAccessibleEventData.h"
#include "nsFrameSelection.h"
#include "nsISelectionController.h"

enum EGetTextType { eGetBefore=-1, eGetAt=0, eGetAfter=1 };



const PRUnichar kEmbeddedObjectChar = 0xfffc;
const PRUnichar kForcedNewLineChar = '\n';

#define NS_HYPERTEXTACCESSIBLE_IMPL_CID                 \
{  /* 245f3bc9-224f-4839-a92e-95239705f30b */           \
  0x245f3bc9,                                           \
  0x224f,                                               \
  0x4839,                                               \
  { 0xa9, 0x2e, 0x95, 0x23, 0x97, 0x05, 0xf3, 0x0b }    \
}





class nsHyperTextAccessible : public nsAccessibleWrap,
                              public nsIAccessibleText,
                              public nsIAccessibleHyperText,
                              public nsIAccessibleEditableText
{
public:
  nsHyperTextAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXT
  NS_DECL_NSIACCESSIBLEHYPERTEXT
  NS_DECL_NSIACCESSIBLEEDITABLETEXT
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_HYPERTEXTACCESSIBLE_IMPL_CID)

  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  void CacheChildren();

  
  static nsresult ContentToRenderedOffset(nsIFrame *aFrame, PRInt32 aContentOffset,
                                          PRUint32 *aRenderedOffset);
  
  
  static nsresult RenderedToContentOffset(nsIFrame *aFrame, PRUint32 aRenderedOffset,
                                          PRInt32 *aContentOffset);

  














  nsresult DOMPointToHypertextOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset,
                                     PRInt32 *aHypertextOffset,
                                     nsIAccessible **aFinalAccessible = nsnull);

protected:
  









  nsresult GetTextHelper(EGetTextType aType, nsAccessibleTextBoundary aBoundaryType,
                         PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                         nsAString & aText);

  









  PRInt32 GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame, PRInt32 aFromOffset,
                            nsSelectionAmount aAmount, nsDirection aDirection, PRBool aNeedsStart);
  











  nsIFrame* GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset, nsAString *aText = nsnull,
                          nsIFrame **aEndFrame = nsnull, nsIntRect *aBoundsRect = nsnull);

  nsIntRect GetBoundsForString(nsIFrame *aFrame, PRUint32 aStartRenderedOffset, PRUint32 aEndRenderedOffset);

  
  nsresult GetSelections(nsISelectionController **aSelCon, nsISelection **aDomSel);
  nsresult SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHyperTextAccessible,
                              NS_HYPERTEXTACCESSIBLE_IMPL_CID)

#endif  

