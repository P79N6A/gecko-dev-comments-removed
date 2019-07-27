




#include "SVGMotionSMILPathUtils.h"

#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h" 
#include "SVGContentUtils.h"
#include "SVGLength.h"

using namespace mozilla::gfx;

namespace mozilla {





void
SVGMotionSMILPathUtils::PathGenerator::
  MoveToOrigin()
{
  NS_ABORT_IF_FALSE(!mHaveReceivedCommands,
                    "Not expecting requests for mid-path MoveTo commands");
  mHaveReceivedCommands = true;
  mPathBuilder->MoveTo(Point(0, 0));
}


bool
SVGMotionSMILPathUtils::PathGenerator::
  MoveToAbsolute(const nsAString& aCoordPairStr)
{
  NS_ABORT_IF_FALSE(!mHaveReceivedCommands,
                    "Not expecting requests for mid-path MoveTo commands");
  mHaveReceivedCommands = true;

  float xVal, yVal;
  if (!ParseCoordinatePair(aCoordPairStr, xVal, yVal)) {
    return false;
  }
  mPathBuilder->MoveTo(Point(xVal, yVal));
  return true;
}


bool
SVGMotionSMILPathUtils::PathGenerator::
  LineToAbsolute(const nsAString& aCoordPairStr, double& aSegmentDistance)
{
  mHaveReceivedCommands = true;

  float xVal, yVal;
  if (!ParseCoordinatePair(aCoordPairStr, xVal, yVal)) {
    return false;
  }
  Point initialPoint = mPathBuilder->CurrentPoint();

  mPathBuilder->LineTo(Point(xVal, yVal));
  aSegmentDistance = NS_hypot(initialPoint.x - xVal, initialPoint.y -yVal);
  return true;
}


bool
SVGMotionSMILPathUtils::PathGenerator::
  LineToRelative(const nsAString& aCoordPairStr, double& aSegmentDistance)
{
  mHaveReceivedCommands = true;

  float xVal, yVal;
  if (!ParseCoordinatePair(aCoordPairStr, xVal, yVal)) {
    return false;
  }
  mPathBuilder->LineTo(mPathBuilder->CurrentPoint() + Point(xVal, yVal));
  aSegmentDistance = NS_hypot(xVal, yVal);
  return true;
}

TemporaryRef<Path>
SVGMotionSMILPathUtils::PathGenerator::GetResultingPath()
{
  return mPathBuilder->Finish();
}




bool
SVGMotionSMILPathUtils::PathGenerator::
  ParseCoordinatePair(const nsAString& aCoordPairStr,
                      float& aXVal, float& aYVal)
{
  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aCoordPairStr, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);

  SVGLength x, y;

  if (!tokenizer.hasMoreTokens() ||
      !x.SetValueFromString(tokenizer.nextToken())) {
    return false;
  }

  if (!tokenizer.hasMoreTokens() ||
      !y.SetValueFromString(tokenizer.nextToken())) { 
    return false;
  }

  if (tokenizer.separatorAfterCurrentToken() ||  
      tokenizer.hasMoreTokens()) {               
    return false;
  }

  float xRes = x.GetValueInUserUnits(mSVGElement, SVGContentUtils::X);
  float yRes = y.GetValueInUserUnits(mSVGElement, SVGContentUtils::Y);

  NS_ENSURE_FINITE2(xRes, yRes, false);

  aXVal = xRes;
  aYVal = yRes;
  return true;
}



bool
SVGMotionSMILPathUtils::MotionValueParser::
  Parse(const nsAString& aValueStr)
{
  bool success;
  if (!mPathGenerator->HaveReceivedCommands()) {
    
    success = mPathGenerator->MoveToAbsolute(aValueStr);
    if (success) {
      success = !!mPointDistances->AppendElement(0.0);
    }
  } else {
    double dist;
    success = mPathGenerator->LineToAbsolute(aValueStr, dist);
    if (success) {
      mDistanceSoFar += dist;
      success = !!mPointDistances->AppendElement(mDistanceSoFar);
    }
  }
  return success;
}

} 
