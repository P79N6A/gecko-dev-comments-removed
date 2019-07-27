




#include "mozilla/ArrayUtils.h"

#include "SVGNumberList.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsString.h"
#include "nsTextFormatter.h"
#include "SVGContentUtils.h"

namespace mozilla {

nsresult
SVGNumberList::CopyFrom(const SVGNumberList& rhs)
{
  if (!mNumbers.SetCapacity(rhs.Length())) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mNumbers = rhs.mNumbers;
  return NS_OK;
}

void
SVGNumberList::GetValueAsString(nsAString& aValue) const
{
  aValue.Truncate();
  char16_t buf[24];
  uint32_t last = mNumbers.Length() - 1;
  for (uint32_t i = 0; i < mNumbers.Length(); ++i) {
    
    
    nsTextFormatter::snprintf(buf, ArrayLength(buf),
                              MOZ_UTF16("%g"),
                              double(mNumbers[i]));
    
    aValue.Append(buf);
    if (i != last) {
      aValue.Append(' ');
    }
  }
}

nsresult
SVGNumberList::SetValueFromString(const nsAString& aValue)
{
  SVGNumberList temp;

  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',', nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);

  while (tokenizer.hasMoreTokens()) {
    float num;
    if (!SVGContentUtils::ParseNumber(tokenizer.nextToken(), num)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    if (!temp.AppendItem(num)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  if (tokenizer.separatorAfterCurrentToken()) {
    return NS_ERROR_DOM_SYNTAX_ERR; 
  }
  return CopyFrom(temp);
}

} 
