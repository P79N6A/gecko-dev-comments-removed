




































#include "SVGMotionSMILAnimationFunction.h"
#include "nsSMILParserUtils.h"
#include "nsSVGAngle.h"
#include "SVGMotionSMILType.h"
#include "SVGMotionSMILPathUtils.h"
#include "nsSVGPathDataParser.h"
#include "nsSVGPathSeg.h"
#include "nsSVGPathElement.h" 
#include "nsSVGMpathElement.h"

namespace mozilla {

SVGMotionSMILAnimationFunction::SVGMotionSMILAnimationFunction()
  : mRotateType(eRotateType_Explicit),
    mRotateAngle(0.0f),
    mPathSourceType(ePathSourceType_None),
    mIsPathStale(PR_TRUE)  
{
}

void
SVGMotionSMILAnimationFunction::MarkStaleIfAttributeAffectsPath(nsIAtom* aAttribute)
{
  PRBool isAffected;
  if (aAttribute == nsGkAtoms::path) {
    isAffected = (mPathSourceType <= ePathSourceType_PathAttr);
  } else if (aAttribute == nsGkAtoms::values) {
    isAffected = (mPathSourceType <= ePathSourceType_ValuesAttr);
  } else if (aAttribute == nsGkAtoms::from ||
             aAttribute == nsGkAtoms::to) {
    isAffected = (mPathSourceType <= ePathSourceType_ToAttr);
  } else if (aAttribute == nsGkAtoms::by) {
    isAffected = (mPathSourceType <= ePathSourceType_ByAttr);
  } else {
    NS_NOTREACHED("Should only call this method for path-describing attrs");
    isAffected = PR_FALSE;
  }

  if (isAffected) {
    mIsPathStale = PR_TRUE;
    mHasChanged = PR_TRUE;
  }
}

PRBool
SVGMotionSMILAnimationFunction::SetAttr(nsIAtom* aAttribute,
                                        const nsAString& aValue,
                                        nsAttrValue& aResult,
                                        nsresult* aParseResult)
{
  
  if (aAttribute == nsGkAtoms::keyPoints) {
    nsresult rv = SetKeyPoints(aValue, aResult);
    if (aParseResult) {
      *aParseResult = rv;
    }
  } else if (aAttribute == nsGkAtoms::rotate) {
    nsresult rv = SetRotate(aValue, aResult);
    if (aParseResult) {
      *aParseResult = rv;
    }
  } else if (aAttribute == nsGkAtoms::path) {
    aResult.SetTo(aValue);
    if (aParseResult) {
      *aParseResult = NS_OK;
    }
    MarkStaleIfAttributeAffectsPath(aAttribute);
  } else if (aAttribute == nsGkAtoms::by ||
             aAttribute == nsGkAtoms::from ||
             aAttribute == nsGkAtoms::to ||
             aAttribute == nsGkAtoms::values) {
    MarkStaleIfAttributeAffectsPath(aAttribute);
  } else {
    
    return nsSMILAnimationFunction::SetAttr(aAttribute, aValue,
                                            aResult, aParseResult);
  }

  return PR_TRUE;
}

PRBool
SVGMotionSMILAnimationFunction::UnsetAttr(nsIAtom* aAttribute)
{
  if (aAttribute == nsGkAtoms::keyPoints) {
    UnsetKeyPoints();
  } else if (aAttribute == nsGkAtoms::rotate) {
    UnsetRotate();
  } else if (aAttribute == nsGkAtoms::path ||
             aAttribute == nsGkAtoms::by ||
             aAttribute == nsGkAtoms::from ||
             aAttribute == nsGkAtoms::to ||
             aAttribute == nsGkAtoms::values) {
    MarkStaleIfAttributeAffectsPath(aAttribute);
  } else {
    
    return nsSMILAnimationFunction::UnsetAttr(aAttribute);
  }

  return PR_TRUE;
}

nsSMILAnimationFunction::nsSMILCalcMode
SVGMotionSMILAnimationFunction::GetCalcMode() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::calcMode);
  if (!value) {
    return CALC_PACED; 
  }

  return nsSMILCalcMode(value->GetEnumValue());
}








static nsSVGMpathElement*
GetFirstMpathChild(nsIContent* aElem)
{
  PRUint32 childCount = aElem->GetChildCount();
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent* child = aElem->GetChildAt(i);
    if (child->Tag() == nsGkAtoms::mpath &&
        child->GetNameSpaceID() == kNameSpaceID_SVG) {
      return static_cast<nsSVGMpathElement*>(child);
    }
  }

  return nsnull;
}

void
SVGMotionSMILAnimationFunction::
  RebuildPathAndVerticesFromBasicAttrs(const nsIContent* aContextElem)
{
  NS_ABORT_IF_FALSE(!HasAttr(nsGkAtoms::path),
                    "Should be using |path| attr if we have it");
  NS_ABORT_IF_FALSE(!mPath, "regenerating when we aleady have path");
  NS_ABORT_IF_FALSE(mPathVertices.IsEmpty(),
                    "regenerating when we already have vertices");

  if (aContextElem->GetNameSpaceID() != kNameSpaceID_SVG) {
    NS_ERROR("Uh oh, SVG animateMotion element targeting a non-SVG node");
    return;
  }

  
  
  
  nsSVGElement* svgCtx =
    static_cast<nsSVGElement*>(const_cast<nsIContent*>(aContextElem));
  SVGMotionSMILPathUtils::PathGenerator pathGenerator(svgCtx);

  PRBool success = PR_FALSE;
  if (HasAttr(nsGkAtoms::values)) {
    
    mPathSourceType = ePathSourceType_ValuesAttr;
    const nsAString& valuesStr = GetAttr(nsGkAtoms::values)->GetStringValue();
    SVGMotionSMILPathUtils::MotionValueParser parser(&pathGenerator,
                                                     &mPathVertices);
    success =
      NS_SUCCEEDED(nsSMILParserUtils::ParseValuesGeneric(valuesStr, parser));
  } else if (HasAttr(nsGkAtoms::to) || HasAttr(nsGkAtoms::by)) {
    
    if (HasAttr(nsGkAtoms::from)) {
      const nsAString& fromStr = GetAttr(nsGkAtoms::from)->GetStringValue();
      success = pathGenerator.MoveToAbsolute(fromStr);
      mPathVertices.AppendElement(0.0);
    } else {
      
      
      
      
      pathGenerator.MoveToOrigin();
      if (!HasAttr(nsGkAtoms::to)) {
        mPathVertices.AppendElement(0.0);
      }
      success = PR_TRUE;
    }

    
    if (success) {
      double dist;
      if (HasAttr(nsGkAtoms::to)) {
        mPathSourceType = ePathSourceType_ToAttr;
        const nsAString& toStr = GetAttr(nsGkAtoms::to)->GetStringValue();
        success = pathGenerator.LineToAbsolute(toStr, dist);
      } else { 
        mPathSourceType = ePathSourceType_ByAttr;
        const nsAString& byStr = GetAttr(nsGkAtoms::by)->GetStringValue();
        success = pathGenerator.LineToRelative(byStr, dist);
      }
      if (success) {
        mPathVertices.AppendElement(dist);
      }
    }
  }
  if (success) {
    mPath = pathGenerator.GetResultingPath();
  } else {
    
    mPathVertices.Clear();
  }
}

void
SVGMotionSMILAnimationFunction::
  RebuildPathAndVerticesFromMpathElem(nsSVGMpathElement* aMpathElem)
{
  mPathSourceType = ePathSourceType_Mpath;

  
  nsSVGPathElement* pathElem = aMpathElem->GetReferencedPath();
  if (pathElem) {
    if (pathElem->HasAttr(kNameSpaceID_None, nsGkAtoms::d)) {
      const nsAString& pathSpec =
        pathElem->GetParsedAttr(nsGkAtoms::d)->GetStringValue();
      nsresult rv = SetPathVerticesFromPathString(pathSpec);
      if (NS_SUCCEEDED(rv)) {
        mPath = pathElem->GetFlattenedPath(
          pathElem->PrependLocalTransformTo(gfxMatrix()));
      }
    }
  }
}

void
SVGMotionSMILAnimationFunction::RebuildPathAndVerticesFromPathAttr()
{
  const nsAString& pathSpec = GetAttr(nsGkAtoms::path)->GetStringValue();
  mPathSourceType = ePathSourceType_PathAttr;

  
  nsresult rv;
  nsSVGPathList pathData;
  nsSVGPathDataParserToInternal pathParser(&pathData);
  rv = pathParser.Parse(pathSpec);
  if (NS_FAILED(rv)) {
    
    return;
  }
  mPath = pathData.GetFlattenedPath(gfxMatrix());

  
  rv = SetPathVerticesFromPathString(pathSpec);

  if (NS_FAILED(rv)) {
    
    
    
    mPath = nsnull;
    NS_WARNING("nsSVGPathDataParserToInternal successfully parsed path string, but "
               "nsSVGPathDataParserToDOM did not");
  }
}

nsresult
SVGMotionSMILAnimationFunction::SetPathVerticesFromPathString(const nsAString& aPathSpec)
{
  
  nsCOMArray<nsIDOMSVGPathSeg> pathSegments;
  nsSVGPathDataParserToDOM segmentParser(&pathSegments);
  nsresult rv = segmentParser.Parse(aPathSpec);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  PRUint32 numSegments = pathSegments.Count();
  nsSVGPathSegTraversalState ts;
  double runningDistTotal = 0.0;
  for (PRUint32 i = 0; i < numSegments; ++i) {
    nsSVGPathSeg* segment = static_cast<nsSVGPathSeg*>(pathSegments[i]);

    PRUint16 type = nsIDOMSVGPathSeg::PATHSEG_UNKNOWN;
    segment->GetPathSegType(&type);
    if (i == 0 ||
        (type != nsIDOMSVGPathSeg::PATHSEG_MOVETO_ABS &&
         type != nsIDOMSVGPathSeg::PATHSEG_MOVETO_REL)) {

      
      runningDistTotal += segment->GetLength(&ts);

      
      if (!mPathVertices.AppendElement(runningDistTotal)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }
  return NS_OK;
}


void
SVGMotionSMILAnimationFunction::
  RebuildPathAndVertices(const nsIContent* aTargetElement)
{
  NS_ABORT_IF_FALSE(mIsPathStale, "rebuilding path when it isn't stale");

  
  mPath = nsnull;
  mPathVertices.Clear();
  mPathSourceType = ePathSourceType_None;

  
  
  nsSVGMpathElement* firstMpathChild =
    GetFirstMpathChild(&mAnimationElement->Content());

  if (firstMpathChild) {
    RebuildPathAndVerticesFromMpathElem(firstMpathChild);
    mValueNeedsReparsingEverySample = PR_FALSE;
  } else if (HasAttr(nsGkAtoms::path)) {
    RebuildPathAndVerticesFromPathAttr();
    mValueNeedsReparsingEverySample = PR_FALSE;
  } else {
    

    RebuildPathAndVerticesFromBasicAttrs(aTargetElement);
    mValueNeedsReparsingEverySample = PR_TRUE;
  }
  mIsPathStale = PR_FALSE;
}

PRBool
SVGMotionSMILAnimationFunction::
  GenerateValuesForPathAndPoints(gfxFlattenedPath* aPath,
                                 PRBool aIsKeyPoints,
                                 nsTArray<double>& aPointDistances,
                                 nsTArray<nsSMILValue>& aResult)
{
  NS_ABORT_IF_FALSE(aResult.IsEmpty(), "outparam is non-empty");

  
  
  double distanceMultiplier = aIsKeyPoints ? aPath->GetLength() : 1.0;
  const PRUint32 numPoints = aPointDistances.Length();
  for (PRUint32 i = 0; i < numPoints; ++i) {
    double curDist = aPointDistances[i] * distanceMultiplier;
    if (!aResult.AppendElement(
          SVGMotionSMILType::ConstructSMILValue(aPath, curDist,
                                                mRotateType, mRotateAngle))) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

nsresult
SVGMotionSMILAnimationFunction::GetValues(const nsISMILAttr& aSMILAttr,
                                          nsSMILValueArray& aResult)
{
  if (mIsPathStale) {
    RebuildPathAndVertices(aSMILAttr.GetTargetNode());
  }
  NS_ABORT_IF_FALSE(!mIsPathStale, "Forgot to clear 'is path stale' state");

  if (!mPath) {
    
    NS_ABORT_IF_FALSE(mPathVertices.IsEmpty(), "have vertices but no path");
    return NS_ERROR_FAILURE;
  }
  NS_ABORT_IF_FALSE(!mPathVertices.IsEmpty(), "have a path but no vertices");

  
  PRBool isUsingKeyPoints = !mKeyPoints.IsEmpty();
  PRBool success = GenerateValuesForPathAndPoints(mPath, isUsingKeyPoints,
                                                  isUsingKeyPoints ?
                                                  mKeyPoints : mPathVertices,
                                                  aResult);
  if (!success) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
SVGMotionSMILAnimationFunction::
  CheckValueListDependentAttrs(PRUint32 aNumValues)
{
  
  nsSMILAnimationFunction::CheckValueListDependentAttrs(aNumValues);

  
  CheckKeyPoints();
}

void
SVGMotionSMILAnimationFunction::CheckKeyPoints()
{
  if (!HasAttr(nsGkAtoms::keyPoints))
    return;

  
  if (GetCalcMode() == CALC_PACED) {
    SetKeyPointsErrorFlag(PR_FALSE);
  }

  if (mKeyPoints.IsEmpty()) {
    
    SetKeyPointsErrorFlag(PR_TRUE);
    return;
  }

  
  
  
  
  
  
  
  
}

nsresult
SVGMotionSMILAnimationFunction::SetKeyPoints(const nsAString& aKeyPoints,
                                             nsAttrValue& aResult)
{
  mKeyPoints.Clear();
  aResult.SetTo(aKeyPoints);

  nsresult rv =
    nsSMILParserUtils::ParseSemicolonDelimitedProgressList(aKeyPoints, PR_FALSE,
                                                           mKeyPoints);

  if (NS_SUCCEEDED(rv) && mKeyPoints.Length() < 1)
    rv = NS_ERROR_FAILURE;

  if (NS_FAILED(rv)) {
    mKeyPoints.Clear();
  }

  mHasChanged = PR_TRUE;

  return NS_OK;
}

void
SVGMotionSMILAnimationFunction::UnsetKeyPoints()
{
  mKeyTimes.Clear();
  SetKeyPointsErrorFlag(PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
SVGMotionSMILAnimationFunction::SetRotate(const nsAString& aRotate,
                                          nsAttrValue& aResult)
{
  mHasChanged = PR_TRUE;

  aResult.SetTo(aRotate);
  if (aRotate.EqualsLiteral("auto")) {
    mRotateType = eRotateType_Auto;
  } else if (aRotate.EqualsLiteral("auto-reverse")) {
    mRotateType = eRotateType_AutoReverse;
  } else {
    mRotateType = eRotateType_Explicit;

    
    nsSVGAngle svgAngle;
    svgAngle.Init();
    nsresult rv = svgAngle.SetBaseValueString(aRotate, nsnull, PR_FALSE);
    if (NS_FAILED(rv)) { 
      mRotateAngle = 0.0f; 
      
      return rv;
    }

    mRotateAngle = svgAngle.GetBaseValInSpecifiedUnits();

    
    PRUint8 angleUnit = svgAngle.GetBaseValueUnit();
    if (angleUnit != nsIDOMSVGAngle::SVG_ANGLETYPE_RAD) {
      mRotateAngle *= nsSVGAngle::GetDegreesPerUnit(angleUnit) /
        nsSVGAngle::GetDegreesPerUnit(nsIDOMSVGAngle::SVG_ANGLETYPE_RAD);
    }
  }
  return NS_OK;
}

void
SVGMotionSMILAnimationFunction::UnsetRotate()
{
  mRotateAngle = 0.0f; 
  mRotateType = eRotateType_Explicit;
  mHasChanged = PR_TRUE;
}

PRBool
SVGMotionSMILAnimationFunction::TreatSingleValueAsStatic() const
{
  
  
  
  return (mPathSourceType == ePathSourceType_ValuesAttr ||
          mPathSourceType == ePathSourceType_PathAttr ||
          mPathSourceType == ePathSourceType_Mpath);
}

} 
