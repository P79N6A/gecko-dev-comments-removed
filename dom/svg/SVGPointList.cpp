




#include "mozilla/ArrayUtils.h"

#include "SVGPointList.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsTextFormatter.h"
#include "SVGContentUtils.h"

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
  char16_t buf[50];
  uint32_t last = mItems.Length() - 1;
  for (uint32_t i = 0; i < mItems.Length(); ++i) {
    
    
    nsTextFormatter::snprintf(buf, ArrayLength(buf),
                              MOZ_UTF16("%g,%g"),
                              double(mItems[i].mX), double(mItems[i].mY));
    
    aValue.Append(buf);
    if (i != last) {
      aValue.Append(' ');
    }
  }
}

nsresult
SVGPointList::SetValueFromString(const nsAString& aValue)
{
  
  
  
  

  nsresult rv = NS_OK;

  SVGPointList temp;

  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',', nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);

  while (tokenizer.hasMoreTokens()) {

    const nsAString& token = tokenizer.nextToken();

    RangedPtr<const char16_t> iter =
      SVGContentUtils::GetStartRangedPtr(token);
    const RangedPtr<const char16_t> end =
      SVGContentUtils::GetEndRangedPtr(token);

    float x;
    if (!SVGContentUtils::ParseNumber(iter, end, x)) {
      rv = NS_ERROR_DOM_SYNTAX_ERR;
      break;
    }

    float y;
    if (iter == end) {
      if (!tokenizer.hasMoreTokens() ||
          !SVGContentUtils::ParseNumber(tokenizer.nextToken(), y)) {
        rv = NS_ERROR_DOM_SYNTAX_ERR;
        break;
      }
    } else {
      
      
      const nsAString& leftOver = Substring(iter.get(), end.get());
      if (leftOver[0] != '-' || !SVGContentUtils::ParseNumber(leftOver, y)) {
        rv = NS_ERROR_DOM_SYNTAX_ERR;
        break;
      }
    }
    temp.AppendItem(SVGPoint(x, y));
  }
  if (tokenizer.separatorAfterCurrentToken()) {
    rv = NS_ERROR_DOM_SYNTAX_ERR; 
  }
  nsresult rv2 = CopyFrom(temp);
  if (NS_FAILED(rv2)) {
    return rv2; 
  }
  return rv;
}

} 
