





#include "SVGStringList.h"
#include "nsError.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsString.h"
#include "nsWhitespaceTokenizer.h"
#include "SVGContentUtils.h"

namespace mozilla {

nsresult
SVGStringList::CopyFrom(const SVGStringList& rhs)
{
  if (!mStrings.Assign(rhs.mStrings, fallible)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mIsSet = true;
  return NS_OK;
}

void
SVGStringList::GetValue(nsAString& aValue) const
{
  aValue.Truncate();
  uint32_t last = mStrings.Length() - 1;
  for (uint32_t i = 0; i < mStrings.Length(); ++i) {
    aValue.Append(mStrings[i]);
    if (i != last) {
      if (mIsCommaSeparated) {
        aValue.Append(',');
      }
      aValue.Append(' ');
    }
  }
}

nsresult
SVGStringList::SetValue(const nsAString& aValue)
{
  SVGStringList temp;

  if (mIsCommaSeparated) {
    nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
      tokenizer(aValue, ',');

    while (tokenizer.hasMoreTokens()) {
      if (!temp.AppendItem(tokenizer.nextToken())) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    if (tokenizer.separatorAfterCurrentToken()) {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
  } else {
    nsWhitespaceTokenizerTemplate<IsSVGWhitespace> tokenizer(aValue);

    while (tokenizer.hasMoreTokens()) {
      if (!temp.AppendItem(tokenizer.nextToken())) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  return CopyFrom(temp);
}

} 
