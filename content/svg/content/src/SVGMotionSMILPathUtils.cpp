




































#include "SVGMotionSMILPathUtils.h"
#include "nsSVGElement.h"
#include "nsSVGLength2.h"
#include "nsContentCreatorFunctions.h" 


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


PRBool
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


PRBool
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


PRBool
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




static PRBool
ParseOneCoordinate(char** aRest, nsSVGLength2& aLengthVal)
{
  aLengthVal.Init();

  
  
  
  
  
  
  char* token = nsCRT::strtok(*aRest, SVG_COMMA_WSP_DELIM, aRest);
  if (!token) {
    return PR_FALSE;
  }

  
  nsresult rv = aLengthVal.SetBaseValueString(NS_ConvertASCIItoUTF16(token),
                                              nsnull, PR_FALSE);
  return NS_SUCCEEDED(rv);
}

PRBool
SVGMotionSMILPathUtils::PathGenerator::
  ParseCoordinatePair(const nsAString& aCoordPairStr,
                      float& aXVal, float& aYVal)
{
  nsSVGLength2 xLength, yLength;
  
  char* str = ToNewCString(aCoordPairStr);
  char* rest = str;
  PRBool success = PR_FALSE;

  if (ParseOneCoordinate(&rest, xLength) &&
      ParseOneCoordinate(&rest, yLength)) {

    
    PRBool foundTrailingNonWhitespace = PR_FALSE;
    while (*rest != '\0') {
      if (!IsSVGWhitespace(*rest)) {
        foundTrailingNonWhitespace = PR_TRUE;
        break;
      }
    }
    if (!foundTrailingNonWhitespace) {
      success = PR_TRUE;
    }
  }
  nsMemory::Free(str);

  if (success) {
    aXVal = xLength.GetBaseValue(mSVGElement);
    aYVal = yLength.GetBaseValue(mSVGElement);
  }
  return success;
}



nsresult
SVGMotionSMILPathUtils::MotionValueParser::
  Parse(const nsAString& aValueStr)
{
  PRBool success;
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
