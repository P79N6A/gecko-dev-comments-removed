




#ifndef nsTransform2D_h___
#define nsTransform2D_h___

#include "gfxCore.h"
#include "nsCoord.h"

class NS_GFX nsTransform2D
{
private:
 











  float     m00, m11, m20, m21;

public:
  nsTransform2D(void)                         { m20 = m21 = 0.0f; m00 = m11 = 1.0f; }

  ~nsTransform2D(void)                        { }

 






  void SetToTranslate(float tx, float ty)    { m00 = m11 = 1.0f; m20 = tx; m21 = ty; }
  
 





  void GetTranslationCoord(nscoord *ptX, nscoord *ptY) const { *ptX = NSToCoordRound(m20); *ptY = NSToCoordRound(m21); }

 





  void TransformCoord(nscoord *ptX, nscoord *ptY) const;

 





  void TransformCoord(nscoord *aX, nscoord *aY, nscoord *aWidth, nscoord *aHeight) const;

 






  void AddScale(float ptX, float ptY) { m00 *= ptX; m11 *= ptY; }

 







  void SetScale(float ptX, float ptY) { m00 = ptX; m11 = ptY; }
};

#endif
