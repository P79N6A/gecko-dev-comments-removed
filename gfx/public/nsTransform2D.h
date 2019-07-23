




































#ifndef nsTransform2D_h___
#define nsTransform2D_h___

#include "gfxCore.h"
#include "nsCoord.h"
#include "nsUnitConversion.h"

#define MG_2DIDENTITY     0
#define MG_2DTRANSLATION  1
#define MG_2DSCALE        2

class NS_GFX nsTransform2D
{
private:
 











  float     m00, m11, m20, m21;
  PRUint16  type;

public:
  nsTransform2D(void)                         { SetToIdentity(); }
  nsTransform2D(nsTransform2D *aTransform2D)  { SetMatrix(aTransform2D); }

  ~nsTransform2D(void)                        { }

 








  PRUint16 GetType(void) const                { return type; }

 







  void SetToIdentity(void)                    { m20 = m21 = 0.0f; m00 = m11 = 1.0f; type = MG_2DIDENTITY; }

 








  void SetToScale(float sx, float sy)        { m00 = sx; m11 = sy; m20 = m21 = 0.0f; type = MG_2DSCALE; }
  

 








  void SetToTranslate(float tx, float ty)    { m00 = m11 = 1.0f; m20 = tx; m21 = ty; type = MG_2DTRANSLATION; }
  

 







  void GetTranslation(float *ptX, float *ptY) const { *ptX = m20; *ptY = m21; }
  void GetTranslationCoord(nscoord *ptX, nscoord *ptY) const { *ptX = NSToCoordRound(m20); *ptY = NSToCoordRound(m21); }

 







  void SetTranslation(float tX, float tY) {
    m20 = tX;
    m21 = tY;
    type |= MG_2DTRANSLATION;
  }

 







  float GetXTranslation(void)  const          { return m20; }
  nscoord GetXTranslationCoord(void) const    { return NSToCoordRound(m20); }

 







  float GetYTranslation(void) const         { return m21; }
  nscoord GetYTranslationCoord(void) const  { return NSToCoordRound(m21); }

 







  void SetMatrix(nsTransform2D *aTransform2D);

 







  void Concatenate(nsTransform2D *newxform);

 







  void PreConcatenate(nsTransform2D *newxform);

 







  void TransformNoXLate(float *ptX, float *ptY) const;
  void TransformNoXLateCoord(nscoord *ptX, nscoord *ptY) const;

 







  void Transform(float *ptX, float *ptY) const;
  void TransformCoord(nscoord *ptX, nscoord *ptY) const;

 







  void Transform(float *aX, float *aY, float *aWidth, float *aHeight) const;
  void TransformCoord(nscoord *aX, nscoord *aY, nscoord *aWidth, nscoord *aHeight) const;
  void TransformNoXLateCoord(nscoord *aX, nscoord *aY, nscoord *aWidth, nscoord *aHeight) const;

  







  void ScaleXCoords(const nscoord* aSrc, PRUint32 aNumCoords, PRIntn* aDst) const;
  void ScaleYCoords(const nscoord* aSrc, PRUint32 aNumCoords, PRIntn* aDst) const;

 








  void AddTranslation(float ptX, float ptY);

 








  void AddScale(float ptX, float ptY);
};

#endif
