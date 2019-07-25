




































#include "nsRect.h"
#include "nsString.h"
#include "nsIDeviceContext.h"
#include "prlog.h"
#include <limits.h>


PR_STATIC_ASSERT((NS_SIDE_TOP == 0) && (NS_SIDE_RIGHT == 1) && (NS_SIDE_BOTTOM == 2) && (NS_SIDE_LEFT == 3));



const nsIntRect nsIntRect::kMaxSizedIntRect(0, 0, INT_MAX, INT_MAX);


PRBool nsRect::Contains(nscoord aX, nscoord aY) const
{
  return (PRBool) ((aX >= x) && (aY >= y) &&
                   (aX < XMost()) && (aY < YMost()));
}


PRBool nsRect::Contains(const nsRect &aRect) const
{
  return aRect.IsEmpty() ||
          ((PRBool) ((aRect.x >= x) && (aRect.y >= y) &&
                    (aRect.XMost() <= XMost()) && (aRect.YMost() <= YMost())));
}



PRBool nsRect::Intersects(const nsRect &aRect) const
{
  return (PRBool) ((x < aRect.XMost()) && (y < aRect.YMost()) &&
                   (aRect.x < XMost()) && (aRect.y < YMost()));
}



PRBool nsRect::IntersectRect(const nsRect &aRect1, const nsRect &aRect2)
{
  nscoord  xmost1 = aRect1.XMost();
  nscoord  ymost1 = aRect1.YMost();
  nscoord  xmost2 = aRect2.XMost();
  nscoord  ymost2 = aRect2.YMost();
  nscoord  temp;

  x = PR_MAX(aRect1.x, aRect2.x);
  y = PR_MAX(aRect1.y, aRect2.y);

  
  temp = PR_MIN(xmost1, xmost2);
  if (temp <= x) {
    width = 0;
  } else {
    width = temp - x;
  }

  
  temp = PR_MIN(ymost1, ymost2);
  if (temp <= y) {
    height = 0;
  } else {
    height = temp - y;
  }

  return !IsEmpty();
}




PRBool nsRect::UnionRect(const nsRect &aRect1, const nsRect &aRect2)
{
  PRBool  result = PR_TRUE;

  
  if (aRect1.IsEmpty()) {
    if (aRect2.IsEmpty()) {
      
      Empty();
      result = PR_FALSE;
    } else {
      
      *this = aRect2;
    }
  } else if (aRect2.IsEmpty()) {
    
    *this = aRect1;
  } else {
    UnionRectIncludeEmpty(aRect1, aRect2);
  }

  return result;
}

void nsRect::UnionRectIncludeEmpty(const nsRect &aRect1, const nsRect &aRect2)
{
  nscoord xmost1 = aRect1.XMost();
  nscoord xmost2 = aRect2.XMost();
  nscoord ymost1 = aRect1.YMost();
  nscoord ymost2 = aRect2.YMost();

  
  x = PR_MIN(aRect1.x, aRect2.x);
  y = PR_MIN(aRect1.y, aRect2.y);

  
  width = PR_MAX(xmost1, xmost2) - x;
  height = PR_MAX(ymost1, ymost2) - y;
}


void nsRect::Inflate(nscoord aDx, nscoord aDy)
{
  x -= aDx;
  y -= aDy;
  width += 2 * aDx;
  height += 2 * aDy;
}


void nsRect::Inflate(const nsMargin &aMargin)
{
  x -= aMargin.left;
  y -= aMargin.top;
  width += aMargin.left + aMargin.right;
  height += aMargin.top + aMargin.bottom;
}


void nsRect::Deflate(nscoord aDx, nscoord aDy)
{
  x += aDx;
  y += aDy;
  width = PR_MAX(0, width - 2 * aDx);
  height = PR_MAX(0, height - 2 * aDy);
}


void nsRect::Deflate(const nsMargin &aMargin)
{
  x += aMargin.left;
  y += aMargin.top;
  width = PR_MAX(0, width - aMargin.LeftRight());
  height = PR_MAX(0, height - aMargin.TopBottom());
}


nsMargin nsRect::operator-(const nsRect& aRect) const
{
  nsMargin margin;
  margin.left = aRect.x - x;
  margin.right = XMost() - aRect.XMost();
  margin.top = aRect.y - y;
  margin.bottom = YMost() - aRect.YMost();
  return margin;
}


nsRect& nsRect::ScaleRoundOut(float aXScale, float aYScale)
{
  nscoord right = NSToCoordCeil(float(XMost()) * aXScale);
  nscoord bottom = NSToCoordCeil(float(YMost()) * aYScale);
  x = NSToCoordFloor(float(x) * aXScale);
  y = NSToCoordFloor(float(y) * aYScale);
  width = (right - x);
  height = (bottom - y);
  return *this;
}

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
                       nsIDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral(", ");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.y,
                       nsIDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral(", ");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.width,
                       nsIDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral(", ");
  tmp.AppendFloat(NSAppUnitsToFloatPixels(rect.height,
                       nsIDeviceContext::AppUnitsPerCSSPixel()));
  tmp.AppendLiteral("}");
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
  return out;
}

#endif 



PRBool nsIntRect::IntersectRect(const nsIntRect &aRect1, const nsIntRect &aRect2)
{
  PRInt32  xmost1 = aRect1.XMost();
  PRInt32  ymost1 = aRect1.YMost();
  PRInt32  xmost2 = aRect2.XMost();
  PRInt32  ymost2 = aRect2.YMost();
  PRInt32  temp;

  x = PR_MAX(aRect1.x, aRect2.x);
  y = PR_MAX(aRect1.y, aRect2.y);

  
  temp = PR_MIN(xmost1, xmost2);
  if (temp <= x) {
    Empty();
    return PR_FALSE;
  }
  width = temp - x;

  
  temp = PR_MIN(ymost1, ymost2);
  if (temp <= y) {
    Empty();
    return PR_FALSE;
  }
  height = temp - y;

  return PR_TRUE;
}




PRBool nsIntRect::UnionRect(const nsIntRect &aRect1, const nsIntRect &aRect2)
{
  PRBool  result = PR_TRUE;

  
  if (aRect1.IsEmpty()) {
    if (aRect2.IsEmpty()) {
      
      Empty();
      result = PR_FALSE;
    } else {
      
      *this = aRect2;
    }
  } else if (aRect2.IsEmpty()) {
    
    *this = aRect1;
  } else {
    PRInt32 xmost1 = aRect1.XMost();
    PRInt32 xmost2 = aRect2.XMost();
    PRInt32 ymost1 = aRect1.YMost();
    PRInt32 ymost2 = aRect2.YMost();

    
    x = PR_MIN(aRect1.x, aRect2.x);
    y = PR_MIN(aRect1.y, aRect2.y);

    
    width = PR_MAX(xmost1, xmost2) - x;
    height = PR_MAX(ymost1, ymost2) - y;
  }

  return result;
}


nsIntRect& nsIntRect::ScaleRoundOut(float aXScale, float aYScale)
{
  nscoord right = NSToCoordCeil(float(XMost()) * aXScale);
  nscoord bottom = NSToCoordCeil(float(YMost()) * aYScale);
  x = NSToCoordFloor(float(x) * aXScale);
  y = NSToCoordFloor(float(y) * aYScale);
  width = (right - x);
  height = (bottom - y);
  return *this;
}
