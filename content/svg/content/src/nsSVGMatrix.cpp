





































#include "nsSVGMatrix.h"
#include "nsDOMError.h"
#include "nsSVGValue.h"
#include <math.h>
#include "nsContentUtils.h"

const double radPerDegree = 2.0*3.1415926535 / 360.0;

class nsSVGMatrix : public nsIDOMSVGMatrix,
                    public nsSVGValue
{
public:
  nsSVGMatrix(float a, float b, float c, float d, float e, float f);
  
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGMATRIX

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);
  
protected:
  float mA, mB, mC, mD, mE, mF;

  
  nsresult RotateRadians(float rad, nsIDOMSVGMatrix **_retval);
};




nsresult
NS_NewSVGMatrix(nsIDOMSVGMatrix** result,
                float a, float b, float c,
                float d, float e, float f)
{
  *result = new nsSVGMatrix(a, b, c, d, e, f);
  if (!*result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

nsSVGMatrix::nsSVGMatrix(float a, float b, float c,
                         float d, float e, float f)
  : mA(a), mB(b), mC(c), mD(d), mE(e), mF(f)
{
}




NS_IMPL_ADDREF(nsSVGMatrix)
NS_IMPL_RELEASE(nsSVGMatrix)

NS_INTERFACE_MAP_BEGIN(nsSVGMatrix)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGMatrix)


  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGMatrix)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END






NS_IMETHODIMP nsSVGMatrix::GetA(float *aA)
{
  *aA = mA;
  return NS_OK;
}
NS_IMETHODIMP nsSVGMatrix::SetA(float aA)
{
  WillModify();
  mA = aA;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGMatrix::GetB(float *aB)
{
  *aB = mB;
  return NS_OK;
}
NS_IMETHODIMP nsSVGMatrix::SetB(float aB)
{
  WillModify();
  mB = aB;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGMatrix::GetC(float *aC)
{
  *aC = mC;
  return NS_OK;
}
NS_IMETHODIMP nsSVGMatrix::SetC(float aC)
{
  WillModify();
  mC = aC;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGMatrix::GetD(float *aD)
{
  *aD = mD;
  return NS_OK;
}
NS_IMETHODIMP nsSVGMatrix::SetD(float aD)
{
  WillModify();
  mD = aD;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGMatrix::GetE(float *aE)
{
  *aE = mE;
  return NS_OK;
}
NS_IMETHODIMP nsSVGMatrix::SetE(float aE)
{
  WillModify();
  mE = aE;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGMatrix::GetF(float *aF)
{
  *aF = mF;
  return NS_OK;
}
NS_IMETHODIMP nsSVGMatrix::SetF(float aF)
{
  WillModify();
  mF = aF;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGMatrix::Multiply(nsIDOMSVGMatrix *secondMatrix,
                                    nsIDOMSVGMatrix **_retval)
{
  if (!secondMatrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  float sa,sb,sc,sd,se,sf;
  secondMatrix->GetA(&sa);
  secondMatrix->GetB(&sb);
  secondMatrix->GetC(&sc);
  secondMatrix->GetD(&sd);
  secondMatrix->GetE(&se);
  secondMatrix->GetF(&sf);

  return NS_NewSVGMatrix(_retval,
                         mA*sa + mC*sb,      mB*sa + mD*sb,
                         mA*sc + mC*sd,      mB*sc + mD*sd,
                         mA*se + mC*sf + mE, mB*se + mD*sf + mF);
}


NS_IMETHODIMP nsSVGMatrix::Inverse(nsIDOMSVGMatrix **_retval)
{
  double det = mA*mD - mC*mB;
  if (det == 0.0)
    return NS_ERROR_DOM_SVG_MATRIX_NOT_INVERTABLE;

  return NS_NewSVGMatrix(_retval,
                         (float)( mD/det),             (float)(-mB/det),
                         (float)(-mC/det),             (float)( mA/det),
                         (float)((mC*mF - mE*mD)/det), (float)((mE*mB - mA*mF)/det));
}


NS_IMETHODIMP nsSVGMatrix::Translate(float x, float y, nsIDOMSVGMatrix **_retval)
{
  return NS_NewSVGMatrix(_retval,
                         mA,               mB,
                         mC,               mD,
                         mA*x + mC*y + mE, mB*x + mD*y + mF);
}


NS_IMETHODIMP nsSVGMatrix::Scale(float scaleFactor, nsIDOMSVGMatrix **_retval)
{
  return NS_NewSVGMatrix(_retval,
                         mA*scaleFactor, mB*scaleFactor,
                         mC*scaleFactor, mD*scaleFactor,
                         mE,             mF);  
}


NS_IMETHODIMP nsSVGMatrix::ScaleNonUniform(float scaleFactorX, float scaleFactorY, nsIDOMSVGMatrix **_retval)
{
  return NS_NewSVGMatrix(_retval,
                         mA*scaleFactorX, mB*scaleFactorX,
                         mC*scaleFactorY, mD*scaleFactorY,
                         mE,              mF);  
}


NS_IMETHODIMP nsSVGMatrix::Rotate(float angle, nsIDOMSVGMatrix **_retval)
{
  return RotateRadians(angle*radPerDegree, _retval);
}


NS_IMETHODIMP nsSVGMatrix::RotateFromVector(float x, float y, nsIDOMSVGMatrix **_retval)
{
  if (x == 0.0 || y == 0.0)
    return NS_ERROR_DOM_SVG_INVALID_VALUE_ERR;

  double rad = atan2(y, x);

  return RotateRadians(rad, _retval);
}


NS_IMETHODIMP nsSVGMatrix::FlipX(nsIDOMSVGMatrix **_retval)
{
  return NS_NewSVGMatrix(_retval,
                          -mA, -mB,
                           mC,  mD,
                           mE,  mF);
}


NS_IMETHODIMP nsSVGMatrix::FlipY(nsIDOMSVGMatrix **_retval)
{
  return NS_NewSVGMatrix(_retval,
                           mA,  mB,
                          -mC, -mD,
                           mE,  mF);
}


NS_IMETHODIMP nsSVGMatrix::SkewX(float angle, nsIDOMSVGMatrix **_retval)
{
  double ta = tan( angle*radPerDegree );

  return NS_NewSVGMatrix(_retval,
                         mA,                    mB,
                         (float) ( mC + mA*ta), (float) ( mD + mB*ta),
                         mE,                    mF);
}


NS_IMETHODIMP nsSVGMatrix::SkewY(float angle, nsIDOMSVGMatrix **_retval)
{
  double ta = tan( angle*radPerDegree );

  return NS_NewSVGMatrix(_retval,
                         (float) (mA + mC*ta), (float) (mB + mD*ta),
                         mC,                    mD,
                         mE,                    mF);
}




NS_IMETHODIMP
nsSVGMatrix::SetValueString(const nsAString& aValue)
{
  NS_NOTYETIMPLEMENTED("nsSVGMatrix::SetValueString");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSVGMatrix::GetValueString(nsAString& aValue)
{
  NS_NOTYETIMPLEMENTED("nsSVGMatrix::GetValueString");
  return NS_ERROR_NOT_IMPLEMENTED;
}




nsresult
nsSVGMatrix::RotateRadians(float rad, nsIDOMSVGMatrix **_retval)
{
  double ca = cos(rad);
  double sa = sin(rad);

  return NS_NewSVGMatrix(_retval,
                         (float) (mA*ca + mC*sa), (float) (mB*ca + mD*sa),
                         (float) (mC*ca - mA*sa), (float) (mD*ca - mB*sa),
                         mE,                      mF);
}
