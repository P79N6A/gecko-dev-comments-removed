





































#include "TextUpdater.h"

#include "nsDocAccessible.h"
#include "nsTextAccessible.h"

void
TextUpdater::Run(nsDocAccessible* aDocument, nsTextAccessible* aTextLeaf,
                 const nsAString& aNewText)
{
  NS_ASSERTION(aTextLeaf, "No text leaf accessible?");

  const nsString& oldText = aTextLeaf->Text();
  PRUint32 oldLen = oldText.Length(), newLen = aNewText.Length();
  PRUint32 minLen = NS_MIN(oldLen, newLen);

  
  PRUint32 skipStart = 0;
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
                      PRUint32 aSkipStart)
{
  nsAccessible* parent = mTextLeaf->Parent();
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

  PRUint32 oldLen = aOldText.Length(), newLen = aNewText.Length();
  PRUint32 minLen = NS_MIN(oldLen, newLen);

  
  PRUint32 skipEnd = 0;
  while (minLen - skipEnd > aSkipStart &&
         aNewText[newLen - skipEnd - 1] == aOldText[oldLen - skipEnd - 1]) {
    skipEnd++;
  }

  PRInt32 strLen1 = oldLen - aSkipStart - skipEnd;
  PRInt32 strLen2 = newLen - aSkipStart - skipEnd;

  const nsAString& str1 = Substring(aOldText, aSkipStart, strLen1);
  const nsAString& str2 = Substring(aNewText, aSkipStart, strLen2);

  
  mTextOffset += aSkipStart;

  
  
  
  if (strLen1 == 0 || strLen2 == 0 ||
      strLen1 > kMaxStrLen || strLen2 > kMaxStrLen) {
    if (strLen1 > 0) {
      
      nsRefPtr<AccEvent> textRemoveEvent =
        new AccTextChangeEvent(mHyperText, mTextOffset, str1, false);
      mDocument->FireDelayedAccessibleEvent(textRemoveEvent);
    }

    if (strLen2 > 0) {
      
      nsRefPtr<AccEvent> textInsertEvent =
        new AccTextChangeEvent(mHyperText, mTextOffset, str2, true);
      mDocument->FireDelayedAccessibleEvent(textInsertEvent);
    }

    mDocument->MaybeNotifyOfValueChange(mHyperText);

    
    mTextLeaf->SetText(aNewText);
    return;
  }

  
  
  

  
  PRUint32 len1 = strLen1 + 1, len2 = strLen2 + 1;
  PRUint32* entries = new PRUint32[len1 * len2];

  for (PRUint32 colIdx = 0; colIdx < len1; colIdx++)
    entries[colIdx] = colIdx;

  PRUint32* row = entries;
  for (PRUint32 rowIdx = 1; rowIdx < len2; rowIdx++) {
    PRUint32* prevRow = row;
    row += len1;
    row[0] = rowIdx;
    for (PRUint32 colIdx = 1; colIdx < len1; colIdx++) {
      if (str1[colIdx - 1] != str2[rowIdx - 1]) {
        PRUint32 left = row[colIdx - 1];
        PRUint32 up = prevRow[colIdx];
        PRUint32 upleft = prevRow[colIdx - 1];
        row[colIdx] = NS_MIN(upleft, NS_MIN(left, up)) + 1;
      } else {
        row[colIdx] = prevRow[colIdx - 1];
      }
    }
  }

  
  nsTArray<nsRefPtr<AccEvent> > events;
  ComputeTextChangeEvents(str1, str2, entries, events);

  delete [] entries;

  
  for (PRInt32 idx = events.Length() - 1; idx >= 0; idx--)
    mDocument->FireDelayedAccessibleEvent(events[idx]);

  mDocument->MaybeNotifyOfValueChange(mHyperText);

  
  mTextLeaf->SetText(aNewText);
}

void
TextUpdater::ComputeTextChangeEvents(const nsAString& aStr1,
                                     const nsAString& aStr2,
                                     PRUint32* aEntries,
                                     nsTArray<nsRefPtr<AccEvent> >& aEvents)
{
  PRInt32 colIdx = aStr1.Length(), rowIdx = aStr2.Length();

  
  PRInt32 colEnd = colIdx;
  PRInt32 rowEnd = rowIdx;

  PRInt32 colLen = colEnd + 1;
  PRUint32* row = aEntries + rowIdx * colLen;
  PRInt32 dist = row[colIdx]; 
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
