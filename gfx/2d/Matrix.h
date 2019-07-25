




































#ifndef MOZILLA_GFX_MATRIX_H_
#define MOZILLA_GFX_MATRIX_H_

#include "Types.h"
#include "Rect.h"
#include "Point.h"
#include <math.h>

namespace mozilla {
namespace gfx {

class Matrix
{
public:
  Matrix()
    : _11(1.0f), _12(0)
    , _21(0), _22(1.0f)
    , _31(0), _32(0)
  {}
  Matrix(Float a11, Float a12, Float a21, Float a22, Float a31, Float a32)
    : _11(a11), _12(a12)
    , _21(a21), _22(a22)
    , _31(a31), _32(a32)
  {}
  Float _11, _12;
  Float _21, _22;
  Float _31, _32;

  Point operator *(const Point &aPoint) const
  {
    Point retPoint;

    retPoint.x = aPoint.x * _11 + aPoint.y * _21 + _31;
    retPoint.y = aPoint.x * _12 + aPoint.y * _22 + _32;

    return retPoint;
  }

  Rect TransformBounds(const Rect& rect) const;

  
  
  Matrix &Scale(Float aX, Float aY)
  {
    _11 *= aX;
    _12 *= aX;
    _21 *= aY;
    _22 *= aY;

    return *this;
  }

  Matrix &Translate(Float aX, Float aY)
  {
    _31 += _11 * aX + _21 * aY;
    _32 += _12 * aX + _22 * aY;

    return *this;
  }

  bool Invert()
  {
    
    Float A = _22;
    Float B = -_21;
    Float C = _21 * _32 - _22 * _31;
    Float D = -_12;
    Float E = _11;
    Float F = _31 * _12 - _11 * _32;

    Float det = Determinant();

    if (!det) {
      return false;
    }

    Float inv_det = 1 / det;

    _11 = inv_det * A;
    _12 = inv_det * D;
    _21 = inv_det * B;
    _22 = inv_det * E;
    _31 = inv_det * C;
    _32 = inv_det * F;

    return true;
  }

  Float Determinant() const
  {
    return _11 * _22 - _12 * _21;
  }
  
  static Matrix Rotation(Float aAngle);

  Matrix operator*(const Matrix &aMatrix) const
  {
    Matrix resultMatrix;

    resultMatrix._11 = this->_11 * aMatrix._11 + this->_12 * aMatrix._21;
    resultMatrix._12 = this->_11 * aMatrix._12 + this->_12 * aMatrix._22;
    resultMatrix._21 = this->_21 * aMatrix._11 + this->_22 * aMatrix._21;
    resultMatrix._22 = this->_21 * aMatrix._12 + this->_22 * aMatrix._22;
    resultMatrix._31 = this->_31 * aMatrix._11 + this->_32 * aMatrix._21 + aMatrix._31;
    resultMatrix._32 = this->_31 * aMatrix._12 + this->_32 * aMatrix._22 + aMatrix._32;

    return resultMatrix;
  }
};

}
}

#endif 
