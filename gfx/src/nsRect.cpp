




































#include "nsRect.h"
#include "nsString.h"
#include "nsDeviceContext.h"
#include "prlog.h"
#include <limits.h>


PR_STATIC_ASSERT((NS_SIDE_TOP == 0) && (NS_SIDE_RIGHT == 1) && (NS_SIDE_BOTTOM == 2) && (NS_SIDE_LEFT == 3));


const nsIntRect nsIntRect::kMaxSizedIntRect(0, 0, INT_MAX, INT_MAX);

#ifdef DEBUG
static bool IsFloatInteger(float aFloat)
{
  return fabs(aFloat - NS_round(aFloat)) < 1e-6;
}
#endif

nsRect& nsRect::ExtendForScaling(float aXMult, float aYMult)
{
  NS_ASSERTION((IsFloatInteger(aXMult) || IsFloatInteger(1/aXMult)) &&
               (IsFloatInteger(aYMult) || IsFloatInteger(1/aYMult)),
               "Multiplication factors must be integers or 1/integer");

  
  
  if (aXMult < 1) {
    nscoord right = NSToCoordRound(ceil(float(XMost()) * aXMult) / aXMult);
    x = NSToCoordRound(floor(float(x) * aXMult) / aXMult);
    width = right - x;
  }
  if (aYMult < 1) {
    nscoord bottom = NSToCoordRound(ceil(float(YMost()) * aYMult) / aYMult);
    y = NSToCoordRound(floor(float(y) * aYMult) / aYMult);
    height = bottom - y;
  }
  return *this;
}

#ifdef DEBUG


FILE* operator<<(FILE* out, const nsRect& rect)
{
  nsAutoString tmp;

  
  tmp.AppendLiteral("{");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.x,
                       nsDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral(", ");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.y,
                       nsDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral(", ");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.width,
                       nsDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral(", ");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.height,
                       nsDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral("}");
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
  return out;
}

#endif 
