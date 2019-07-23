





































#ifndef __NS_SVGPATHDATAPARSER_H__
#define __NS_SVGPATHDATAPARSER_H__

#include "nsSVGDataParser.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"
#include "nsIDOMSVGPathSeg.h"
#include "nsTArray.h"

class nsSVGPathList;






class nsSVGPathDataParser : public nsSVGDataParser
{
protected:
  
  virtual nsresult StoreMoveTo(PRBool absCoords, float x, float y) = 0;
  virtual nsresult StoreClosePath() = 0;
  virtual nsresult StoreLineTo(PRBool absCoords, float x, float y) = 0;
  virtual nsresult StoreHLineTo(PRBool absCoords, float x) = 0;
  virtual nsresult StoreVLineTo(PRBool absCoords, float y) = 0;
  virtual nsresult StoreCurveTo(PRBool absCoords, float x, float y,
                                float x1, float y1, float x2, float y2) = 0;
  virtual nsresult StoreSmoothCurveTo(PRBool absCoords, float x, float y,
                                      float x2, float y2) = 0;
  virtual nsresult StoreQuadCurveTo(PRBool absCoords, float x, float y,
                                    float x1, float y1) = 0;
  virtual nsresult StoreSmoothQuadCurveTo(PRBool absCoords,
                                          float x, float y) = 0;
  virtual nsresult StoreEllipticalArc(PRBool absCoords, float x, float y,
                                      float r1, float r2, float angle,
                                      PRBool largeArcFlag, PRBool sweepFlag) = 0;
  nsresult  Match();
 
  nsresult MatchCoordPair(float* aX, float* aY);
  PRBool IsTokenCoordPairStarter();

  nsresult MatchCoord(float* aX);
  PRBool IsTokenCoordStarter();

  nsresult MatchFlag(PRBool* f);

  nsresult MatchSvgPath();
  
  nsresult MatchSubPaths();
  PRBool IsTokenSubPathsStarter();
  
  nsresult MatchSubPath();
  PRBool IsTokenSubPathStarter();
  
  nsresult MatchSubPathElements();
  PRBool IsTokenSubPathElementsStarter();

  nsresult MatchSubPathElement();
  PRBool IsTokenSubPathElementStarter();

  nsresult MatchMoveto();
  nsresult MatchMovetoArgSeq(PRBool absCoords);
  
  nsresult MatchClosePath();
  
  nsresult MatchLineto();
  
  nsresult MatchLinetoArgSeq(PRBool absCoords);
  PRBool IsTokenLinetoArgSeqStarter();
  
  nsresult MatchHorizontalLineto();
  nsresult MatchHorizontalLinetoArgSeq(PRBool absCoords);
  
  nsresult MatchVerticalLineto();
  nsresult MatchVerticalLinetoArgSeq(PRBool absCoords);
  
  nsresult MatchCurveto();
  nsresult MatchCurvetoArgSeq(PRBool absCoords);
  nsresult MatchCurvetoArg(float* x, float* y, float* x1,
                           float* y1, float* x2, float* y2);
  PRBool IsTokenCurvetoArgStarter();
  
  nsresult MatchSmoothCurveto();
  nsresult MatchSmoothCurvetoArgSeq(PRBool absCoords);
  nsresult MatchSmoothCurvetoArg(float* x, float* y, float* x2, float* y2);
  PRBool IsTokenSmoothCurvetoArgStarter();
  
  nsresult MatchQuadBezierCurveto();
  nsresult MatchQuadBezierCurvetoArgSeq(PRBool absCoords);  
  nsresult MatchQuadBezierCurvetoArg(float* x, float* y, float* x1, float* y1);
  PRBool IsTokenQuadBezierCurvetoArgStarter();
  
  nsresult MatchSmoothQuadBezierCurveto();  
  nsresult MatchSmoothQuadBezierCurvetoArgSeq(PRBool absCoords);
  
  nsresult MatchEllipticalArc();  
  nsresult MatchEllipticalArcArgSeq(PRBool absCoords);
  nsresult MatchEllipticalArcArg(float* x, float* y,
                                 float* r1, float* r2, float* angle,
                                 PRBool* largeArcFlag, PRBool* sweepFlag);
  PRBool IsTokenEllipticalArcArgStarter();
  
 };

class nsSVGPathDataParserToInternal : public nsSVGPathDataParser
{
public:
  nsSVGPathDataParserToInternal(nsSVGPathList *data) : mPathData(data) {}
  virtual nsresult Parse(const nsAString &aValue);

protected:
  virtual nsresult StoreMoveTo(PRBool absCoords, float x, float y);
  virtual nsresult StoreClosePath();
  virtual nsresult StoreLineTo(PRBool absCoords, float x, float y);
  virtual nsresult StoreHLineTo(PRBool absCoords, float x);
  virtual nsresult StoreVLineTo(PRBool absCoords, float y);
  virtual nsresult StoreCurveTo(PRBool absCoords, float x, float y,
                                float x1, float y1, float x2, float y2);
  virtual nsresult StoreSmoothCurveTo(PRBool absCoords, float x, float y,
                                      float x2, float y2);
  virtual nsresult StoreQuadCurveTo(PRBool absCoords, float x, float y,
                                    float x1, float y1);
  virtual nsresult StoreSmoothQuadCurveTo(PRBool absCoords,
                                          float x, float y);
  virtual nsresult StoreEllipticalArc(PRBool absCoords, float x, float y,
                                      float r1, float r2, float angle,
                                      PRBool largeArcFlag, PRBool sweepFlag);

private:
  nsSVGPathList *mPathData;
  PRUint16 mPrevSeg;       
  float mPx, mPy;          
  float mCx, mCy;          
  float mStartX, mStartY;  

  
  nsTArray<PRUint8> mCommands;
  nsTArray<float>   mArguments;
  PRUint32          mNumArguments;
  PRUint32          mNumCommands;

  
  nsresult ConvertArcToCurves(float x2, float y2, float rx, float ry,
                              float angle, PRBool largeArcFlag, PRBool sweepFlag);

  nsresult PathEnsureSpace(PRUint32 aNumArgs);
  void PathAddCommandCode(PRUint8 aCommand);
  nsresult PathMoveTo(float x, float y);
  nsresult PathLineTo(float x, float y);
  nsresult PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3);
  nsresult PathClose();
  nsresult PathFini();
};

class nsSVGPathDataParserToDOM : public nsSVGPathDataParser
{
public:
  nsSVGPathDataParserToDOM(nsCOMArray<nsIDOMSVGPathSeg>* data) : mData(data) {}

protected:
  virtual nsresult StoreMoveTo(PRBool absCoords, float x, float y);
  virtual nsresult StoreClosePath();
  virtual nsresult StoreLineTo(PRBool absCoords, float x, float y);
  virtual nsresult StoreHLineTo(PRBool absCoords, float x);
  virtual nsresult StoreVLineTo(PRBool absCoords, float y);
  virtual nsresult StoreCurveTo(PRBool absCoords, float x, float y,
                                float x1, float y1, float x2, float y2);
  virtual nsresult StoreSmoothCurveTo(PRBool absCoords, float x, float y,
                                      float x2, float y2);
  virtual nsresult StoreQuadCurveTo(PRBool absCoords, float x, float y,
                                    float x1, float y1);
  virtual nsresult StoreSmoothQuadCurveTo(PRBool absCoords,
                                          float x, float y);
  virtual nsresult StoreEllipticalArc(PRBool absCoords, float x, float y,
                                      float r1, float r2, float angle,
                                      PRBool largeArcFlag, PRBool sweepFlag);

private:
  nsresult AppendSegment(nsIDOMSVGPathSeg* seg);

  nsCOMArray<nsIDOMSVGPathSeg>* mData;
};

class nsSVGArcConverter
{
public:
  nsSVGArcConverter(float x1, float y1,
                    float x2, float y2,
                    float rx, float ry,
                    float angle,
                    PRBool largeArcFlag,
                    PRBool sweepFlag);
  PRBool GetNextSegment(float *x1, float *y1,
                        float *x2, float *y2,
                        float *x3, float *y3);
protected:
  PRInt32 mNumSegs, mSegIndex;
  float mTheta, mDelta, mT;
  float mSinPhi, mCosPhi;
  float mX1, mY1, mRx, mRy, mCx, mCy;

};

#endif 
