



































#include "SVGLengthList.h"
#include "SVGAnimatedLengthList.h"
#include "SVGLength.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsString.h"
#include "nsSVGUtils.h"
#include "string.h"

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
  PRUint32 last = mLengths.Length() - 1;
  for (PRUint32 i = 0; i < mLengths.Length(); ++i) {
    nsAutoString length;
    mLengths[i].GetValueAsString(length);
    
    aValue.Append(length);
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
SVGLengthList::SetValueFromString(const nsAString& aValue)
{
  SVGLengthList temp;

  NS_ConvertUTF16toUTF8 value(aValue);
  char* start = SkipWhitespace(value.BeginWriting());

  
  
  

  while (*start != '\0') {
    int end = strcspn(start, SVG_COMMA_WSP_DELIM);
    if (end == 0) {
      
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    SVGLength length;
    if (!length.SetValueFromString(NS_ConvertUTF8toUTF16(start, PRUint32(end)))) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    if (!temp.AppendItem(length)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    start = SkipWhitespace(start + end);
    if (*start == ',') {
      start = SkipWhitespace(start + 1);
    }
  }

  return CopyFrom(temp);
}

bool
SVGLengthList::operator==(const SVGLengthList& rhs) const
{
  if (Length() != rhs.Length()) {
    return false;
  }
  for (PRUint32 i = 0; i < Length(); ++i) {
    if (!(mLengths[i] == rhs.mLengths[i])) {
      return false;
    }
  }
  return true;
}

} 
