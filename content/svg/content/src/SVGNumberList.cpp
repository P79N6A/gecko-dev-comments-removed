



































#include "SVGNumberList.h"
#include "SVGAnimatedNumberList.h"
#include "nsSVGElement.h"
#include "nsISVGValueUtils.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsString.h"
#include "nsSVGUtils.h"
#include "string.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsCharSeparatedTokenizer.h"

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
  PRUint32 last = mNumbers.Length() - 1;
  for (PRUint32 i = 0; i < mNumbers.Length(); ++i) {
    
    
    nsTextFormatter::snprintf(buf, NS_ARRAY_LENGTH(buf),
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

  nsCAutoString str;  

  while (tokenizer.hasMoreTokens()) {
    CopyUTF16toUTF8(tokenizer.nextToken(), str); 
    const char *token = str.get();
    if (token == '\0') {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
    char *end;
    float num = float(PR_strtod(token, &end));
    if (*end != '\0' || !NS_FloatIsFinite(num)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    temp.AppendItem(num);
  }
  if (tokenizer.lastTokenEndedWithSeparator()) {
    return NS_ERROR_DOM_SYNTAX_ERR; 
  }
  return CopyFrom(temp);
}

} 
