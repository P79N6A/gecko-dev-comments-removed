



































#include "SVGPointList.h"
#include "SVGAnimatedPointList.h"
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
SVGPointList::CopyFrom(const SVGPointList& rhs)
{
  if (!SetCapacity(rhs.Length())) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mItems = rhs.mItems;
  return NS_OK;
}

void
SVGPointList::GetValueAsString(nsAString& aValue) const
{
  aValue.Truncate();
  PRUnichar buf[50];
  PRUint32 last = mItems.Length() - 1;
  for (PRUint32 i = 0; i < mItems.Length(); ++i) {
    
    
    nsTextFormatter::snprintf(buf, NS_ARRAY_LENGTH(buf),
                              NS_LITERAL_STRING("%g,%g").get(),
                              double(mItems[i].mX), double(mItems[i].mY));
    
    aValue.Append(buf);
    if (i != last) {
      aValue.Append(' ');
    }
  }
}

static inline char* SkipWhitespace(char* str)
{
  while (IsSVGWhitespace(*str))
    ++str;
  return str;
}

nsresult
SVGPointList::SetValueFromString(const nsAString& aValue)
{
  
  
  
  

  nsresult rv = NS_OK;

  SVGPointList temp;

  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',', nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);

  nsCAutoString str1, str2;  

  while (tokenizer.hasMoreTokens()) {
    CopyUTF16toUTF8(tokenizer.nextToken(), str1);
    const char *token1 = str1.get();
    if (*token1 == '\0') {
      rv = NS_ERROR_DOM_SYNTAX_ERR;
      break;
    }
    char *end;
    float x = float(PR_strtod(token1, &end));
    if (end == token1 || !NS_FloatIsFinite(x)) {
      rv = NS_ERROR_DOM_SYNTAX_ERR;
      break;
    }
    const char *token2;
    if (*end == '-') {
      
      
      token2 = end;
    } else {
      if (!tokenizer.hasMoreTokens()) {
        rv = NS_ERROR_DOM_SYNTAX_ERR;
        break;
      }
      CopyUTF16toUTF8(tokenizer.nextToken(), str2);
      token2 = str2.get();
      if (*token2 == '\0') {
        rv = NS_ERROR_DOM_SYNTAX_ERR;
        break;
      }
    }

    float y = float(PR_strtod(token2, &end));
    if (*end != '\0' || !NS_FloatIsFinite(y)) {
      rv = NS_ERROR_DOM_SYNTAX_ERR;
      break;
    }

    temp.AppendItem(SVGPoint(x, y));
  }
  if (tokenizer.lastTokenEndedWithSeparator()) {
    rv = NS_ERROR_DOM_SYNTAX_ERR; 
  }
  nsresult rv2 = CopyFrom(temp);
  if (NS_FAILED(rv2)) {
    return rv2; 
  }
  return rv;
}

} 
