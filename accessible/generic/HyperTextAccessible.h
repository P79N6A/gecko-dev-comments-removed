




#ifndef mozilla_a11y_HyperTextAccessible_h__
#define mozilla_a11y_HyperTextAccessible_h__

#include "AccessibleWrap.h"
#include "nsIAccessibleTypes.h"
#include "xpcAccessibleHyperText.h"
#include "nsDirection.h"
#include "WordMovementType.h"
#include "nsIFrame.h"

#include "nsISelectionController.h"

class nsFrameSelection;
class nsRange;
class nsIWidget;

namespace mozilla {

namespace dom {
class Selection;
}

namespace a11y {

class TextRange;

struct DOMPoint {
  DOMPoint() : node(nullptr), idx(0) { }
  DOMPoint(nsINode* aNode, int32_t aIdx) : node(aNode), idx(aIdx) { }

  nsINode* node;
  int32_t idx;
};



const char16_t kEmbeddedObjectChar = 0xfffc;
const char16_t kImaginaryEmbeddedObjectChar = ' ';
const char16_t kForcedNewLineChar = '\n';




class HyperTextAccessible : public AccessibleWrap,
                            public xpcAccessibleHyperText
{
public:
  HyperTextAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual int32_t GetLevelInternal();
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual mozilla::a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;

  virtual void InvalidateChildren();
  virtual bool RemoveChild(Accessible* aAccessible);

  

  
  nsresult ContentToRenderedOffset(nsIFrame *aFrame, int32_t aContentOffset,
                                   uint32_t *aRenderedOffset) const;

  
  nsresult RenderedToContentOffset(nsIFrame *aFrame, uint32_t aRenderedOffset,
                                   int32_t *aContentOffset) const;

  
  

  


  uint32_t LinkCount()
    { return EmbeddedChildCount(); }

  


  Accessible* LinkAt(uint32_t aIndex)
  {
    return GetEmbeddedChildAt(aIndex);
  }

  


  int32_t LinkIndexOf(Accessible* aLink)
  {
    return GetIndexOfEmbeddedChild(aLink);
  }

  


  int32_t LinkIndexAtOffset(uint32_t aOffset)
  {
    Accessible* child = GetChildAtOffset(aOffset);
    return child ? LinkIndexOf(child) : -1;
  }

  
  

  


















  uint32_t DOMPointToOffset(nsINode* aNode, int32_t aNodeOffset,
                            bool aIsEndOffset = false) const;

  


  uint32_t TransformOffset(Accessible* aDescendant, uint32_t aOffset,
                           bool aIsEndOffset) const;

  







  bool OffsetsToDOMRange(int32_t aStartOffset, int32_t aEndOffset,
                         nsRange* aRange);

  






  DOMPoint OffsetToDOMPoint(int32_t aOffset);

  



  bool IsTextRole();

  
  

  


  uint32_t CharacterCount() const
    { return GetChildOffset(ChildCount()); }

  


  bool CharAt(int32_t aOffset, nsAString& aChar,
              int32_t* aStartOffset = nullptr, int32_t* aEndOffset = nullptr)
  {
    NS_ASSERTION(!aStartOffset == !aEndOffset,
                 "Offsets should be both defined or both undefined!");

    int32_t childIdx = GetChildIndexAtOffset(aOffset);
    if (childIdx == -1)
      return false;

    Accessible* child = GetChildAt(childIdx);
    child->AppendTextTo(aChar, aOffset - GetChildOffset(childIdx), 1);

    if (aStartOffset && aEndOffset) {
      *aStartOffset = aOffset;
      *aEndOffset = aOffset + aChar.Length();
    }
    return true;
  }

  char16_t CharAt(int32_t aOffset)
  {
    nsAutoString charAtOffset;
    CharAt(aOffset, charAtOffset);
    return charAtOffset.CharAt(0);
  }

  


  bool IsCharAt(int32_t aOffset, char16_t aChar)
    { return CharAt(aOffset) == aChar; }

  


  bool IsLineEndCharAt(int32_t aOffset)
    { return IsCharAt(aOffset, '\n'); }

  


  void TextSubstring(int32_t aStartOffset, int32_t aEndOffset, nsAString& aText);

  



  void TextBeforeOffset(int32_t aOffset, AccessibleTextBoundary aBoundaryType,
                       int32_t* aStartOffset, int32_t* aEndOffset,
                       nsAString& aText);
  void TextAtOffset(int32_t aOffset, AccessibleTextBoundary aBoundaryType,
                    int32_t* aStartOffset, int32_t* aEndOffset,
                    nsAString& aText);
  void TextAfterOffset(int32_t aOffset, AccessibleTextBoundary aBoundaryType,
                       int32_t* aStartOffset, int32_t* aEndOffset,
                       nsAString& aText);

  


  already_AddRefed<nsIPersistentProperties>
    TextAttributes(bool aIncludeDefAttrs, int32_t aOffset,
                   int32_t* aStartOffset, int32_t* aEndOffset);

  


  already_AddRefed<nsIPersistentProperties> DefaultTextAttributes();

  







  int32_t GetChildOffset(Accessible* aChild,
                         bool aInvalidateAfter = false) const
  {
    int32_t index = GetIndexOf(aChild);
    return index == -1 ? -1 : GetChildOffset(index, aInvalidateAfter);
  }

  


  int32_t GetChildOffset(uint32_t aChildIndex,
                         bool aInvalidateAfter = false) const;

  




  int32_t GetChildIndexAtOffset(uint32_t aOffset) const;

  




  Accessible* GetChildAtOffset(uint32_t aOffset) const
  {
    return GetChildAt(GetChildIndexAtOffset(aOffset));
  }

  


  bool IsValidOffset(int32_t aOffset);
  bool IsValidRange(int32_t aStartOffset, int32_t aEndOffset);

  


  int32_t OffsetAtPoint(int32_t aX, int32_t aY, uint32_t aCoordType);

  


  nsIntRect TextBounds(int32_t aStartOffset, int32_t aEndOffset,
                       uint32_t aCoordType = nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE);

  



  nsIntRect CharBounds(int32_t aOffset, uint32_t aCoordType)
  {
    int32_t endOffset = aOffset == static_cast<int32_t>(CharacterCount()) ?
      aOffset : aOffset + 1;
    return TextBounds(aOffset, endOffset, aCoordType);
  }

  


  int32_t CaretOffset() const;
  void SetCaretOffset(int32_t aOffset);

  



  int32_t CaretLineNumber();

  






  nsIntRect GetCaretRect(nsIWidget** aWidget);

  


  int32_t SelectionCount();

  


  bool SelectionBoundsAt(int32_t aSelectionNum,
                         int32_t* aStartOffset, int32_t* aEndOffset);

  



  bool SetSelectionBoundsAt(int32_t aSelectionNum,
                            int32_t aStartOffset, int32_t aEndOffset);

  



  bool AddToSelection(int32_t aStartOffset, int32_t aEndOffset);

  



  bool RemoveFromSelection(int32_t aSelectionNum);

  


  void ScrollSubstringTo(int32_t aStartOffset, int32_t aEndOffset,
                         uint32_t aScrollType);

  


  void ScrollSubstringToPoint(int32_t aStartOffset,
                              int32_t aEndOffset,
                              uint32_t aCoordinateType,
                              int32_t aX, int32_t aY);

  



  void EnclosingRange(TextRange& aRange) const;

  



  void SelectionRanges(nsTArray<TextRange>* aRanges) const;

  



  void VisibleRanges(nsTArray<TextRange>* aRanges) const;

  


  void RangeByChild(Accessible* aChild, TextRange& aRange) const;

  


  void RangeAtPoint(int32_t aX, int32_t aY, TextRange& aRange) const;

  
  

  void ReplaceText(const nsAString& aText);
  void InsertText(const nsAString& aText, int32_t aPosition);
  void CopyText(int32_t aStartPos, int32_t aEndPos);
  void CutText(int32_t aStartPos, int32_t aEndPos);
  void DeleteText(int32_t aStartPos, int32_t aEndPos);
  void PasteText(int32_t aPosition);

  


  virtual already_AddRefed<nsIEditor> GetEditor() const;

  


  dom::Selection* DOMSelection() const;

protected:
  virtual ~HyperTextAccessible() { }

  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
  virtual void CacheChildren() MOZ_OVERRIDE;

  

  


  index_t ConvertMagicOffset(int32_t aOffset) const;

  


  uint32_t AdjustCaretOffset(uint32_t aOffset) const;

  


  bool IsCaretAtEndOfLine() const;

  


  bool IsEmptyLastLineOffset(int32_t aOffset)
  {
    return aOffset == static_cast<int32_t>(CharacterCount()) &&
      IsLineEndCharAt(aOffset - 1);
  }

  


  uint32_t FindWordBoundary(uint32_t aOffset, nsDirection aDirection,
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

  


  uint32_t FindLineBoundary(uint32_t aOffset,
                            EWhichLineBoundary aWhichLineBoundary);

  



  uint32_t FindOffset(uint32_t aOffset, nsDirection aDirection,
                      nsSelectionAmount aAmount,
                      EWordMovementType aWordMovementType = eDefaultBehavior);

  



  nsIntRect GetBoundsInFrame(nsIFrame* aFrame,
                             uint32_t aStartRenderedOffset,
                             uint32_t aEndRenderedOffset);

  

  


  already_AddRefed<nsFrameSelection> FrameSelection() const;

  


  void GetSelectionDOMRanges(int16_t aType, nsTArray<nsRange*>* aRanges);

  nsresult SetSelectionRange(int32_t aStartPos, int32_t aEndPos);

  
  nsresult GetDOMPointByFrameOffset(nsIFrame* aFrame, int32_t aOffset,
                                    Accessible* aAccessible,
                                    mozilla::a11y::DOMPoint* aPoint);

  












  void GetSpellTextAttr(nsINode* aNode, int32_t aNodeOffset,
                        uint32_t* aStartOffset, uint32_t* aEndOffset,
                        nsIPersistentProperties* aAttributes);

private:
  


  mutable nsTArray<uint32_t> mOffsets;
};





inline HyperTextAccessible*
Accessible::AsHyperText()
{
  return IsHyperText() ? static_cast<HyperTextAccessible*>(this) : nullptr;
}

} 
} 

#endif

