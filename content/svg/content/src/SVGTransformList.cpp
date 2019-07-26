





#include "SVGTransformList.h"
#include "SVGTransformListParser.h"
#include "nsString.h"
#include "nsError.h"

namespace mozilla {

gfxMatrix
SVGTransformList::GetConsolidationMatrix() const
{
  
  
  
  gfxMatrix result;

  if (mItems.IsEmpty())
    return result;

  result = mItems[0].Matrix();

  if (mItems.Length() == 1)
    return result;

  for (uint32_t i = 1; i < mItems.Length(); ++i) {
    result.PreMultiply(mItems[i].Matrix());
  }

  return result;
}

nsresult
SVGTransformList::CopyFrom(const SVGTransformList& rhs)
{
  return CopyFrom(rhs.mItems);
}

nsresult
SVGTransformList::CopyFrom(const nsTArray<nsSVGTransform>& aTransformArray)
{
  if (!mItems.SetCapacity(aTransformArray.Length())) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mItems = aTransformArray;
  return NS_OK;
}

void
SVGTransformList::GetValueAsString(nsAString& aValue) const
{
  aValue.Truncate();
  uint32_t last = mItems.Length() - 1;
  for (uint32_t i = 0; i < mItems.Length(); ++i) {
    nsAutoString length;
    mItems[i].GetValueAsString(length);
    
    aValue.Append(length);
    if (i != last) {
      aValue.Append(' ');
    }
  }
}

nsresult
SVGTransformList::SetValueFromString(const nsAString& aValue)
{
  SVGTransformListParser parser;
  nsresult rv = parser.Parse(aValue);

  if (NS_FAILED(rv)) {
    
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  return CopyFrom(parser.GetTransformList());
}

} 
