




#ifndef GFX_3DMATRIX_H
#define GFX_3DMATRIX_H

#include <gfxTypes.h>
#include <gfxPoint3D.h>
#include <gfxPointH3D.h>
#include <gfxQuad.h>

class gfxMatrix;
















class gfx3DMatrix
{
public:
  


  gfx3DMatrix(void);

  friend std::ostream& operator<<(std::ostream& stream, const gfx3DMatrix& m) {
    if (m.IsIdentity()) {
      return stream << "[ I ]";
    }

    if (m.Is2D()) {
      return stream << "["
             << m._11 << " " << m._12 << "; "
             << m._21 << " " << m._22 << "; "
             << m._41 << " " << m._42
             << "]";
    }

    return stream << "["
           << m._11 << " " << m._12 << " " << m._13 << " " << m._14 << "; "
           << m._21 << " " << m._22 << " " << m._23 << " " << m._24 << "; "
           << m._31 << " " << m._32 << " " << m._33 << " " << m._34 << "; "
           << m._41 << " " << m._42 << " " << m._43 << " " << m._44
           << "]";
  }

  


  gfx3DMatrix operator*(const gfx3DMatrix &aMatrix) const;
  gfx3DMatrix& operator*=(const gfx3DMatrix &aMatrix);

  gfxPointH3D& operator[](int aIndex)
  {
      NS_ABORT_IF_FALSE(aIndex >= 0 && aIndex <= 3, "Invalid matrix array index");
      return *reinterpret_cast<gfxPointH3D*>((&_11)+4*aIndex);
  }
  const gfxPointH3D& operator[](int aIndex) const
  {
      NS_ABORT_IF_FALSE(aIndex >= 0 && aIndex <= 3, "Invalid matrix array index");
      return *reinterpret_cast<const gfxPointH3D*>((&_11)+4*aIndex);
  }

  


  bool operator==(const gfx3DMatrix& aMatrix) const;
  bool operator!=(const gfx3DMatrix& aMatrix) const;

  bool FuzzyEqual(const gfx3DMatrix& aMatrix) const;
  
  


  gfx3DMatrix& operator/=(gfxFloat scalar);

  




  static gfx3DMatrix From2D(const gfxMatrix &aMatrix);

  




  bool Is2D(gfxMatrix* aMatrix) const;
  bool Is2D() const;

  











  bool CanDraw2D(gfxMatrix* aMatrix = nullptr) const;

  



  gfx3DMatrix& ProjectTo2D();

  



  bool IsIdentity() const;

  







  








  void Translate(const gfxPoint3D& aPoint);

  








  void SkewXY(double aXSkew, double aYSkew);
  
  void SkewXY(double aSkew);
  void SkewXZ(double aSkew);
  void SkewYZ(double aSkew);

  








  void Scale(float aX, float aY, float aZ);

  


  float GetXScale() const { return _11; }
  float GetYScale() const { return _22; }
  float GetZScale() const { return _33; }

  








  void RotateX(double aTheta);
  
  








  void RotateY(double aTheta);
  
  








  void RotateZ(double aTheta);

  








  void Perspective(float aDepth);

  



  void PreMultiply(const gfx3DMatrix& aOther);
  void PreMultiply(const gfxMatrix& aOther);

  







  




  void TranslatePost(const gfxPoint3D& aPoint);

  void ScalePost(float aX, float aY, float aZ);

  









  void ChangeBasis(const gfxPoint3D& aOrigin);

  


  gfxPoint Transform(const gfxPoint& point) const;

  


  gfxRect TransformBounds(const gfxRect& rect) const;


  gfxQuad TransformRect(const gfxRect& aRect) const;

  


  gfxPoint3D Transform3D(const gfxPoint3D& point) const;
  gfxPointH3D Transform4D(const gfxPointH3D& aPoint) const;
  gfxPointH3D TransposeTransform4D(const gfxPointH3D& aPoint) const;

  



  gfxPointH3D ProjectPoint(const gfxPoint& aPoint) const;

  



  gfx3DMatrix Inverse() const;

  gfx3DMatrix& Invert()
  {
      *this = Inverse();
      return *this;
  }

  gfx3DMatrix& Normalize();

  gfxPointH3D TransposedVector(int aIndex) const
  {
      NS_ABORT_IF_FALSE(aIndex >= 0 && aIndex <= 3, "Invalid matrix array index");
      return gfxPointH3D(*((&_11)+aIndex), *((&_21)+aIndex), *((&_31)+aIndex), *((&_41)+aIndex));
  }

  void SetTransposedVector(int aIndex, gfxPointH3D &aVector)
  {
      NS_ABORT_IF_FALSE(aIndex >= 0 && aIndex <= 3, "Invalid matrix array index");
      *((&_11)+aIndex) = aVector.x;
      *((&_21)+aIndex) = aVector.y;
      *((&_31)+aIndex) = aVector.z;
      *((&_41)+aIndex) = aVector.w;
  }

  gfx3DMatrix& Transpose();
  gfx3DMatrix Transposed() const;

  



  gfxPoint3D GetNormalVector() const;

  



  bool IsBackfaceVisible() const;

  


  bool IsSingular() const;

  






  static gfx3DMatrix Translation(float aX, float aY, float aZ);
  static gfx3DMatrix Translation(const gfxPoint3D& aPoint);

  




  static gfx3DMatrix ScalingMatrix(float aFactor);

  


  static gfx3DMatrix ScalingMatrix(float aX, float aY, float aZ);

  gfxFloat Determinant() const;

  void NudgeToIntegers(void);
  void NudgeToIntegersFixedEpsilon();

private:

  gfxFloat Determinant3x3() const;
  gfx3DMatrix Inverse3x3() const;

  gfx3DMatrix Multiply2D(const gfx3DMatrix &aMatrix) const;

public:

  
  float _11, _12, _13, _14;
  float _21, _22, _23, _24;
  float _31, _32, _33, _34;
  float _41, _42, _43, _44;
};

#endif 
