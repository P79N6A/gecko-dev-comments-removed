




#ifndef __NS_SVGPATHDATAPARSER_H__
#define __NS_SVGPATHDATAPARSER_H__

#include "mozilla/Attributes.h"
#include "mozilla/gfx/Point.h"
#include "nsSVGDataParser.h"

namespace mozilla {
class SVGPathData;
}






class nsSVGPathDataParser : public nsSVGDataParser
{
public:
  nsSVGPathDataParser(const nsAString& aValue,
                      mozilla::SVGPathData* aList)
    : nsSVGDataParser(aValue),
      mPathSegList(aList)
  {
    MOZ_ASSERT(aList, "null path data");
  }

  bool Parse();

private:

  bool ParseCoordPair(float& aX, float& aY);
  bool ParseFlag(bool& aFlag);

  bool ParsePath();
  bool IsStartOfSubPath() const;
  bool ParseSubPath();
  
  bool ParseSubPathElements();
  bool ParseSubPathElement(char16_t aCommandType,
                           bool aAbsCoords);

  bool ParseMoveto();
  bool ParseClosePath();
  bool ParseLineto(bool aAbsCoords);
  bool ParseHorizontalLineto(bool aAbsCoords);
  bool ParseVerticalLineto(bool aAbsCoords);
  bool ParseCurveto(bool aAbsCoords);
  bool ParseSmoothCurveto(bool aAbsCoords);
  bool ParseQuadBezierCurveto(bool aAbsCoords);
  bool ParseSmoothQuadBezierCurveto(bool aAbsCoords);  
  bool ParseEllipticalArc(bool aAbsCoords);  

  mozilla::SVGPathData * const mPathSegList;
};

class nsSVGArcConverter
{
  typedef mozilla::gfx::Point Point;

public:
  nsSVGArcConverter(const Point& from,
                    const Point& to,
                    const Point& radii,
                    double angle,
                    bool largeArcFlag,
                    bool sweepFlag);
  bool GetNextSegment(Point* cp1, Point* cp2, Point* to);
protected:
  int32_t mNumSegs, mSegIndex;
  double mTheta, mDelta, mT;
  double mSinPhi, mCosPhi;
  double mRx, mRy;
  Point mFrom, mC;
};

#endif 
