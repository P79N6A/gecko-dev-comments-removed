






































#ifndef _nsHyperTextAccessible_H_
#define _nsHyperTextAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"
#include "nsAccessibleEventData.h"
#include "nsTextAttrs.h"

#include "nsFrameSelection.h"
#include "nsISelectionController.h"

enum EGetTextType { eGetBefore=-1, eGetAt=0, eGetAfter=1 };



const PRUnichar kEmbeddedObjectChar = 0xfffc;
const PRUnichar kImaginaryEmbeddedObjectChar = ' ';
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

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  static nsresult ContentToRenderedOffset(nsIFrame *aFrame, PRInt32 aContentOffset,
                                          PRUint32 *aRenderedOffset);
  
  
  static nsresult RenderedToContentOffset(nsIFrame *aFrame, PRUint32 aRenderedOffset,
                                          PRInt32 *aContentOffset);

  
























  nsresult DOMPointToHypertextOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset,
                                     PRInt32 *aHypertextOffset,
                                     nsIAccessible **aFinalAccessible = nsnull,
                                     PRBool aIsEndOffset = PR_FALSE);

  






  nsresult HypertextOffsetToDOMPoint(PRInt32 aHTOffset,
                                     nsIDOMNode **aNode,
                                     PRInt32 *aOffset);

  









  nsresult HypertextOffsetsToDOMRange(PRInt32 aStartHTOffset,
                                      PRInt32 aEndHTOffset,
                                      nsIDOMNode **aStartNode,
                                      PRInt32 *aStartOffset,
                                      nsIDOMNode **aEndNode,
                                      PRInt32 *aEndOffset);

protected:

  
  virtual void CacheChildren();

  

  









  nsresult GetTextHelper(EGetTextType aType, nsAccessibleTextBoundary aBoundaryType,
                         PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                         nsAString & aText);

  













  PRInt32 GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame,
                            PRInt32 aFromOffset, nsIAccessible *aFromAccessible,
                            nsSelectionAmount aAmount, nsDirection aDirection,
                            PRBool aNeedsStart);

  
























  nsIFrame* GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset,
                          nsAString *aText = nsnull,
                          nsIFrame **aEndFrame = nsnull,
                          nsIntRect *aBoundsRect = nsnull,
                          nsIAccessible **aStartAcc = nsnull,
                          nsIAccessible **aEndAcc = nsnull);

  nsIntRect GetBoundsForString(nsIFrame *aFrame, PRUint32 aStartRenderedOffset, PRUint32 aEndRenderedOffset);

  

    











  nsresult GetSelections(PRInt16 aType,
                         nsISelectionController **aSelCon,
                         nsISelection **aDomSel = nsnull,
                         nsCOMArray<nsIDOMRange>* aRanges = nsnull);

  nsresult SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos);

  




  PRInt32 GetCaretLineNumber();

  
  nsresult GetDOMPointByFrameOffset(nsIFrame *aFrame, PRInt32 aOffset,
                                    nsIAccessible *aAccessible,
                                    nsIDOMNode **aNode, PRInt32 *aNodeOffset);

  
  












  nsresult DOMRangeBoundToHypertextOffset(nsIDOMRange *aRange,
                                          PRBool aIsStartBound,
                                          PRBool aIsStartOffset,
                                          PRInt32 *aHTOffset);

  












  nsresult GetSpellTextAttribute(nsIDOMNode *aNode, PRInt32 aNodeOffset,
                                 PRInt32 *aStartOffset,
                                 PRInt32 *aEndOffset,
                                 nsIPersistentProperties *aAttributes);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHyperTextAccessible,
                              NS_HYPERTEXTACCESSIBLE_IMPL_CID)

#endif  

