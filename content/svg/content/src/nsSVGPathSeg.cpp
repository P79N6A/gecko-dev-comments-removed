





































#include "nsSVGPathSeg.h"
#include "prdtoa.h"
#include "nsSVGValue.h"
#include "nsTextFormatter.h"
#include "nsContentUtils.h"


struct PathPoint {
  float x, y;
};



static const float PATH_SEG_LENGTH_TOLERANCE = 0.0000001f;




#define NS_IMPL_NSISUPPORTS_SVGPATHSEG(basename)              \
NS_IMPL_ADDREF(ns##basename)                                  \
NS_IMPL_RELEASE(ns##basename)                                 \
                                                              \
NS_INTERFACE_MAP_BEGIN(ns##basename)                          \
  NS_INTERFACE_MAP_ENTRY(nsIDOM##basename)                    \
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(basename)          \
NS_INTERFACE_MAP_END_INHERITING(nsSVGPathSeg)




static float GetDistance(float x1, float x2, float y1, float y2)
{
  return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

static void SplitQuadraticBezier(PathPoint *curve,
                                 PathPoint *left,
                                 PathPoint *right)
{
  left[0].x = curve[0].x;
  left[0].y = curve[0].y;
  right[2].x = curve[2].x;
  right[2].y = curve[2].y;
  left[1].x = (curve[0].x + curve[1].x) / 2;
  left[1].y = (curve[0].y + curve[1].y) / 2;
  right[1].x = (curve[1].x + curve[2].x) / 2;
  right[1].y = (curve[1].y + curve[2].y) / 2;
  left[2].x = right[0].x = (left[1].x + right[1].x) / 2;
  left[2].y = right[0].y = (left[1].y + right[1].y) / 2;
}

static void SplitCubicBezier(PathPoint *curve,
                             PathPoint *left,
                             PathPoint *right)
{
  PathPoint tmp;
  tmp.x = (curve[1].x + curve[2].x) / 4;
  tmp.y = (curve[1].y + curve[2].y) / 4;
  left[0].x = curve[0].x;
  left[0].y = curve[0].y;
  right[3].x = curve[3].x;
  right[3].y = curve[3].y;
  left[1].x = (curve[0].x + curve[1].x) / 2;
  left[1].y = (curve[0].y + curve[1].y) / 2;
  right[2].x = (curve[2].x + curve[3].x) / 2;
  right[2].y = (curve[2].y + curve[3].y) / 2;
  left[2].x = left[1].x / 2 + tmp.x;
  left[2].y = left[1].y / 2 + tmp.y;
  right[1].x = right[2].x / 2 + tmp.x;
  right[1].y = right[2].y / 2 + tmp.y;
  left[3].x = right[0].x = (left[2].x + right[1].x) / 2;
  left[3].y = right[0].y = (left[2].y + right[1].y) / 2;
}

static float CalcBezLength(PathPoint *curve,PRUint32 numPts,
                           void (*split)(PathPoint*, PathPoint*, PathPoint*))
{
  PathPoint left[4];
  PathPoint right[4];
  float length = 0, dist;
  PRUint32 i;
  for (i = 0; i < numPts - 1; i++) {
    length += GetDistance(curve[i].x, curve[i+1].x, curve[i].y, curve[i+1].y);
  }
  dist = GetDistance(curve[0].x, curve[numPts - 1].x,
                     curve[0].y, curve[numPts - 1].y);
  if (length - dist > PATH_SEG_LENGTH_TOLERANCE) {
      split(curve, left, right);
      return CalcBezLength(left, numPts, split) + 
             CalcBezLength(right, numPts, split);
  }
  return length;
}




char nsSVGPathSeg::mTypeLetters[] = {
  'X', 'z', 'M', 'm', 'L', 'l', 'C', 'c', 'S', 's',
  'A', 'a', 'H', 'h', 'V', 'v', 'Q', 'q', 'T', 't'
};

nsQueryReferent
nsSVGPathSeg::GetCurrentList() const
{
  return do_QueryReferent(mCurrentList);
}

nsresult 
nsSVGPathSeg::SetCurrentList(nsISVGValue* aList)
{
  if (!aList) {
    mCurrentList = nsnull;
    return NS_OK;
  }
  nsresult rv;
  mCurrentList = do_GetWeakReference(aList, &rv);
  return rv;
}

NS_IMETHODIMP
nsSVGPathSeg::GetPathSegType(PRUint16 *aPathSegType)
{
  *aPathSegType = GetSegType();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGPathSeg::GetPathSegTypeAsLetter(nsAString & aPathSegTypeAsLetter)
{
  aPathSegTypeAsLetter.AssignASCII(&mTypeLetters[GetSegType()], 1);
  return NS_OK;
}

void
nsSVGPathSeg::DidModify(void)
{
  if (mCurrentList) {
    nsCOMPtr<nsISVGValue> val = do_QueryReferent(mCurrentList);
    if (val) {
      val->BeginBatchUpdate();
      val->EndBatchUpdate();
    }
  }
}




NS_IMPL_ADDREF(nsSVGPathSeg)
NS_IMPL_RELEASE(nsSVGPathSeg)

NS_INTERFACE_MAP_BEGIN(nsSVGPathSeg)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPathSeg)
  NS_INTERFACE_MAP_ENTRY(nsSVGPathSeg)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




class nsSVGPathSegClosePath : public nsIDOMSVGPathSegClosePath,
                              public nsSVGPathSeg
{
public:

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCLOSEPATH

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CLOSEPATH; }
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegClosePath()
{
  return new nsSVGPathSegClosePath();
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegClosePath)



NS_IMETHODIMP
nsSVGPathSegClosePath::GetValueString(nsAString& aValue)
{
  aValue.AssignLiteral("z");
  return NS_OK;
}

float
nsSVGPathSegClosePath::GetLength(nsSVGPathSegTraversalState *ts)
{
  float dist = GetDistance(ts->startPosX, ts->curPosX,
                           ts->startPosY, ts->curPosY);
  ts->quadCPX = ts->cubicCPX = ts->curPosX = ts->startPosX;
  ts->quadCPY = ts->cubicCPY = ts->curPosY = ts->startPosY;
  return dist;
}




class nsSVGPathSegMovetoAbs : public nsIDOMSVGPathSegMovetoAbs,
                              public nsSVGPathSeg
{
public:
  nsSVGPathSegMovetoAbs(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGMOVETOABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_MOVETO_ABS; }

protected:
  float mX, mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegMovetoAbs(float x, float y)
{
  return new nsSVGPathSegMovetoAbs(x, y);
}

nsSVGPathSegMovetoAbs::nsSVGPathSegMovetoAbs(float x, float y)
    : mX(x), mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegMovetoAbs)



NS_IMETHODIMP
nsSVGPathSegMovetoAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[48];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("M%g,%g").get(), mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegMovetoAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->cubicCPX = ts->quadCPX = ts->startPosX = ts->curPosX = mX;
  ts->cubicCPY = ts->quadCPY = ts->startPosY = ts->curPosY = mY;
  return 0;
}





NS_IMETHODIMP nsSVGPathSegMovetoAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegMovetoAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegMovetoAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegMovetoAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}





class nsSVGPathSegMovetoRel : public nsIDOMSVGPathSegMovetoRel,
                              public nsSVGPathSeg
{
public:
  nsSVGPathSegMovetoRel(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGMOVETOREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_MOVETO_REL; }

protected:
  float mX, mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegMovetoRel(float x, float y)
{
  return new nsSVGPathSegMovetoRel(x, y);
}

nsSVGPathSegMovetoRel::nsSVGPathSegMovetoRel(float x, float y)
    : mX(x), mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegMovetoRel)



NS_IMETHODIMP
nsSVGPathSegMovetoRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[48];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("m%g,%g").get(), mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegMovetoRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->cubicCPX = ts->quadCPX = ts->startPosX = ts->curPosX += mX;
  ts->cubicCPY = ts->quadCPY = ts->startPosY = ts->curPosY += mY;
  return 0;
}





NS_IMETHODIMP nsSVGPathSegMovetoRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegMovetoRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegMovetoRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegMovetoRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegLinetoAbs : public nsIDOMSVGPathSegLinetoAbs,
                              public nsSVGPathSeg
{
public:
  nsSVGPathSegLinetoAbs(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLINETOABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_LINETO_ABS; }

protected:
  float mX, mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoAbs(float x, float y)
{
  return new nsSVGPathSegLinetoAbs(x, y);
}

nsSVGPathSegLinetoAbs::nsSVGPathSegLinetoAbs(float x, float y)
    : mX(x), mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegLinetoAbs)



NS_IMETHODIMP
nsSVGPathSegLinetoAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[48];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("L%g,%g").get(), mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegLinetoAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  
  float dist = GetDistance(ts->curPosX, mX, ts->curPosY, mY);
  ts->cubicCPX = ts->quadCPX = ts->curPosX = mX;
  ts->cubicCPY = ts->quadCPY = ts->curPosY = mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegLinetoAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegLinetoAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}





class nsSVGPathSegLinetoRel : public nsIDOMSVGPathSegLinetoRel,
                              public nsSVGPathSeg
{
public:
  nsSVGPathSegLinetoRel(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLINETOREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_LINETO_REL; }

protected:
  float mX, mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoRel(float x, float y)
{
  return new nsSVGPathSegLinetoRel(x, y);
}

nsSVGPathSegLinetoRel::nsSVGPathSegLinetoRel(float x, float y)
    : mX(x), mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegLinetoRel)



NS_IMETHODIMP
nsSVGPathSegLinetoRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[48];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("l%g,%g").get(), mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegLinetoRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->cubicCPX = ts->quadCPX = ts->curPosX += mX;
  ts->cubicCPY = ts->quadCPY = ts->curPosY += mY;
  return sqrt(mX * mX + mY * mY);
}





NS_IMETHODIMP nsSVGPathSegLinetoRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegLinetoRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}





class nsSVGPathSegCurvetoCubicAbs : public nsIDOMSVGPathSegCurvetoCubicAbs,
                                    public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoCubicAbs(float x, float y,
                              float x1, float y1,
                              float x2, float y2);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOCUBICABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_ABS; }

protected:
  float mX, mY, mX1, mY1, mX2, mY2;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicAbs(float x, float y,
                                float x1, float y1,
                                float x2, float y2)
{
  return new nsSVGPathSegCurvetoCubicAbs(x, y, x1, y1, x2, y2);
}

nsSVGPathSegCurvetoCubicAbs::nsSVGPathSegCurvetoCubicAbs(float x, float y,
                                                         float x1, float y1,
                                                         float x2, float y2)
    : mX(x), mY(y), mX1(x1), mY1(y1), mX2(x2), mY2(y2)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoCubicAbs)



NS_IMETHODIMP
nsSVGPathSegCurvetoCubicAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[144];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("C%g,%g %g,%g %g,%g").get(), 
                            mX1, mY1, mX2, mY2, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoCubicAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  PathPoint curve[4] = { {ts->curPosX, ts->curPosY},
                         {mX1, mY1},
                         {mX2, mY2},
                         {mX, mY} };
  float dist = CalcBezLength(curve, 4, SplitCubicBezier);
  
  ts->quadCPX = ts->curPosX = mX;
  ts->quadCPY = ts->curPosY = mY;
  ts->cubicCPX = mX2;
  ts->cubicCPY = mY2;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::GetX1(float *aX1)
{
  *aX1 = mX1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::SetX1(float aX1)
{
  mX1 = aX1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::GetY1(float *aY1)
{
  *aY1 = mY1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::SetY1(float aY1)
{
  mY1 = aY1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::GetX2(float *aX2)
{
  *aX2 = mX2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::SetX2(float aX2)
{
  mX2 = aX2;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::GetY2(float *aY2)
{
  *aY2 = mY2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicAbs::SetY2(float aY2)
{
  mY2 = aY2;
  DidModify();
  return NS_OK;
}





class nsSVGPathSegCurvetoCubicRel : public nsIDOMSVGPathSegCurvetoCubicRel,
                                    public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoCubicRel(float x, float y,
                              float x1, float y1,
                              float x2, float y2);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOCUBICREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_REL; }

protected:
  float mX, mY, mX1, mY1, mX2, mY2;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicRel(float x, float y,
                                float x1, float y1,
                                float x2, float y2)
{
  return new nsSVGPathSegCurvetoCubicRel(x, y, x1, y1, x2, y2);
}

nsSVGPathSegCurvetoCubicRel::nsSVGPathSegCurvetoCubicRel(float x, float y,
                                                         float x1, float y1,
                                                         float x2, float y2)
    : mX(x), mY(y), mX1(x1), mY1(y1), mX2(x2), mY2(y2)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoCubicRel)



NS_IMETHODIMP
nsSVGPathSegCurvetoCubicRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[144];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("c%g,%g %g,%g %g,%g").get(),
                            mX1, mY1, mX2, mY2, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoCubicRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  PathPoint curve[4] = { {0, 0}, {mX1, mY1}, {mX2, mY2}, {mX, mY} };
  float dist = CalcBezLength(curve, 4, SplitCubicBezier);
  
  ts->cubicCPX = mX2 + ts->curPosX;
  ts->cubicCPY = mY2 + ts->curPosY;
  ts->quadCPX = ts->curPosX += mX;
  ts->quadCPY = ts->curPosY += mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::GetX1(float *aX1)
{
  *aX1 = mX1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::SetX1(float aX1)
{
  mX1 = aX1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::GetY1(float *aY1)
{
  *aY1 = mY1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::SetY1(float aY1)
{
  mY1 = aY1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::GetX2(float *aX2)
{
  *aX2 = mX2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::SetX2(float aX2)
{
  mX2 = aX2;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::GetY2(float *aY2)
{
  *aY2 = mY2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicRel::SetY2(float aY2)
{
  mY2 = aY2;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegCurvetoQuadraticAbs : public nsIDOMSVGPathSegCurvetoQuadraticAbs,
                                        public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoQuadraticAbs(float x, float y,
                                  float x1, float y1);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOQUADRATICABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_ABS; }

protected:
  float mX, mY, mX1, mY1;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticAbs(float x, float y,
                                    float x1, float y1)
{
  return new nsSVGPathSegCurvetoQuadraticAbs(x, y, x1, y1);
}


nsSVGPathSegCurvetoQuadraticAbs::nsSVGPathSegCurvetoQuadraticAbs(float x, float y,
                                                                 float x1, float y1)
    : mX(x), mY(y), mX1(x1), mY1(y1)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoQuadraticAbs)



NS_IMETHODIMP
nsSVGPathSegCurvetoQuadraticAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[96];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("Q%g,%g %g,%g").get(),
                            mX1, mY1, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoQuadraticAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  PathPoint curve[3] = { {ts->curPosX, ts->curPosY}, {mX1, mY1}, {mX, mY} };
  float dist = CalcBezLength(curve, 3, SplitQuadraticBezier);
  
  ts->quadCPX = mX1;
  ts->quadCPY = mY1;
  ts->cubicCPX = ts->curPosX = mX;
  ts->cubicCPY = ts->curPosY = mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::GetX1(float *aX1)
{
  *aX1 = mX1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::SetX1(float aX1)
{
  mX1 = aX1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::GetY1(float *aY1)
{
  *aY1 = mY1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticAbs::SetY1(float aY1)
{
  mY1 = aY1;
  DidModify();
  return NS_OK;
}





class nsSVGPathSegCurvetoQuadraticRel : public nsIDOMSVGPathSegCurvetoQuadraticRel,
                                        public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoQuadraticRel(float x, float y,
                                  float x1, float y1);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOQUADRATICREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_REL; }


protected:
  float mX, mY, mX1, mY1;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticRel(float x, float y,
                                    float x1, float y1)
{
  return new nsSVGPathSegCurvetoQuadraticRel(x, y, x1, y1);
}


nsSVGPathSegCurvetoQuadraticRel::nsSVGPathSegCurvetoQuadraticRel(float x, float y,
                                                                 float x1, float y1)
    : mX(x), mY(y), mX1(x1), mY1(y1)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoQuadraticRel)



NS_IMETHODIMP
nsSVGPathSegCurvetoQuadraticRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[96];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("q%g,%g %g,%g").get(), 
                            mX1, mY1, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoQuadraticRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  PathPoint curve[3] = { {0, 0}, {mX1, mY1}, {mX, mY} };
  float dist = CalcBezLength(curve, 3, SplitQuadraticBezier);
  
  ts->quadCPX = mX1 + ts->curPosX;
  ts->quadCPY = mY1 + ts->curPosY;
  ts->cubicCPX = ts->curPosX += mX;
  ts->cubicCPY = ts->curPosY += mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::GetX1(float *aX1)
{
  *aX1 = mX1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::SetX1(float aX1)
{
  mX1 = aX1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::GetY1(float *aY1)
{
  *aY1 = mY1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticRel::SetY1(float aY1)
{
  mY1 = aY1;
  DidModify();
  return NS_OK;
}





class nsSVGPathSegArcAbs : public nsIDOMSVGPathSegArcAbs,
                           public nsSVGPathSeg
{
public:
  nsSVGPathSegArcAbs(float x, float y,
                     float r1, float r2, float angle,
                     PRBool largeArcFlag, PRBool sweepFlag);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGARCABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_ARC_ABS; }

protected:
  float  mX, mY, mR1, mR2, mAngle;
  PRPackedBool mLargeArcFlag;
  PRPackedBool mSweepFlag;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegArcAbs(float x, float y,
                       float r1, float r2, float angle,
                       PRBool largeArcFlag, PRBool sweepFlag)
{
  return new nsSVGPathSegArcAbs(x, y, r1, r2, angle, largeArcFlag, sweepFlag);
}


nsSVGPathSegArcAbs::nsSVGPathSegArcAbs(float x, float y,
                                       float r1, float r2, float angle,
                                       PRBool largeArcFlag, PRBool sweepFlag)
    : mX(x), mY(y), mR1(r1), mR2(r2), mAngle(angle),
      mLargeArcFlag(largeArcFlag), mSweepFlag(sweepFlag)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegArcAbs)



NS_IMETHODIMP
nsSVGPathSegArcAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[168];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("A%g,%g %g %d,%d %g,%g").get(), 
                            mR1, mR2, mAngle, mLargeArcFlag, mSweepFlag, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegArcAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  PathPoint bez[4] = { {ts->curPosX, ts->curPosY}, {0,0}, {0,0}, {0,0}};
  nsSVGArcConverter converter(ts->curPosX, ts->curPosY, mX, mY, mR1,
                              mR2, mAngle, mLargeArcFlag, mSweepFlag);
  float dist = 0;
  while (converter.GetNextSegment(&bez[1].x, &bez[1].y, &bez[2].x, &bez[2].y,
                                  &bez[3].x, &bez[3].y)) 
  {
    dist += CalcBezLength(bez, 4, SplitCubicBezier);
    bez[0].x = bez[3].x;
    bez[0].y = bez[3].y;
  }
  ts->cubicCPX = ts->quadCPX = ts->curPosX = mX;
  ts->cubicCPY = ts->quadCPY = ts->curPosY = mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegArcAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcAbs::GetR1(float *aR1)
{
  *aR1 = mR1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetR1(float aR1)
{
  mR1 = aR1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcAbs::GetR2(float *aR2)
{
  *aR2 = mR2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetR2(float aR2)
{
  mR2 = aR2;
  DidModify();
  return NS_OK;
}



NS_IMETHODIMP nsSVGPathSegArcAbs::GetAngle(float *aAngle)
{
  *aAngle = mAngle;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetAngle(float aAngle)
{
  mAngle = aAngle;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcAbs::GetLargeArcFlag(PRBool *aLargeArcFlag)
{
  *aLargeArcFlag = mLargeArcFlag;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetLargeArcFlag(PRBool aLargeArcFlag)
{
  mLargeArcFlag = aLargeArcFlag;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcAbs::GetSweepFlag(PRBool *aSweepFlag)
{
  *aSweepFlag = mSweepFlag;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcAbs::SetSweepFlag(PRBool aSweepFlag)
{
  mSweepFlag = aSweepFlag;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegArcRel : public nsIDOMSVGPathSegArcRel,
                           public nsSVGPathSeg
{
public:
  nsSVGPathSegArcRel(float x, float y,
                     float r1, float r2, float angle,
                     PRBool largeArcFlag, PRBool sweepFlag);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGARCREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_ARC_REL; }

protected:
  float  mX, mY, mR1, mR2, mAngle;
  PRBool mLargeArcFlag, mSweepFlag;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegArcRel(float x, float y,
                       float r1, float r2, float angle,
                       PRBool largeArcFlag, PRBool sweepFlag)
{
  return new nsSVGPathSegArcRel(x, y, r1, r2, angle, largeArcFlag, sweepFlag);
}


nsSVGPathSegArcRel::nsSVGPathSegArcRel(float x, float y,
                                       float r1, float r2, float angle,
                                       PRBool largeArcFlag, PRBool sweepFlag)
    : mX(x), mY(y), mR1(r1), mR2(r2), mAngle(angle),
      mLargeArcFlag(largeArcFlag), mSweepFlag(sweepFlag)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegArcRel)



NS_IMETHODIMP
nsSVGPathSegArcRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[168];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("a%g,%g %g %d,%d %g,%g").get(), 
                            mR1, mR2, mAngle, mLargeArcFlag, mSweepFlag, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegArcRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  PathPoint bez[4] = { {0, 0}, {0,0}, {0,0}, {0,0}};
  nsSVGArcConverter converter(0, 0, mX, mY, mR1, mR2, mAngle,
                              mLargeArcFlag, mSweepFlag);
  float dist = 0;
  while (converter.GetNextSegment(&bez[1].x, &bez[1].y, &bez[2].x, &bez[2].y,
                                  &bez[3].x, &bez[3].y)) 
  {
    dist += CalcBezLength(bez, 4, SplitCubicBezier);
    bez[0].x = bez[3].x;
    bez[0].y = bez[3].y;
  }

  ts->cubicCPX = ts->quadCPX = ts->curPosX += mX;
  ts->cubicCPY = ts->quadCPY = ts->curPosY += mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegArcRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcRel::GetR1(float *aR1)
{
  *aR1 = mR1;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetR1(float aR1)
{
  mR1 = aR1;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcRel::GetR2(float *aR2)
{
  *aR2 = mR2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetR2(float aR2)
{
  mR2 = aR2;
  DidModify();
  return NS_OK;
}



NS_IMETHODIMP nsSVGPathSegArcRel::GetAngle(float *aAngle)
{
  *aAngle = mAngle;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetAngle(float aAngle)
{
  mAngle = aAngle;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcRel::GetLargeArcFlag(PRBool *aLargeArcFlag)
{
  *aLargeArcFlag = mLargeArcFlag;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetLargeArcFlag(PRBool aLargeArcFlag)
{
  mLargeArcFlag = aLargeArcFlag;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegArcRel::GetSweepFlag(PRBool *aSweepFlag)
{
  *aSweepFlag = mSweepFlag;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegArcRel::SetSweepFlag(PRBool aSweepFlag)
{
  mSweepFlag = aSweepFlag;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegLinetoHorizontalAbs : public nsIDOMSVGPathSegLinetoHorizontalAbs,
                                        public nsSVGPathSeg
{
public:
  nsSVGPathSegLinetoHorizontalAbs(float x);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLINETOHORIZONTALABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_LINETO_HORIZONTAL_ABS; }

protected:
  float mX;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoHorizontalAbs(float x)
{
  return new nsSVGPathSegLinetoHorizontalAbs(x);
}


nsSVGPathSegLinetoHorizontalAbs::nsSVGPathSegLinetoHorizontalAbs(float x)
    : mX(x)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegLinetoHorizontalAbs)



NS_IMETHODIMP
nsSVGPathSegLinetoHorizontalAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("H%g").get(), mX);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegLinetoHorizontalAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  float dist = fabs(mX - ts->curPosX);
  ts->cubicCPX = ts->quadCPX = ts->curPosX = mX;
  ts->cubicCPY = ts->quadCPY = ts->curPosY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegLinetoHorizontalAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoHorizontalAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegLinetoHorizontalRel : public nsIDOMSVGPathSegLinetoHorizontalRel,
                                        public nsSVGPathSeg
{
public:
  nsSVGPathSegLinetoHorizontalRel(float x);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLINETOHORIZONTALREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_LINETO_HORIZONTAL_REL; }

protected:
  float mX;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoHorizontalRel(float x)
{
  return new nsSVGPathSegLinetoHorizontalRel(x);
}


nsSVGPathSegLinetoHorizontalRel::nsSVGPathSegLinetoHorizontalRel(float x)
    : mX(x)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegLinetoHorizontalRel)



NS_IMETHODIMP
nsSVGPathSegLinetoHorizontalRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("h%g").get(), mX);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegLinetoHorizontalRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->cubicCPX = ts->quadCPX = ts->curPosX += mX;
  ts->cubicCPY = ts->quadCPY = ts->curPosY;
  return fabs(mX);
}





NS_IMETHODIMP nsSVGPathSegLinetoHorizontalRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoHorizontalRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegLinetoVerticalAbs : public nsIDOMSVGPathSegLinetoVerticalAbs,
                                      public nsSVGPathSeg
{
public:
  nsSVGPathSegLinetoVerticalAbs(float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLINETOVERTICALABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_LINETO_VERTICAL_ABS; }

protected:
  float mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoVerticalAbs(float y)
{
  return new nsSVGPathSegLinetoVerticalAbs(y);
}


nsSVGPathSegLinetoVerticalAbs::nsSVGPathSegLinetoVerticalAbs(float y)
    : mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegLinetoVerticalAbs)



NS_IMETHODIMP
nsSVGPathSegLinetoVerticalAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("V%g").get(), mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegLinetoVerticalAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  float dist = fabs(mY - ts->curPosY);
  ts->cubicCPY = ts->quadCPY = ts->curPosY = mY;
  ts->cubicCPX = ts->quadCPX = ts->curPosX;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegLinetoVerticalAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoVerticalAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegLinetoVerticalRel : public nsIDOMSVGPathSegLinetoVerticalRel,
                                      public nsSVGPathSeg
{
public:
  nsSVGPathSegLinetoVerticalRel(float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGLINETOVERTICALREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_LINETO_VERTICAL_REL; }


protected:
  float mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoVerticalRel(float y)
{
  return new nsSVGPathSegLinetoVerticalRel(y);
}


nsSVGPathSegLinetoVerticalRel::nsSVGPathSegLinetoVerticalRel(float y)
    : mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegLinetoVerticalRel)



NS_IMETHODIMP
nsSVGPathSegLinetoVerticalRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("v%g").get(), mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegLinetoVerticalRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->cubicCPY = ts->quadCPY = ts->curPosY += mY;
  ts->cubicCPX = ts->quadCPX = ts->curPosX;
  return fabs(mY);
}





NS_IMETHODIMP nsSVGPathSegLinetoVerticalRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegLinetoVerticalRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegCurvetoCubicSmoothAbs : public nsIDOMSVGPathSegCurvetoCubicSmoothAbs,
                                          public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoCubicSmoothAbs(float x, float y,
                                    float x2, float y2);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOCUBICSMOOTHABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_ABS; }

protected:
  float mX, mY, mX2, mY2;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicSmoothAbs(float x, float y,
                                      float x2, float y2)
{
  return new nsSVGPathSegCurvetoCubicSmoothAbs(x, y, x2, y2);
}


nsSVGPathSegCurvetoCubicSmoothAbs::nsSVGPathSegCurvetoCubicSmoothAbs(float x, float y,
                                                                     float x2, float y2)
    : mX(x), mY(y), mX2(x2), mY2(y2)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoCubicSmoothAbs)



NS_IMETHODIMP
nsSVGPathSegCurvetoCubicSmoothAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[96];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("S%g,%g %g,%g").get(),
                            mX2, mY2, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoCubicSmoothAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  float x1 = 2 * ts->curPosX - ts->cubicCPX;
  float y1 = 2 * ts->curPosY - ts->cubicCPY;

  PathPoint curve[4] = { {ts->curPosX, ts->curPosY},
                         {x1, y1},
                         {mX2, mY2},
                         {mX, mY} };
  float dist = CalcBezLength(curve, 4, SplitCubicBezier);

  ts->cubicCPX = mX2;
  ts->cubicCPY = mY2;
  ts->quadCPX = ts->curPosX = mX;
  ts->quadCPY = ts->curPosY = mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::GetX2(float *aX2)
{
  *aX2 = mX2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::SetX2(float aX2)
{
  mX2 = aX2;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::GetY2(float *aY2)
{
  *aY2 = mY2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothAbs::SetY2(float aY2)
{
  mY2 = aY2;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegCurvetoCubicSmoothRel : public nsIDOMSVGPathSegCurvetoCubicSmoothRel,
                                          public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoCubicSmoothRel(float x, float y,
                                    float x2, float y2);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOCUBICSMOOTHREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_REL; }

protected:
  float mX, mY, mX2, mY2;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicSmoothRel(float x, float y,
                                      float x2, float y2)
{
  return new nsSVGPathSegCurvetoCubicSmoothRel(x, y, x2, y2);
}


nsSVGPathSegCurvetoCubicSmoothRel::nsSVGPathSegCurvetoCubicSmoothRel(float x, float y,
                                                                     float x2, float y2)
    : mX(x), mY(y), mX2(x2), mY2(y2)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoCubicSmoothRel)



NS_IMETHODIMP
nsSVGPathSegCurvetoCubicSmoothRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[96];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("s%g,%g %g,%g").get(),
                            mX2, mY2, mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoCubicSmoothRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  
  float x1 = ts->curPosX - ts->cubicCPX;
  float y1 = ts->curPosY - ts->cubicCPY;

  PathPoint curve[4] = { {0, 0}, {x1, y1}, {mX2, mY2}, {mX, mY} };
  float dist = CalcBezLength(curve, 4, SplitCubicBezier);

  ts->cubicCPX = mX2 + ts->curPosX;
  ts->cubicCPY = mY2 + ts->curPosY;
  ts->quadCPX = ts->curPosX += mX;
  ts->quadCPY = ts->curPosY += mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::GetX2(float *aX2)
{
  *aX2 = mX2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::SetX2(float aX2)
{
  mX2 = aX2;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::GetY2(float *aY2)
{
  *aY2 = mY2;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoCubicSmoothRel::SetY2(float aY2)
{
  mY2 = aY2;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegCurvetoQuadraticSmoothAbs : public nsIDOMSVGPathSegCurvetoQuadraticSmoothAbs,
                                              public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOQUADRATICSMOOTHABS

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS; }

protected:
  float mX, mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y)
{
  return new nsSVGPathSegCurvetoQuadraticSmoothAbs(x, y);
}

nsSVGPathSegCurvetoQuadraticSmoothAbs::nsSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y)
    : mX(x), mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoQuadraticSmoothAbs)



NS_IMETHODIMP
nsSVGPathSegCurvetoQuadraticSmoothAbs::GetValueString(nsAString& aValue)
{
  PRUnichar buf[48];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("T%g,%g").get(), mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoQuadraticSmoothAbs::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->quadCPX = 2 * ts->curPosX - ts->quadCPX;
  ts->quadCPY = 2 * ts->curPosY - ts->quadCPY;

  PathPoint bez[3] = { {ts->curPosX, ts->curPosY},
                       {ts->quadCPX, ts->quadCPY},
                       {mX, mY} };
  float dist = CalcBezLength(bez, 3, SplitQuadraticBezier);

  ts->cubicCPX = ts->curPosX = mX;
  ts->cubicCPY = ts->curPosY = mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothAbs::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothAbs::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothAbs::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothAbs::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}




class nsSVGPathSegCurvetoQuadraticSmoothRel : public nsIDOMSVGPathSegCurvetoQuadraticSmoothRel,
                                              public nsSVGPathSeg
{
public:
  nsSVGPathSegCurvetoQuadraticSmoothRel(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEGCURVETOQUADRATICSMOOTHREL

  
  NS_IMETHOD GetValueString(nsAString& aValue);
  virtual float GetLength(nsSVGPathSegTraversalState *ts);
  virtual PRUint16 GetSegType()
    { return nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL; }


protected:
  float mX, mY;
};




nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticSmoothRel(float x, float y)
{
  return new nsSVGPathSegCurvetoQuadraticSmoothRel(x, y);
}


nsSVGPathSegCurvetoQuadraticSmoothRel::nsSVGPathSegCurvetoQuadraticSmoothRel(float x, float y)
    : mX(x), mY(y)
{
}



NS_IMPL_NSISUPPORTS_SVGPATHSEG(SVGPathSegCurvetoQuadraticSmoothRel)



NS_IMETHODIMP
nsSVGPathSegCurvetoQuadraticSmoothRel::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("t%g,%g").get(), mX, mY);
  aValue.Assign(buf);

  return NS_OK;
}

float
nsSVGPathSegCurvetoQuadraticSmoothRel::GetLength(nsSVGPathSegTraversalState *ts)
{
  ts->quadCPX = ts->curPosX - ts->quadCPX;
  ts->quadCPY = ts->curPosY - ts->quadCPY;

  PathPoint bez[3] = { {0, 0}, {ts->quadCPX, ts->quadCPY}, {mX, mY} };
  float dist = CalcBezLength(bez, 3, SplitQuadraticBezier);

  ts->quadCPX += ts->curPosX;
  ts->quadCPY += ts->curPosY;

  ts->cubicCPX = ts->curPosX += mX;
  ts->cubicCPY = ts->curPosY += mY;
  return dist;
}





NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothRel::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothRel::SetX(float aX)
{
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothRel::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPathSegCurvetoQuadraticSmoothRel::SetY(float aY)
{
  mY = aY;
  DidModify();
  return NS_OK;
}

