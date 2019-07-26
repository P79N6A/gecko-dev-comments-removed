




#include "mozilla/Util.h"

#include "SVGNumberList.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsError.h"
#include "nsMathUtils.h"
#include "nsString.h"
#include "nsTextFormatter.h"
#include "prdtoa.h"
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
  PRUnichar buf[24];
  uint32_t last = mNumbers.Length() - 1;
  for (uint32_t i = 0; i < mNumbers.Length(); ++i) {
    
    
    nsTextFormatter::snprintf(buf, ArrayLength(buf),
                              NS_LITERAL_STRING("%g").get(),
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

  nsAutoCString str;  

  while (tokenizer.hasMoreTokens()) {
    CopyUTF16toUTF8(tokenizer.nextToken(), str); 
    const char *token = str.get();
    if (*token == '\0') {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
    char *end;
    float num = float(PR_strtod(token, &end));
    if (*end != '\0' || !NS_finite(num)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    if (!temp.AppendItem(num)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  if (tokenizer.lastTokenEndedWithSeparator()) {
    return NS_ERROR_DOM_SYNTAX_ERR; 
  }
  return CopyFrom(temp);
}

} 
