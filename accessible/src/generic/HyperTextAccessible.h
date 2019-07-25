




#ifndef mozilla_a11y_HyperTextAccessible_h__
#define mozilla_a11y_HyperTextAccessible_h__

#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"

#include "AccCollector.h"
#include "AccessibleWrap.h"

#include "nsFrameSelection.h"
#include "nsISelectionController.h"

namespace mozilla {
namespace a11y {
struct DOMPoint {
  nsINode* node;
  PRInt32 idx;
};
}
}

enum EGetTextType { eGetBefore=-1, eGetAt=0, eGetAfter=1 };



const PRUnichar kEmbeddedObjectChar = 0xfffc;
const PRUnichar kImaginaryEmbeddedObjectChar = ' ';
const PRUnichar kForcedNewLineChar = '\n';




class HyperTextAccessible : public AccessibleWrap,
                            public nsIAccessibleText,
                            public nsIAccessibleHyperText,
                            public nsIAccessibleEditableText
{
public:
  HyperTextAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HyperTextAccessible() { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXT
  NS_DECL_NSIACCESSIBLEHYPERTEXT
  NS_DECL_NSIACCESSIBLEEDITABLETEXT

  
  virtual PRInt32 GetLevelInternal();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  virtual void InvalidateChildren();
  virtual bool RemoveChild(Accessible* aAccessible);

  

  
  static nsresult ContentToRenderedOffset(nsIFrame *aFrame, PRInt32 aContentOffset,
                                          PRUint32 *aRenderedOffset);
  
  
  static nsresult RenderedToContentOffset(nsIFrame *aFrame, PRUint32 aRenderedOffset,
                                          PRInt32 *aContentOffset);

  
  

  


  PRUint32 GetLinkCount()
  {
    return EmbeddedChildCount();
  }

  


  Accessible* GetLinkAt(PRUint32 aIndex)
  {
    return GetEmbeddedChildAt(aIndex);
  }

  


  PRInt32 GetLinkIndex(Accessible* aLink)
  {
    return GetIndexOfEmbeddedChild(aLink);
  }

  


  PRInt32 GetLinkIndexAtOffset(PRUint32 aOffset)
  {
    Accessible* child = GetChildAtOffset(aOffset);
    return child ? GetLinkIndex(child) : -1;
  }

  
  

  
























  Accessible* DOMPointToHypertextOffset(nsINode *aNode,
                                        PRInt32 aNodeOffset,
                                        PRInt32* aHypertextOffset,
                                        bool aIsEndOffset = false);

  






  nsresult HypertextOffsetsToDOMRange(PRInt32 aStartHTOffset,
                                      PRInt32 aEndHTOffset,
                                      nsRange* aRange);

  



  bool IsTextRole();

  
  

  


  PRUint32 CharacterCount()
  {
    return GetChildOffset(ChildCount());
  }

  










  bool GetCharAt(PRInt32 aOffset, EGetTextType aShift, nsAString& aChar,
                 PRInt32* aStartOffset = nsnull, PRInt32* aEndOffset = nsnull);

  







  PRInt32 GetChildOffset(Accessible* aChild,
                         bool aInvalidateAfter = false)
  {
    PRInt32 index = GetIndexOf(aChild);
    return index == -1 ? -1 : GetChildOffset(index, aInvalidateAfter);
  }

  


  PRInt32 GetChildOffset(PRUint32 aChildIndex,
                         bool aInvalidateAfter = false);

  




  PRInt32 GetChildIndexAtOffset(PRUint32 aOffset);

  




  Accessible* GetChildAtOffset(PRUint32 aOffset)
  {
    return GetChildAt(GetChildIndexAtOffset(aOffset));
  }

  


  nsIntRect GetTextBounds(PRInt32 aStartOffset, PRInt32 aEndOffset)
  {
    nsIntRect bounds;
    GetPosAndText(aStartOffset, aEndOffset, nsnull, nsnull, &bounds);
    return bounds;
  }

  



  PRInt32 CaretLineNumber();

  
  

  


  virtual already_AddRefed<nsIEditor> GetEditor() const;

protected:
  

  


  PRInt32 ConvertMagicOffset(PRInt32 aOffset)
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

  









  nsresult GetTextHelper(EGetTextType aType, AccessibleTextBoundary aBoundaryType,
                         PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                         nsAString & aText);

  













  PRInt32 GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame,
                            PRInt32 aFromOffset, Accessible* aFromAccessible,
                            nsSelectionAmount aAmount, nsDirection aDirection,
                            bool aNeedsStart);

  
























  nsIFrame* GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset,
                          nsAString *aText = nsnull,
                          nsIFrame **aEndFrame = nsnull,
                          nsIntRect *aBoundsRect = nsnull,
                          Accessible** aStartAcc = nsnull,
                          Accessible** aEndAcc = nsnull);

  nsIntRect GetBoundsForString(nsIFrame *aFrame, PRUint32 aStartRenderedOffset, PRUint32 aEndRenderedOffset);

  

  


  virtual already_AddRefed<nsFrameSelection> FrameSelection();

  


  void GetSelectionDOMRanges(PRInt16 aType, nsTArray<nsRange*>* aRanges);

  nsresult SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos);

  
  nsresult GetDOMPointByFrameOffset(nsIFrame* aFrame, PRInt32 aOffset,
                                    Accessible* aAccessible,
                                    mozilla::a11y::DOMPoint* aPoint);

  
  












  nsresult RangeBoundToHypertextOffset(nsRange *aRange,
                                       bool aIsStartBound,
                                       bool aIsStartOffset,
                                       PRInt32 *aHTOffset);

  












  nsresult GetSpellTextAttribute(nsINode* aNode, PRInt32 aNodeOffset,
                                 PRInt32 *aStartOffset,
                                 PRInt32 *aEndOffset,
                                 nsIPersistentProperties *aAttributes);

private:
  


  nsTArray<PRUint32> mOffsets;
};





inline HyperTextAccessible*
Accessible::AsHyperText()
{
  return mFlags & eHyperTextAccessible ?
    static_cast<HyperTextAccessible*>(this) : nsnull;
}

#endif

