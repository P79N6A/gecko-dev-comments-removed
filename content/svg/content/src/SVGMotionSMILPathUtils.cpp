




































#include "SVGMotionSMILPathUtils.h"
#include "nsSVGElement.h"
#include "SVGLength.h"
#include "nsContentCreatorFunctions.h" 
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"

namespace mozilla {





void
SVGMotionSMILPathUtils::PathGenerator::
  MoveToOrigin()
{
  NS_ABORT_IF_FALSE(!mHaveReceivedCommands,
                    "Not expecting requests for mid-path MoveTo commands");
  mHaveReceivedCommands = PR_TRUE;
  mGfxContext.MoveTo(gfxPoint(0, 0));
}


bool
SVGMotionSMILPathUtils::PathGenerator::
  MoveToAbsolute(const nsAString& aCoordPairStr)
{
  NS_ABORT_IF_FALSE(!mHaveReceivedCommands,
                    "Not expecting requests for mid-path MoveTo commands");
  mHaveReceivedCommands = PR_TRUE;

  float xVal, yVal;
  if (!ParseCoordinatePair(aCoordPairStr, xVal, yVal)) {
    return PR_FALSE;
  }
  mGfxContext.MoveTo(gfxPoint(xVal, yVal));
  return PR_TRUE;
}


bool
SVGMotionSMILPathUtils::PathGenerator::
  LineToAbsolute(const nsAString& aCoordPairStr, double& aSegmentDistance)
{
  mHaveReceivedCommands = PR_TRUE;

  float xVal, yVal;
  if (!ParseCoordinatePair(aCoordPairStr, xVal, yVal)) {
    return PR_FALSE;
  }
  gfxPoint initialPoint = mGfxContext.CurrentPoint();

  mGfxContext.LineTo(gfxPoint(xVal, yVal));
  aSegmentDistance = NS_hypot(initialPoint.x - xVal, initialPoint.y -yVal);
  return PR_TRUE;
}


bool
SVGMotionSMILPathUtils::PathGenerator::
  LineToRelative(const nsAString& aCoordPairStr, double& aSegmentDistance)
{
  mHaveReceivedCommands = PR_TRUE;

  float xVal, yVal;
  if (!ParseCoordinatePair(aCoordPairStr, xVal, yVal)) {
    return PR_FALSE;
  }
  mGfxContext.LineTo(mGfxContext.CurrentPoint() + gfxPoint(xVal, yVal));
  aSegmentDistance = NS_hypot(xVal, yVal);
  return PR_TRUE;
}

already_AddRefed<gfxFlattenedPath>
SVGMotionSMILPathUtils::PathGenerator::GetResultingPath()
{
  return mGfxContext.GetFlattenedPath();
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
    return PR_FALSE;
  }

  if (!tokenizer.hasMoreTokens() ||
      !y.SetValueFromString(tokenizer.nextToken())) { 
    return PR_FALSE;
  }

  if (tokenizer.lastTokenEndedWithSeparator() || 
      tokenizer.hasMoreTokens()) {               
    return PR_FALSE;
  }

  float xRes = x.GetValueInUserUnits(mSVGElement, nsSVGUtils::X);
  float yRes = y.GetValueInUserUnits(mSVGElement, nsSVGUtils::Y);

  NS_ENSURE_FINITE2(xRes, yRes, PR_FALSE);

  aXVal = xRes;
  aYVal = yRes;
  return PR_TRUE;
}



nsresult
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
  return success ? NS_OK : NS_ERROR_FAILURE;
}

} 
