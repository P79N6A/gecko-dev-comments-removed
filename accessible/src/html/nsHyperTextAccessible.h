






































#ifndef _nsHyperTextAccessible_H_
#define _nsHyperTextAccessible_H_

#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"

#include "AccCollector.h"
#include "nsAccessibleWrap.h"
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
  nsHyperTextAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXT
  NS_DECL_NSIACCESSIBLEHYPERTEXT
  NS_DECL_NSIACCESSIBLEEDITABLETEXT
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_HYPERTEXTACCESSIBLE_IMPL_CID)

  
  virtual PRInt32 GetLevelInternal();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  virtual void InvalidateChildren();
  virtual PRBool RemoveChild(nsAccessible* aAccessible);

  

  
  static nsresult ContentToRenderedOffset(nsIFrame *aFrame, PRInt32 aContentOffset,
                                          PRUint32 *aRenderedOffset);
  
  
  static nsresult RenderedToContentOffset(nsIFrame *aFrame, PRUint32 aRenderedOffset,
                                          PRInt32 *aContentOffset);

  
  

  


  inline PRUint32 GetLinkCount()
  {
    return GetEmbeddedChildCount();
  }

  


  inline nsAccessible* GetLinkAt(PRUint32 aIndex)
  {
    return GetEmbeddedChildAt(aIndex);
  }

  


  inline PRInt32 GetLinkIndex(nsAccessible* aLink)
  {
    return GetIndexOfEmbeddedChild(aLink);
  }

  


  inline PRInt32 GetLinkIndexAtOffset(PRUint32 aOffset)
  {
    nsAccessible* child = GetChildAtOffset(aOffset);
    return child ? GetLinkIndex(child) : -1;
  }

  
  

  
























  nsAccessible *DOMPointToHypertextOffset(nsINode *aNode,
                                          PRInt32 aNodeOffset,
                                          PRInt32 *aHypertextOffset,
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

  
  

  


  inline PRUint32 CharacterCount()
  {
    return GetChildOffset(GetChildCount());
  }

  










  bool GetCharAt(PRInt32 aOffset, EGetTextType aShift, nsAString& aChar,
                 PRInt32* aStartOffset = nsnull, PRInt32* aEndOffset = nsnull);

  







  PRInt32 GetChildOffset(nsAccessible* aChild,
                         PRBool aInvalidateAfter = PR_FALSE)
  {
    PRInt32 index = GetIndexOf(aChild);
    return index == -1 ? -1 : GetChildOffset(index, aInvalidateAfter);
  }

  


  PRInt32 GetChildOffset(PRUint32 aChildIndex,
                         PRBool aInvalidateAfter = PR_FALSE);

  




  PRInt32 GetChildIndexAtOffset(PRUint32 aOffset);

  




  nsAccessible* GetChildAtOffset(PRUint32 aOffset)
  {
    return GetChildAt(GetChildIndexAtOffset(aOffset));
  }

protected:
  

  


  inline PRInt32 ConvertMagicOffset(PRInt32 aOffset)
  {
    if (aOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT)
      return CharacterCount();

    if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
      PRInt32 caretOffset = -1;
      GetCaretOffset(&caretOffset);
      return caretOffset;
    }

    return aOffset;
  }

  









  nsresult GetTextHelper(EGetTextType aType, nsAccessibleTextBoundary aBoundaryType,
                         PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                         nsAString & aText);

  













  PRInt32 GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame,
                            PRInt32 aFromOffset, nsAccessible *aFromAccessible,
                            nsSelectionAmount aAmount, nsDirection aDirection,
                            PRBool aNeedsStart);

  
























  nsIFrame* GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset,
                          nsAString *aText = nsnull,
                          nsIFrame **aEndFrame = nsnull,
                          nsIntRect *aBoundsRect = nsnull,
                          nsAccessible **aStartAcc = nsnull,
                          nsAccessible **aEndAcc = nsnull);

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

private:
  


  nsTArray<PRUint32> mOffsets;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHyperTextAccessible,
                              NS_HYPERTEXTACCESSIBLE_IMPL_CID)





inline nsHyperTextAccessible*
nsAccessible::AsHyperText()
{
  return mFlags & eHyperTextAccessible ?
    static_cast<nsHyperTextAccessible*>(this) : nsnull;
}

#endif  

