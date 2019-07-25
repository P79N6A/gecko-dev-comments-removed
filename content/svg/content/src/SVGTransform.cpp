





































#include "SVGTransform.h"
#include "nsContentUtils.h"
#include "nsTextFormatter.h"

namespace {
  const double radPerDegree = 2.0*3.1415926535 / 360.0;
}

namespace mozilla {

void
SVGTransform::GetValueAsString(nsAString& aValue) const
{
  PRUnichar buf[256];

  switch (mType) {
    case nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE:
      if (mMatrix.x0 != 0.0f)
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            NS_LITERAL_STRING("translate(%g, %g)").get(),
            mMatrix.x0, mMatrix.y0);
      else
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            NS_LITERAL_STRING("translate(%g)").get(),
            mMatrix.x0);
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE:
      if (mOriginX != 0.0f || mOriginY != 0.0f)
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            NS_LITERAL_STRING("rotate(%g, %g, %g)").get(),
            mAngle, mOriginX, mOriginY);
      else
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            NS_LITERAL_STRING("rotate(%g)").get(), mAngle);
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_SCALE:
      if (mMatrix.xx != mMatrix.yy)
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            NS_LITERAL_STRING("scale(%g, %g)").get(), mMatrix.xx, mMatrix.yy);
      else
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            NS_LITERAL_STRING("scale(%g)").get(), mMatrix.xx);
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX:
      nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                NS_LITERAL_STRING("skewX(%g)").get(), mAngle);
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY:
      nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                NS_LITERAL_STRING("skewY(%g)").get(), mAngle);
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX:
      nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
          NS_LITERAL_STRING("matrix(%g, %g, %g, %g, %g, %g)").get(),
                            mMatrix.xx, mMatrix.yx,
                            mMatrix.xy, mMatrix.yy,
                            mMatrix.x0, mMatrix.y0);
      break;
    default:
      buf[0] = '\0';
      NS_ERROR("unknown transformation type");
      break;
  }

  aValue.Assign(buf);
}

void
SVGTransform::SetMatrix(const gfxMatrix& aMatrix)
{
  mType    = nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX;
  mMatrix  = aMatrix;
  
  
  mAngle   = 0.f;
  mOriginX = 0.f;
  mOriginY = 0.f;
}

void
SVGTransform::SetTranslate(float aTx, float aTy)
{
  mType    = nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE;
  mMatrix.Reset();
  mMatrix.x0 = aTx;
  mMatrix.y0 = aTy;
  mAngle   = 0.f;
  mOriginX = 0.f;
  mOriginY = 0.f;
}

void
SVGTransform::SetScale(float aSx, float aSy)
{
  mType    = nsIDOMSVGTransform::SVG_TRANSFORM_SCALE;
  mMatrix.Reset();
  mMatrix.xx = aSx;
  mMatrix.yy = aSy;
  mAngle   = 0.f;
  mOriginX = 0.f;
  mOriginY = 0.f;
}

void
SVGTransform::SetRotate(float aAngle, float aCx, float aCy)
{
  mType    = nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE;
  mMatrix.Reset();
  mMatrix.Translate(gfxPoint(aCx, aCy));
  mMatrix.Rotate(aAngle*radPerDegree);
  mMatrix.Translate(gfxPoint(-aCx, -aCy));
  mAngle   = aAngle;
  mOriginX = aCx;
  mOriginY = aCy;
}

nsresult
SVGTransform::SetSkewX(float aAngle)
{
  double ta = tan(aAngle*radPerDegree);
  NS_ENSURE_FINITE(ta, NS_ERROR_DOM_SVG_INVALID_VALUE_ERR);

  mType    = nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX;
  mMatrix.Reset();
  mMatrix.xy = ta;
  mAngle   = aAngle;
  mOriginX = 0.f;
  mOriginY = 0.f;
  return NS_OK;
}

nsresult
SVGTransform::SetSkewY(float aAngle)
{
  double ta = tan(aAngle*radPerDegree);
  NS_ENSURE_FINITE(ta, NS_ERROR_DOM_SVG_INVALID_VALUE_ERR);

  mType    = nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY;
  mMatrix.Reset();
  mMatrix.yx = ta;
  mAngle   = aAngle;
  mOriginX = 0.f;
  mOriginY = 0.f;
  return NS_OK;
}

} 
