




#ifndef mozilla_a11y_HyperTextAccessible_h__
#define mozilla_a11y_HyperTextAccessible_h__

#include "nsIAccessibleText.h"
#include "nsIAccessibleHyperText.h"
#include "nsIAccessibleEditableText.h"

#include "AccessibleWrap.h"

#include "nsFrameSelection.h"
#include "nsISelectionController.h"

namespace mozilla {
namespace a11y {

struct DOMPoint {
  nsINode* node;
  int32_t idx;
};

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

  
  virtual int32_t GetLevelInternal();
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();

  virtual void InvalidateChildren();
  virtual bool RemoveChild(Accessible* aAccessible);

  

  
  nsresult ContentToRenderedOffset(nsIFrame *aFrame, int32_t aContentOffset,
                                   uint32_t *aRenderedOffset);
  
  
  nsresult RenderedToContentOffset(nsIFrame *aFrame, uint32_t aRenderedOffset,
                                   int32_t *aContentOffset);

  
  

  


  uint32_t GetLinkCount()
  {
    return EmbeddedChildCount();
  }

  


  Accessible* GetLinkAt(uint32_t aIndex)
  {
    return GetEmbeddedChildAt(aIndex);
  }

  


  int32_t GetLinkIndex(Accessible* aLink)
  {
    return GetIndexOfEmbeddedChild(aLink);
  }

  


  int32_t GetLinkIndexAtOffset(uint32_t aOffset)
  {
    Accessible* child = GetChildAtOffset(aOffset);
    return child ? GetLinkIndex(child) : -1;
  }

  
  

  
























  Accessible* DOMPointToHypertextOffset(nsINode *aNode,
                                        int32_t aNodeOffset,
                                        int32_t* aHypertextOffset,
                                        bool aIsEndOffset = false);

  






  nsresult HypertextOffsetsToDOMRange(int32_t aStartHTOffset,
                                      int32_t aEndHTOffset,
                                      nsRange* aRange);

  



  bool IsTextRole();

  
  

  


  uint32_t CharacterCount()
  {
    return GetChildOffset(ChildCount());
  }

  


  bool CharAt(int32_t aOffset, nsAString& aChar)
  {
    int32_t childIdx = GetChildIndexAtOffset(aOffset);
    if (childIdx == -1)
      return false;

    Accessible* child = GetChildAt(childIdx);
    child->AppendTextTo(aChar, aOffset - GetChildOffset(childIdx), 1);
    return true;
  }

  


  bool IsCharAt(int32_t aOffset, char aChar)
  {
    nsAutoString charAtOffset;
    CharAt(aOffset, charAtOffset);
    return charAtOffset.CharAt(0) == aChar;
  }

  


  bool IsLineEndCharAt(int32_t aOffset)
    { return IsCharAt(aOffset, '\n'); }

  










  bool GetCharAt(int32_t aOffset, EGetTextType aShift, nsAString& aChar,
                 int32_t* aStartOffset = nullptr, int32_t* aEndOffset = nullptr);

  







  int32_t GetChildOffset(Accessible* aChild,
                         bool aInvalidateAfter = false)
  {
    int32_t index = GetIndexOf(aChild);
    return index == -1 ? -1 : GetChildOffset(index, aInvalidateAfter);
  }

  


  int32_t GetChildOffset(uint32_t aChildIndex,
                         bool aInvalidateAfter = false);

  




  int32_t GetChildIndexAtOffset(uint32_t aOffset);

  




  Accessible* GetChildAtOffset(uint32_t aOffset)
  {
    return GetChildAt(GetChildIndexAtOffset(aOffset));
  }

  


  nsIntRect GetTextBounds(int32_t aStartOffset, int32_t aEndOffset)
  {
    nsIntRect bounds;
    GetPosAndText(aStartOffset, aEndOffset, nullptr, nullptr, &bounds);
    return bounds;
  }

  



  int32_t CaretLineNumber();

  






  nsIntRect GetCaretRect(nsIWidget** aWidget);

  
  

  


  virtual already_AddRefed<nsIEditor> GetEditor() const;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
  virtual void CacheChildren() MOZ_OVERRIDE;

  

  


  int32_t ConvertMagicOffset(int32_t aOffset)
  {
    if (aOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT)
      return CharacterCount();

    if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
      int32_t caretOffset = -1;
      GetCaretOffset(&caretOffset);
      return caretOffset;
    }

    return aOffset;
  }

  


  int32_t AdjustCaretOffset(int32_t aOffset)
  {
    
    
    
    
    
    if (aOffset > 0) {
      nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
      if (frameSelection &&
          frameSelection->GetHint() == nsFrameSelection::HINTLEFT) {
        return aOffset - 1;
      }
    }
    return aOffset;
  }

  


  bool IsEmptyLastLineOffset(int32_t aOffset)
  {
    return aOffset == static_cast<int32_t>(CharacterCount()) &&
      IsLineEndCharAt(aOffset - 1);
  }

  


  int32_t FindWordBoundary(int32_t aOffset, nsDirection aDirection,
                           EWordMovementType aWordMovementType)
  {
    return FindOffset(aOffset, aDirection, eSelectWord, aWordMovementType);
  }

  





  enum EWhichLineBoundary {
    ePrevLineBegin,
    ePrevLineEnd,
    eThisLineBegin,
    eThisLineEnd,
    eNextLineBegin,
    eNextLineEnd
  };

  


  int32_t FindLineBoundary(int32_t aOffset,
                           EWhichLineBoundary aWhichLineBoundary);

  



  int32_t FindOffset(int32_t aOffset, nsDirection aDirection,
                     nsSelectionAmount aAmount,
                     EWordMovementType aWordMovementType = eDefaultBehavior);

  













  int32_t GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame,
                            int32_t aFromOffset, Accessible* aFromAccessible,
                            nsSelectionAmount aAmount, nsDirection aDirection,
                            bool aNeedsStart,
                            EWordMovementType aWordMovementType);

  
























  nsIFrame* GetPosAndText(int32_t& aStartOffset, int32_t& aEndOffset,
                          nsAString *aText = nullptr,
                          nsIFrame **aEndFrame = nullptr,
                          nsIntRect *aBoundsRect = nullptr,
                          Accessible** aStartAcc = nullptr,
                          Accessible** aEndAcc = nullptr);

  nsIntRect GetBoundsForString(nsIFrame *aFrame, uint32_t aStartRenderedOffset, uint32_t aEndRenderedOffset);

  

  


  virtual already_AddRefed<nsFrameSelection> FrameSelection();

  


  void GetSelectionDOMRanges(int16_t aType, nsTArray<nsRange*>* aRanges);

  nsresult SetSelectionRange(int32_t aStartPos, int32_t aEndPos);

  
  nsresult GetDOMPointByFrameOffset(nsIFrame* aFrame, int32_t aOffset,
                                    Accessible* aAccessible,
                                    mozilla::a11y::DOMPoint* aPoint);

  
  












  nsresult RangeBoundToHypertextOffset(nsRange *aRange,
                                       bool aIsStartBound,
                                       bool aIsStartOffset,
                                       int32_t *aHTOffset);

  












  nsresult GetSpellTextAttribute(nsINode* aNode, int32_t aNodeOffset,
                                 int32_t *aStartOffset,
                                 int32_t *aEndOffset,
                                 nsIPersistentProperties *aAttributes);

private:
  


  nsTArray<uint32_t> mOffsets;
};





inline HyperTextAccessible*
Accessible::AsHyperText()
{
  return IsHyperText() ? static_cast<HyperTextAccessible*>(this) : nullptr;
}

} 
} 

#endif

