





































#include "nsTransform2D.h"

void nsTransform2D :: TransformCoord(nscoord *ptX, nscoord *ptY) const
{
  *ptX = NSToCoordRound(*ptX * m00 + m20);
  *ptY = NSToCoordRound(*ptY * m11 + m21);
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
