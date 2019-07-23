





































#include "nsSVGTransform.h"
#include "prdtoa.h"
#include "nsSVGMatrix.h"
#include "nsISVGValueUtils.h"
#include "nsWeakReference.h"
#include "nsSVGMatrix.h"
#include "nsTextFormatter.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"




nsresult
nsSVGTransform::Create(nsIDOMSVGTransform** aResult)
{
  nsSVGTransform *pl = new nsSVGTransform();
  NS_ENSURE_TRUE(pl, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(pl);
  if (NS_FAILED(pl->Init())) {
    NS_RELEASE(pl);
    *aResult = nsnull;
    return NS_ERROR_FAILURE;
  }
  *aResult = pl;
  return NS_OK;
}


nsSVGTransform::nsSVGTransform()
    : mAngle(0.0f),
      mOriginX(0.0f),
      mOriginY(0.0f),
      mType( SVG_TRANSFORM_MATRIX )
{
}

nsSVGTransform::~nsSVGTransform()
{
  NS_REMOVE_SVGVALUE_OBSERVER(mMatrix);
}

nsresult nsSVGTransform::Init()
{
  nsresult rv = NS_NewSVGMatrix(getter_AddRefs(mMatrix));
  NS_ADD_SVGVALUE_OBSERVER(mMatrix);
  return rv;
}




NS_IMPL_ADDREF(nsSVGTransform)
NS_IMPL_RELEASE(nsSVGTransform)

NS_INTERFACE_MAP_BEGIN(nsSVGTransform)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransform)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTransform)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSVGTransform::SetValueString(const nsAString& aValue)
{
  NS_NOTYETIMPLEMENTED("nsSVGTransform::SetValueString");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSVGTransform::GetValueString(nsAString& aValue)
{
  PRUnichar buf[256];
  
  switch (mType) {
    case nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE:
      {
        float dx, dy;
        mMatrix->GetE(&dx);
        mMatrix->GetF(&dy);
        if (dy != 0.0f)
          nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar), NS_LITERAL_STRING("translate(%g, %g)").get(), dx, dy);
        else
          nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar), NS_LITERAL_STRING("translate(%g)").get(), dx);
      }
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE:
      {
        if (mOriginX != 0.0f || mOriginY != 0.0f)
          nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                    NS_LITERAL_STRING("rotate(%g, %g, %g)").get(),
                                    mAngle, mOriginX, mOriginY);
        else
          nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                    NS_LITERAL_STRING("rotate(%g)").get(), mAngle);
      }
      break;        
    case nsIDOMSVGTransform::SVG_TRANSFORM_SCALE:
      {
        float sx, sy;
        mMatrix->GetA(&sx);
        mMatrix->GetD(&sy);
        if (sy != 0.0f)
          nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                    NS_LITERAL_STRING("scale(%g, %g)").get(), sx, sy);
        else
          nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                    NS_LITERAL_STRING("scale(%g)").get(), sx);
      }
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX:
      {
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                  NS_LITERAL_STRING("skewX(%g)").get(), mAngle);
      }
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY:
      {
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                  NS_LITERAL_STRING("skewY(%g)").get(), mAngle);
      }
      break;
    case nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX:
      {
        float a,b,c,d,e,f;
        mMatrix->GetA(&a);
        mMatrix->GetB(&b);
        mMatrix->GetC(&c);
        mMatrix->GetD(&d);
        mMatrix->GetE(&e);
        mMatrix->GetF(&f);
        nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                                  NS_LITERAL_STRING("matrix(%g, %g, %g, %g, %g, %g)").get(),
                                  a, b, c, d, e, f);
      }
      break;
    default:
      buf[0] = '\0';
      NS_ERROR("unknown transformation type");
      break;
  }

  aValue.Assign(buf);
  
  return NS_OK;
}





NS_IMETHODIMP nsSVGTransform::WillModifySVGObservable(nsISVGValue* observable,
                                                      modificationType aModType)
{
  WillModify();
  return NS_OK;
}

NS_IMETHODIMP nsSVGTransform::DidModifySVGObservable (nsISVGValue* observable,
                                                      modificationType aModType)
{
  DidModify();
  return NS_OK;
}






NS_IMETHODIMP nsSVGTransform::GetType(PRUint16 *aType)
{
  *aType = mType;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::GetMatrix(nsIDOMSVGMatrix * *aMatrix)
{
  
  
  *aMatrix = mMatrix;
  NS_IF_ADDREF(*aMatrix);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::GetAngle(float *aAngle)
{
  *aAngle = mAngle;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::SetMatrix(nsIDOMSVGMatrix *matrix)
{
  if (!matrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  WillModify();

  mType = SVG_TRANSFORM_MATRIX;
  mAngle = 0.0f;
  mOriginX = 0.0f;
  mOriginY = 0.0f;
  
  
  NS_REMOVE_SVGVALUE_OBSERVER(mMatrix);
  mMatrix = matrix;
  NS_ADD_SVGVALUE_OBSERVER(mMatrix);

  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::SetTranslate(float tx, float ty)
{
  NS_ENSURE_FINITE2(tx, ty, NS_ERROR_ILLEGAL_VALUE);

  WillModify();
  
  mType = SVG_TRANSFORM_TRANSLATE;
  mAngle = 0.0f;
  mOriginX = 0.0f;
  mOriginY = 0.0f;
  mMatrix->SetA(1.0f);
  mMatrix->SetB(0.0f);
  mMatrix->SetC(0.0f);
  mMatrix->SetD(1.0f);
  mMatrix->SetE(tx);
  mMatrix->SetF(ty);

  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::SetScale(float sx, float sy)
{
  NS_ENSURE_FINITE2(sx, sy, NS_ERROR_ILLEGAL_VALUE);

  WillModify();
  
  mType = SVG_TRANSFORM_SCALE;
  mAngle = 0.0f;
  mOriginX = 0.0f;
  mOriginY = 0.0f;
  mMatrix->SetA(sx);
  mMatrix->SetB(0.0f);
  mMatrix->SetC(0.0f);
  mMatrix->SetD(sy);
  mMatrix->SetE(0.0f);
  mMatrix->SetF(0.0f);

  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::SetRotate(float angle, float cx, float cy)
{
  NS_ENSURE_FINITE3(angle, cx, cy, NS_ERROR_ILLEGAL_VALUE);

  WillModify();
  
  mType = SVG_TRANSFORM_ROTATE;
  mAngle = angle;
  mOriginX = cx;
  mOriginY = cy;

  NS_REMOVE_SVGVALUE_OBSERVER(mMatrix);
  NS_NewSVGMatrix(getter_AddRefs(mMatrix));
  nsCOMPtr<nsIDOMSVGMatrix> temp;
  mMatrix->Translate(cx, cy, getter_AddRefs(temp));
  mMatrix = temp;
  mMatrix->Rotate(angle, getter_AddRefs(temp));
  mMatrix = temp;
  mMatrix->Translate(-cx,-cy, getter_AddRefs(temp));
  mMatrix = temp;
  NS_ADD_SVGVALUE_OBSERVER(mMatrix);

  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::SetSkewX(float angle)
{
  NS_ENSURE_FINITE(angle, NS_ERROR_ILLEGAL_VALUE);

  WillModify();
  
  mType = SVG_TRANSFORM_SKEWX;
  mAngle = angle;

  NS_REMOVE_SVGVALUE_OBSERVER(mMatrix);
  NS_NewSVGMatrix(getter_AddRefs(mMatrix));
  nsCOMPtr<nsIDOMSVGMatrix> temp;
  mMatrix->SkewX(angle, getter_AddRefs(temp));
  mMatrix = temp;
  NS_ADD_SVGVALUE_OBSERVER(mMatrix);

  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTransform::SetSkewY(float angle)
{
  NS_ENSURE_FINITE(angle, NS_ERROR_ILLEGAL_VALUE);

  WillModify();
  
  mType = SVG_TRANSFORM_SKEWY;
  mAngle = angle;

  NS_REMOVE_SVGVALUE_OBSERVER(mMatrix);
  NS_NewSVGMatrix(getter_AddRefs(mMatrix));
  nsCOMPtr<nsIDOMSVGMatrix> temp;
  mMatrix->SkewY(angle, getter_AddRefs(temp));
  mMatrix = temp;
  NS_ADD_SVGVALUE_OBSERVER(mMatrix);

  DidModify();
  return NS_OK;
}






nsresult
NS_NewSVGTransform(nsIDOMSVGTransform** result)
{
  return nsSVGTransform::Create(result);
}
