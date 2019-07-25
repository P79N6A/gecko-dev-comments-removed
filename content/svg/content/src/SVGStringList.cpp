




































#include "mozilla/Util.h"

#include "SVGStringList.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"
#include "nsString.h"
#include "nsSVGUtils.h"
#include "nsTextFormatter.h"
#include "nsWhitespaceTokenizer.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsMathUtils.h"

namespace mozilla {

nsresult
SVGStringList::CopyFrom(const SVGStringList& rhs)
{
  if (!mStrings.SetCapacity(rhs.Length())) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mStrings = rhs.mStrings;
  mIsSet = true;
  return NS_OK;
}

void
SVGStringList::GetValue(nsAString& aValue, bool aIsCommaSeparated) const
{
  aValue.Truncate();
  PRUint32 last = mStrings.Length() - 1;
  for (PRUint32 i = 0; i < mStrings.Length(); ++i) {
    aValue.Append(mStrings[i]);
    if (i != last) {
      if (aIsCommaSeparated) {
        aValue.Append(',');
      }
      aValue.Append(' ');
    }
  }
}

nsresult
SVGStringList::SetValue(const nsAString& aValue, bool aIsCommaSeparated)
{
  SVGStringList temp;

  if (aIsCommaSeparated) {
    nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
      tokenizer(aValue, ',');

    while (tokenizer.hasMoreTokens()) {
      if (!temp.AppendItem(tokenizer.nextToken())) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    if (tokenizer.lastTokenEndedWithSeparator()) {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
  } else {
    nsWhitespaceTokenizer tokenizer(aValue);

    while (tokenizer.hasMoreTokens()) {
      if (!temp.AppendItem(tokenizer.nextToken())) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  return CopyFrom(temp);
}

} 
