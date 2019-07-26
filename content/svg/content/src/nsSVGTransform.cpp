





#include "nsError.h"
#include "nsSVGTransform.h"
#include "nsContentUtils.h" 
#include "nsTextFormatter.h"

namespace {
  const double radPerDegree = 2.0 * M_PI / 360.0;
}

namespace mozilla {

void
nsSVGTransform::GetValueAsString(nsAString& aValue) const
{
  PRUnichar buf[256];

  switch (mType) {
    case SVG_TRANSFORM_TRANSLATE:
      
      if (mMatrix.y0 != 0)
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            MOZ_UTF16("translate(%g, %g)"),
            mMatrix.x0, mMatrix.y0);
      else
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            MOZ_UTF16("translate(%g)"),
            mMatrix.x0);
      break;
    case SVG_TRANSFORM_ROTATE:
      if (mOriginX != 0.0f || mOriginY != 0.0f)
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            MOZ_UTF16("rotate(%g, %g, %g)"),
            mAngle, mOriginX, mOriginY);
      else
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            MOZ_UTF16("rotate(%g)"), mAngle);
      break;
    case SVG_TRANSFORM_SCALE:
      if (mMatrix.xx != mMatrix.yy)
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            MOZ_UTF16("scale(%g, %g)"), mMatrix.xx, mMatrix.yy);
      else
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
            MOZ_UTF16("scale(%g)"), mMatrix.xx);
      break;
    case SVG_TRANSFORM_SKEWX:
      nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                MOZ_UTF16("skewX(%g)"), mAngle);
      break;
    case SVG_TRANSFORM_SKEWY:
      nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                MOZ_UTF16("skewY(%g)"), mAngle);
      break;
    case SVG_TRANSFORM_MATRIX:
      nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
          MOZ_UTF16("matrix(%g, %g, %g, %g, %g, %g)"),
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
nsSVGTransform::SetMatrix(const gfxMatrix& aMatrix)
{
  mType    = SVG_TRANSFORM_MATRIX;
  mMatrix  = aMatrix;
  
  
  mAngle   = 0.f;
  mOriginX = 0.f;
  mOriginY = 0.f;
}

void
nsSVGTransform::SetTranslate(float aTx, float aTy)
{
  mType    = SVG_TRANSFORM_TRANSLATE;
  mMatrix.Reset();
  mMatrix.x0 = aTx;
  mMatrix.y0 = aTy;
  mAngle   = 0.f;
  mOriginX = 0.f;
  mOriginY = 0.f;
}

void
nsSVGTransform::SetScale(float aSx, float aSy)
{
  mType    = SVG_TRANSFORM_SCALE;
  mMatrix.Reset();
  mMatrix.xx = aSx;
  mMatrix.yy = aSy;
  mAngle   = 0.f;
  mOriginX = 0.f;
  mOriginY = 0.f;
}

void
nsSVGTransform::SetRotate(float aAngle, float aCx, float aCy)
{
  mType    = SVG_TRANSFORM_ROTATE;
  mMatrix.Reset();
  mMatrix.Translate(gfxPoint(aCx, aCy));
  mMatrix.Rotate(aAngle*radPerDegree);
  mMatrix.Translate(gfxPoint(-aCx, -aCy));
  mAngle   = aAngle;
  mOriginX = aCx;
  mOriginY = aCy;
}

nsresult
nsSVGTransform::SetSkewX(float aAngle)
{
  double ta = tan(aAngle*radPerDegree);
  NS_ENSURE_FINITE(ta, NS_ERROR_RANGE_ERR);

  mType    = SVG_TRANSFORM_SKEWX;
  mMatrix.Reset();
  mMatrix.xy = ta;
  mAngle   = aAngle;
  mOriginX = 0.f;
  mOriginY = 0.f;
  return NS_OK;
}

nsresult
nsSVGTransform::SetSkewY(float aAngle)
{
  double ta = tan(aAngle*radPerDegree);
  NS_ENSURE_FINITE(ta, NS_ERROR_RANGE_ERR);

  mType    = SVG_TRANSFORM_SKEWY;
  mMatrix.Reset();
  mMatrix.yx = ta;
  mAngle   = aAngle;
  mOriginX = 0.f;
  mOriginY = 0.f;
  return NS_OK;
}

SVGTransformSMILData::SVGTransformSMILData(const nsSVGTransform& aTransform)
  : mTransformType(aTransform.Type())
{
  NS_ABORT_IF_FALSE(
    mTransformType >= SVG_TRANSFORM_MATRIX &&
    mTransformType <= SVG_TRANSFORM_SKEWY,
    "Unexpected transform type");

  for (uint32_t i = 0; i < NUM_STORED_PARAMS; ++i) {
    mParams[i] = 0.f;
  }

  switch (mTransformType) {
    case SVG_TRANSFORM_MATRIX: {
      const gfxMatrix& mx = aTransform.Matrix();
      mParams[0] = static_cast<float>(mx.xx);
      mParams[1] = static_cast<float>(mx.yx);
      mParams[2] = static_cast<float>(mx.xy);
      mParams[3] = static_cast<float>(mx.yy);
      mParams[4] = static_cast<float>(mx.x0);
      mParams[5] = static_cast<float>(mx.y0);
      break;
    }
    case SVG_TRANSFORM_TRANSLATE: {
      const gfxMatrix& mx = aTransform.Matrix();
      mParams[0] = static_cast<float>(mx.x0);
      mParams[1] = static_cast<float>(mx.y0);
      break;
    }
    case SVG_TRANSFORM_SCALE: {
      const gfxMatrix& mx = aTransform.Matrix();
      mParams[0] = static_cast<float>(mx.xx);
      mParams[1] = static_cast<float>(mx.yy);
      break;
    }
    case SVG_TRANSFORM_ROTATE:
      mParams[0] = aTransform.Angle();
      aTransform.GetRotationOrigin(mParams[1], mParams[2]);
      break;

    case SVG_TRANSFORM_SKEWX:
    case SVG_TRANSFORM_SKEWY:
      mParams[0] = aTransform.Angle();
      break;

    default:
      NS_NOTREACHED("Unexpected transform type");
      break;
  }
}

nsSVGTransform
SVGTransformSMILData::ToSVGTransform() const
{
  nsSVGTransform result;

  switch (mTransformType) {
    case SVG_TRANSFORM_MATRIX:
      result.SetMatrix(gfxMatrix(mParams[0], mParams[1],
                                 mParams[2], mParams[3],
                                 mParams[4], mParams[5]));
      break;

    case SVG_TRANSFORM_TRANSLATE:
      result.SetTranslate(mParams[0], mParams[1]);
      break;

    case SVG_TRANSFORM_SCALE:
      result.SetScale(mParams[0], mParams[1]);
      break;

    case SVG_TRANSFORM_ROTATE:
      result.SetRotate(mParams[0], mParams[1], mParams[2]);
      break;

    case SVG_TRANSFORM_SKEWX:
      result.SetSkewX(mParams[0]);
      break;

    case SVG_TRANSFORM_SKEWY:
      result.SetSkewY(mParams[0]);
      break;

    default:
      NS_NOTREACHED("Unexpected transform type");
      break;
  }
  return result;
}

} 
