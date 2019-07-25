





































#ifndef GFX_3DMATRIX_H
#define GFX_3DMATRIX_H

#include <gfxTypes.h>
#include <gfxMatrix.h>
















class THEBES_API gfx3DMatrix
{
public:
  


  gfx3DMatrix(void);

  


  gfx3DMatrix operator*(const gfx3DMatrix &aMatrix) const;
  gfx3DMatrix& operator*=(const gfx3DMatrix &aMatrix);

  


  bool operator==(const gfx3DMatrix& aMatrix) const;
  
  


  gfx3DMatrix& operator/=(gfxFloat scalar);

  




  static gfx3DMatrix From2D(const gfxMatrix &aMatrix);

  




  PRBool Is2D(gfxMatrix* aMatrix = nsnull) const;

  



  PRBool IsIdentity() const;

  


  gfxPoint Transform(const gfxPoint& point) const;

  


  gfxRect TransformBounds(const gfxRect& rect) const;

  



  gfx3DMatrix& Invert();

  


  PRBool IsSingular() const;

  






  static gfx3DMatrix Translation(float aX, float aY, float aZ);

  




  static gfx3DMatrix Scale(float aFactor);

  


  static gfx3DMatrix Scale(float aX, float aY, float aZ);

private:

  gfxFloat Determinant() const;

public:

  
  float _11, _12, _13, _14;
  float _21, _22, _23, _24;
  float _31, _32, _33, _34;
  float _41, _42, _43, _44;
};

#endif 
