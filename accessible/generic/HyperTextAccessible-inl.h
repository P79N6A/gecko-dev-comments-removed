




#ifndef mozilla_a11y_HyperTextAccessible_inl_h__
#define mozilla_a11y_HyperTextAccessible_inl_h__

#include "HyperTextAccessible.h"

#include "nsAccUtils.h"

#include "nsIClipboard.h"
#include "nsIEditor.h"
#include "nsIPersistentProperties2.h"
#include "nsIPlaintextEditor.h"
#include "nsFrameSelection.h"

namespace mozilla {
namespace a11y {

inline bool
HyperTextAccessible::IsValidOffset(int32_t aOffset)
{
  index_t offset = ConvertMagicOffset(aOffset);
  return offset.IsValid() && offset <= CharacterCount();
}

inline bool
HyperTextAccessible::IsValidRange(int32_t aStartOffset, int32_t aEndOffset)
{
  index_t startOffset = ConvertMagicOffset(aStartOffset);
  index_t endOffset = ConvertMagicOffset(aEndOffset);
  return startOffset.IsValid() && endOffset.IsValid() &&
    startOffset <= endOffset && endOffset <= CharacterCount();
}

inline void
HyperTextAccessible::SetCaretOffset(int32_t aOffset)
{
  SetSelectionRange(aOffset, aOffset);
  
  
  SelectionMgr()->UpdateCaretOffset(this, aOffset);
}

inline bool
HyperTextAccessible::AddToSelection(int32_t aStartOffset, int32_t aEndOffset)
{
  dom::Selection* domSel = DOMSelection();
  return domSel &&
    SetSelectionBoundsAt(domSel->GetRangeCount(), aStartOffset, aEndOffset);
}

inline void
HyperTextAccessible::ReplaceText(const nsAString& aText)
{
  int32_t numChars = CharacterCount();
  if (numChars != 0)
    DeleteText(0, numChars);

  InsertText(aText, 0);
}

inline void
HyperTextAccessible::InsertText(const nsAString& aText, int32_t aPosition)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
  if (peditor) {
    SetSelectionRange(aPosition, aPosition);
    peditor->InsertText(aText);
  }
}

inline void
HyperTextAccessible::CopyText(int32_t aStartPos, int32_t aEndPos)
  {
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (editor) {
      SetSelectionRange(aStartPos, aEndPos);
      editor->Copy();
    }
  }

inline void
HyperTextAccessible::CutText(int32_t aStartPos, int32_t aEndPos)
  {
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (editor) {
      SetSelectionRange(aStartPos, aEndPos);
      editor->Cut();
    }
  }

inline void
HyperTextAccessible::DeleteText(int32_t aStartPos, int32_t aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    SetSelectionRange(aStartPos, aEndPos);
    editor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
  }
}

inline void
HyperTextAccessible::PasteText(int32_t aPosition)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    SetSelectionRange(aPosition, aPosition);
    editor->Paste(nsIClipboard::kGlobalClipboard);
  }
}

inline index_t
HyperTextAccessible::ConvertMagicOffset(int32_t aOffset) const
{
  if (aOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT)
    return CharacterCount();

  if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET)
    return CaretOffset();

  return aOffset;
}

inline uint32_t
HyperTextAccessible::AdjustCaretOffset(uint32_t aOffset) const
{
  
  
  
  
  
  if (aOffset > 0 && IsCaretAtEndOfLine())
    return aOffset - 1;

  return aOffset;
}

inline bool
HyperTextAccessible::IsCaretAtEndOfLine() const
{
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  return frameSelection &&
    frameSelection->GetHint() == CARET_ASSOCIATE_BEFORE;
}

inline already_AddRefed<nsFrameSelection>
HyperTextAccessible::FrameSelection() const
{
  nsIFrame* frame = GetFrame();
  return frame ? frame->GetFrameSelection() : nullptr;
}

inline dom::Selection*
HyperTextAccessible::DOMSelection() const
{
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  return frameSelection ?
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL) :
    nullptr;
}

} 
} 

#endif

