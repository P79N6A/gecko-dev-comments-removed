




#include "SVGLengthList.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsError.h"
#include "nsString.h"
#include "nsSVGElement.h"
#include "SVGAnimatedLengthList.h"
#include "SVGContentUtils.h"
#include "SVGLength.h"

namespace mozilla {

nsresult
SVGLengthList::CopyFrom(const SVGLengthList& rhs)
{
  if (!mLengths.SetCapacity(rhs.Length())) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mLengths = rhs.mLengths;
  return NS_OK;
}

void
SVGLengthList::GetValueAsString(nsAString& aValue) const
{
  aValue.Truncate();
  uint32_t last = mLengths.Length() - 1;
  for (uint32_t i = 0; i < mLengths.Length(); ++i) {
    nsAutoString length;
    mLengths[i].GetValueAsString(length);
    
    aValue.Append(length);
    if (i != last) {
      aValue.Append(' ');
    }
  }
}

nsresult
SVGLengthList::SetValueFromString(const nsAString& aValue)
{
  SVGLengthList temp;

  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',', nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);

  while (tokenizer.hasMoreTokens()) {
    SVGLength length;
    if (!length.SetValueFromString(tokenizer.nextToken())) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    if (!temp.AppendItem(length)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  if (tokenizer.separatorAfterCurrentToken()) {
    return NS_ERROR_DOM_SYNTAX_ERR; 
  }
  return CopyFrom(temp);
}

bool
SVGLengthList::operator==(const SVGLengthList& rhs) const
{
  if (Length() != rhs.Length()) {
    return false;
  }
  for (uint32_t i = 0; i < Length(); ++i) {
    if (!(mLengths[i] == rhs.mLengths[i])) {
      return false;
    }
  }
  return true;
}

} 
