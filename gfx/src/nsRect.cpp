




































#include "nsRect.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"

#ifdef MIN
#undef MIN
#endif

#ifdef MAX
#undef MAX
#endif

#define MIN(a,b)\
  ((a) < (b) ? (a) : (b))
#define MAX(a,b)\
  ((a) > (b) ? (a) : (b))


PRBool nsRect::Contains(nscoord aX, nscoord aY) const
{
  return (PRBool) ((aX >= x) && (aY >= y) &&
                   (aX < XMost()) && (aY < YMost()));
}

PRBool nsRect::Contains(const nsRect &aRect) const
{
  return (PRBool) ((aRect.x >= x) && (aRect.y >= y) &&
                   (aRect.XMost() <= XMost()) && (aRect.YMost() <= YMost()));
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

  x = MAX(aRect1.x, aRect2.x);
  y = MAX(aRect1.y, aRect2.y);

  
  temp = MIN(xmost1, xmost2);
  if (temp <= x) {
    Empty();
    return PR_FALSE;
  }
  width = temp - x;

  
  temp = MIN(ymost1, ymost2);
  if (temp <= y) {
    Empty();
    return PR_FALSE;
  }
  height = temp - y;

  return PR_TRUE;
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
    nscoord xmost1 = aRect1.XMost();
    nscoord xmost2 = aRect2.XMost();
    nscoord ymost1 = aRect1.YMost();
    nscoord ymost2 = aRect2.YMost();

    
    x = MIN(aRect1.x, aRect2.x);
    y = MIN(aRect1.y, aRect2.y);

    
    width = MAX(xmost1, xmost2) - x;
    height = MAX(ymost1, ymost2) - y;
  }

  return result;
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
  width -= 2 * aDx;
  height -= 2 * aDy;
}


void nsRect::Deflate(const nsMargin &aMargin)
{
  x += aMargin.left;
  y += aMargin.top;
  width -= aMargin.left + aMargin.right;
  height -= aMargin.top + aMargin.bottom;
}


nsRect& nsRect::ScaleRoundOut(float aScale) 
{
  nscoord right = NSToCoordCeil(float(XMost()) * aScale);
  nscoord bottom = NSToCoordCeil(float(YMost()) * aScale);
  x = NSToCoordFloor(float(x) * aScale);
  y = NSToCoordFloor(float(y) * aScale);
  width = (right - x);
  height = (bottom - y);
  return *this;
}


nsRect& nsRect::ScaleRoundIn(float aScale) 
{
  nscoord right = NSToCoordFloor(float(XMost()) * aScale);
  nscoord bottom = NSToCoordFloor(float(YMost()) * aScale);
  x = NSToCoordCeil(float(x) * aScale);
  y = NSToCoordCeil(float(y) * aScale);
  width = (right - x);
  height = (bottom - y);
  return *this;
}

nsRect& nsRect::ScaleRoundPreservingCenters(float aScale)
{
  nscoord right = NSToCoordRound(float(XMost()) * aScale);
  nscoord bottom = NSToCoordRound(float(YMost()) * aScale);
  x = NSToCoordRound(float(x) * aScale);
  y = NSToCoordRound(float(y) * aScale);
  width = (right - x);
  height = (bottom - y);
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

#ifdef NS_COORD_IS_FLOAT


PRBool nsIntRect::IntersectRect(const nsIntRect &aRect1, const nsIntRect &aRect2)
{
  PRInt32  xmost1 = aRect1.XMost();
  PRInt32  ymost1 = aRect1.YMost();
  PRInt32  xmost2 = aRect2.XMost();
  PRInt32  ymost2 = aRect2.YMost();
  PRInt32  temp;

  x = MAX(aRect1.x, aRect2.x);
  y = MAX(aRect1.y, aRect2.y);

  
  temp = MIN(xmost1, xmost2);
  if (temp <= x) {
    Empty();
    return PR_FALSE;
  }
  width = temp - x;

  
  temp = MIN(ymost1, ymost2);
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

    
    x = MIN(aRect1.x, aRect2.x);
    y = MIN(aRect1.y, aRect2.y);

    
    width = MAX(xmost1, xmost2) - x;
    height = MAX(ymost1, ymost2) - y;
  }

  return result;
}
#endif

#endif 
