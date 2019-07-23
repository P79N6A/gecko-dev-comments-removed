





































#include "nsTransform2D.h"

void nsTransform2D :: SetMatrix(nsTransform2D *aTransform2D)
{
  m00 = aTransform2D->m00;
  m11 = aTransform2D->m11;
  m20 = aTransform2D->m20;
  m21 = aTransform2D->m21;
  type = aTransform2D->type;
}

void nsTransform2D :: Concatenate(nsTransform2D *newxform)
{
  PRUint16  newtype = newxform->type;

  if (newtype == MG_2DIDENTITY)
  {
    return;
  }
  else if (type == MG_2DIDENTITY)
  {
    SetMatrix(newxform);
    return;
  }
  else if ((type & MG_2DSCALE) != 0)
  {
    

    if ((newtype & MG_2DSCALE) != 0)
    {
      

      if ((newtype & MG_2DTRANSLATION) != 0)
      {
        m20 += newxform->m20 * m00;
        m21 += newxform->m21 * m11;
      }

      m00 *= newxform->m00;
      m11 *= newxform->m11;
    }
    else
    {
      

      m20 += newxform->m20 * m00;
      m21 += newxform->m21 * m11;
    }
  }
  else
  {
    

    if ((newtype & MG_2DSCALE) != 0)
    {
      

      if ((newtype & MG_2DTRANSLATION) != 0)
      {
        m20 += newxform->m20;
        m21 += newxform->m21;
      }

      m00 = newxform->m00;
      m11 = newxform->m11;
    }
    else
    {
        

        m20 += newxform->m20;
        m21 += newxform->m21;
    }
  }

  type |= newtype;
}

void nsTransform2D :: PreConcatenate(nsTransform2D *newxform)
{
  float new00 = newxform->m00;
  float new11 = newxform->m11;

  m00 *= new00;
  m11 *= new11;
  m20 = m20 * new00 + newxform->m20;
  m21 = m21 * new11 + newxform->m21;

  type |= newxform->type;
}

void nsTransform2D :: TransformNoXLate(float *ptX, float *ptY) const
{
  if ((type & MG_2DSCALE) != 0) {
    *ptX *= m00;
    *ptY *= m11;
  }
}

void nsTransform2D :: TransformNoXLateCoord(nscoord *ptX, nscoord *ptY) const
{
  if ((type & MG_2DSCALE) != 0) {
    *ptX = NSToCoordRound(*ptX * m00);
    *ptY = NSToCoordRound(*ptY * m11);
  }
}

inline PRIntn NSToIntNFloor(float aValue)
{
  return PRIntn(floor(aValue));
}

void nsTransform2D :: ScaleXCoords(const nscoord* aSrc,
                                   PRUint32 aNumCoords,
                                   PRIntn* aDst) const
{
const nscoord* end = aSrc + aNumCoords;

  if (type == MG_2DIDENTITY){
    while (aSrc < end ) {
      *aDst++ = PRIntn(*aSrc++);
    }
  } else {
    float scale = m00;
    while (aSrc < end) {
      nscoord c = *aSrc++;
      *aDst++ = NSToIntNFloor(c * scale);
    }
  }
}

void nsTransform2D :: ScaleYCoords(const nscoord* aSrc,
                                   PRUint32 aNumCoords,
                                   PRIntn* aDst) const
{
const nscoord* end = aSrc + aNumCoords;

  if (type == MG_2DIDENTITY){
    while (aSrc < end ) {
      *aDst++ = PRIntn(*aSrc++); 
    } 
  } else {
    float scale = m11;
    while (aSrc < end) {
      nscoord c = *aSrc++;
      *aDst++ = NSToIntNFloor(c * scale);
    }
  }
}
  

void nsTransform2D :: Transform(float *ptX, float *ptY) const
{
  switch (type)
  {
    case MG_2DIDENTITY:
      break;

    case MG_2DTRANSLATION:
      *ptX += m20;
      *ptY += m21;
      break;

    case MG_2DSCALE:
      *ptX *= m00;
      *ptY *= m11;
      break;

    case MG_2DSCALE | MG_2DTRANSLATION:
      *ptX = *ptX * m00 + m20;
      *ptY = *ptY * m11 + m21;
      break;

    default:
      NS_ASSERTION(0, "illegal type");
      break;
  }
}

void nsTransform2D :: TransformCoord(nscoord *ptX, nscoord *ptY) const
{
  switch (type)
  {
    case MG_2DIDENTITY:
      break;

    case MG_2DTRANSLATION:
      *ptX += NSToCoordRound(m20);
      *ptY += NSToCoordRound(m21);
      break;

    case MG_2DSCALE:
      *ptX = NSToCoordRound(*ptX * m00);
      *ptY = NSToCoordRound(*ptY * m11);
      break;

    case MG_2DSCALE | MG_2DTRANSLATION:
      *ptX = NSToCoordRound(*ptX * m00 + m20);
      *ptY = NSToCoordRound(*ptY * m11 + m21);
      break;

    default:
      NS_ASSERTION(0, "illegal type");
      break;
  }
}

void nsTransform2D :: Transform(float *aX, float *aY, float *aWidth, float *aHeight) const
{
  switch (type)
  {
    case MG_2DIDENTITY:
      break;

    case MG_2DTRANSLATION:
      *aX += m20;
      *aY += m21;
      break;

    case MG_2DSCALE:
      *aX *= m00;
      *aY *= m11;
      *aWidth *= m00;
      *aHeight *= m11;
      break;

    case MG_2DSCALE | MG_2DTRANSLATION:
      *aX = *aX * m00 + m20;
      *aY = *aY * m11 + m21;
      *aWidth *= m00;
      *aHeight *= m11;
      break;

    default:
      NS_ASSERTION(0, "illegal type");
      break;
  }
}

void nsTransform2D :: TransformCoord(nscoord *aX, nscoord *aY, nscoord *aWidth, nscoord *aHeight) const
{
  nscoord x2 = *aX + *aWidth;
  nscoord y2 = *aY + *aHeight;
  TransformCoord(aX, aY);
  TransformCoord(&x2, &y2);
  *aWidth = x2 - *aX;
  *aHeight = y2 - *aY;
}

void nsTransform2D :: TransformNoXLateCoord(nscoord *aX, nscoord *aY, nscoord *aWidth, nscoord *aHeight) const
{
  nscoord x2 = *aX + *aWidth;
  nscoord y2 = *aY + *aHeight;
  TransformNoXLateCoord(aX, aY);
  TransformNoXLateCoord(&x2, &y2);
  *aWidth = x2 - *aX;
  *aHeight = y2 - *aY;
}

void nsTransform2D :: AddTranslation(float ptX, float ptY)
{
  if (type == MG_2DIDENTITY)
  {
    m20 = ptX;
    m21 = ptY;
  }
  else if ((type & MG_2DSCALE) != 0)
  {
    m20 += ptX * m00;
    m21 += ptY * m11;
  }
  else
  {
    m20 += ptX;
    m21 += ptY;
  }

  type |= MG_2DTRANSLATION;
}

void nsTransform2D :: AddScale(float ptX, float ptY)
{
  if ((type == MG_2DIDENTITY) || (type == MG_2DTRANSLATION))
  {
    m00 = ptX;
    m11 = ptY;
  }
  else
  {
    m00 *= ptX;
    m11 *= ptY;
  }

  type |= MG_2DSCALE;
}
