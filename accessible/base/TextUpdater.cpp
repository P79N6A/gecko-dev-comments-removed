




#include "TextUpdater.h"

#include "Accessible-inl.h"
#include "DocAccessible-inl.h"
#include "TextLeafAccessible.h"
#include <algorithm>

using namespace mozilla::a11y;

void
TextUpdater::Run(DocAccessible* aDocument, TextLeafAccessible* aTextLeaf,
                 const nsAString& aNewText)
{
  NS_ASSERTION(aTextLeaf, "No text leaf accessible?");

  const nsString& oldText = aTextLeaf->Text();
  uint32_t oldLen = oldText.Length(), newLen = aNewText.Length();
  uint32_t minLen = std::min(oldLen, newLen);

  
  uint32_t skipStart = 0;
  for (; skipStart < minLen; skipStart++) {
    if (aNewText[skipStart] != oldText[skipStart])
      break;
  }

  
  if (skipStart != minLen || oldLen != newLen) {
    TextUpdater updater(aDocument, aTextLeaf);
    updater.DoUpdate(aNewText, oldText, skipStart);
  }
}

void
TextUpdater::DoUpdate(const nsAString& aNewText, const nsAString& aOldText,
                      uint32_t aSkipStart)
{
  Accessible* parent = mTextLeaf->Parent();
  if (!parent)
    return;

  mHyperText = parent->AsHyperText();
  if (!mHyperText) {
    NS_ERROR("Text leaf parent is not hypertext!");
    return;
  }

  
  mTextOffset = mHyperText->GetChildOffset(mTextLeaf, true);
  NS_ASSERTION(mTextOffset != -1,
               "Text leaf hasn't offset within hyper text!");

  uint32_t oldLen = aOldText.Length(), newLen = aNewText.Length();
  uint32_t minLen = std::min(oldLen, newLen);

  
  uint32_t skipEnd = 0;
  while (minLen - skipEnd > aSkipStart &&
         aNewText[newLen - skipEnd - 1] == aOldText[oldLen - skipEnd - 1]) {
    skipEnd++;
  }

  uint32_t strLen1 = oldLen - aSkipStart - skipEnd;
  uint32_t strLen2 = newLen - aSkipStart - skipEnd;

  const nsAString& str1 = Substring(aOldText, aSkipStart, strLen1);
  const nsAString& str2 = Substring(aNewText, aSkipStart, strLen2);

  
  mTextOffset += aSkipStart;

  
  
  
  if (strLen1 == 0 || strLen2 == 0 ||
      strLen1 > kMaxStrLen || strLen2 > kMaxStrLen) {
    if (strLen1 > 0) {
      
      nsRefPtr<AccEvent> textRemoveEvent =
        new AccTextChangeEvent(mHyperText, mTextOffset, str1, false);
      mDocument->FireDelayedEvent(textRemoveEvent);
    }

    if (strLen2 > 0) {
      
      nsRefPtr<AccEvent> textInsertEvent =
        new AccTextChangeEvent(mHyperText, mTextOffset, str2, true);
      mDocument->FireDelayedEvent(textInsertEvent);
    }

    mDocument->MaybeNotifyOfValueChange(mHyperText);

    
    mTextLeaf->SetText(aNewText);
    return;
  }

  
  
  

  
  uint32_t len1 = strLen1 + 1, len2 = strLen2 + 1;
  uint32_t* entries = new uint32_t[len1 * len2];

  for (uint32_t colIdx = 0; colIdx < len1; colIdx++)
    entries[colIdx] = colIdx;

  uint32_t* row = entries;
  for (uint32_t rowIdx = 1; rowIdx < len2; rowIdx++) {
    uint32_t* prevRow = row;
    row += len1;
    row[0] = rowIdx;
    for (uint32_t colIdx = 1; colIdx < len1; colIdx++) {
      if (str1[colIdx - 1] != str2[rowIdx - 1]) {
        uint32_t left = row[colIdx - 1];
        uint32_t up = prevRow[colIdx];
        uint32_t upleft = prevRow[colIdx - 1];
        row[colIdx] = std::min(upleft, std::min(left, up)) + 1;
      } else {
        row[colIdx] = prevRow[colIdx - 1];
      }
    }
  }

  
  nsTArray<nsRefPtr<AccEvent> > events;
  ComputeTextChangeEvents(str1, str2, entries, events);

  delete [] entries;

  
  for (int32_t idx = events.Length() - 1; idx >= 0; idx--)
    mDocument->FireDelayedEvent(events[idx]);

  mDocument->MaybeNotifyOfValueChange(mHyperText);

  
  mTextLeaf->SetText(aNewText);
}

void
TextUpdater::ComputeTextChangeEvents(const nsAString& aStr1,
                                     const nsAString& aStr2,
                                     uint32_t* aEntries,
                                     nsTArray<nsRefPtr<AccEvent> >& aEvents)
{
  int32_t colIdx = aStr1.Length(), rowIdx = aStr2.Length();

  
  int32_t colEnd = colIdx;
  int32_t rowEnd = rowIdx;

  int32_t colLen = colEnd + 1;
  uint32_t* row = aEntries + rowIdx * colLen;
  uint32_t dist = row[colIdx]; 
  while (rowIdx && colIdx) { 
    if (aStr1[colIdx - 1] == aStr2[rowIdx - 1]) { 
      if (rowIdx < rowEnd) { 
        FireInsertEvent(Substring(aStr2, rowIdx, rowEnd - rowIdx),
                        rowIdx, aEvents);
      }
      if (colIdx < colEnd) { 
        FireDeleteEvent(Substring(aStr1, colIdx, colEnd - colIdx),
                        rowIdx, aEvents);
      }

      colEnd = --colIdx; 
      rowEnd = --rowIdx;
      row -= colLen;
      continue;
    }
    --dist;
    if (dist == row[colIdx - 1 - colLen]) { 
      --colIdx;
      --rowIdx;
      row -= colLen;
      continue;
    }
    if (dist == row[colIdx - colLen]) { 
      --rowIdx;
      row -= colLen;
      continue;
    }
    if (dist == row[colIdx - 1]) { 
      --colIdx;
      continue;
    }
    NS_NOTREACHED("huh?");
    return;
  }

  if (rowEnd)
    FireInsertEvent(Substring(aStr2, 0, rowEnd), 0, aEvents);
  if (colEnd)
    FireDeleteEvent(Substring(aStr1, 0, colEnd), 0, aEvents);
}
