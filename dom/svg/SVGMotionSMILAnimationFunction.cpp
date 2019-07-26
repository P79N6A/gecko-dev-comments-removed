




#include "SVGMotionSMILAnimationFunction.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "mozilla/dom/SVGPathElement.h" 
#include "mozilla/dom/SVGMPathElement.h"
#include "mozilla/gfx/2D.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsSMILParserUtils.h"
#include "nsSVGAngle.h"
#include "nsSVGPathDataParser.h"
#include "SVGMotionSMILType.h"
#include "SVGMotionSMILPathUtils.h"

using namespace mozilla::dom;
using namespace mozilla::gfx;

namespace mozilla {

SVGMotionSMILAnimationFunction::SVGMotionSMILAnimationFunction()
  : mRotateType(eRotateType_Explicit),
    mRotateAngle(0.0f),
    mPathSourceType(ePathSourceType_None),
    mIsPathStale(true)  
{
}

void
SVGMotionSMILAnimationFunction::MarkStaleIfAttributeAffectsPath(nsIAtom* aAttribute)
{
  bool isAffected;
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
    isAffected = false;
  }

  if (isAffected) {
    mIsPathStale = true;
    mHasChanged = true;
  }
}

bool
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
  } else if (aAttribute == nsGkAtoms::path ||
             aAttribute == nsGkAtoms::by ||
             aAttribute == nsGkAtoms::from ||
             aAttribute == nsGkAtoms::to ||
             aAttribute == nsGkAtoms::values) {
    aResult.SetTo(aValue);
    MarkStaleIfAttributeAffectsPath(aAttribute);
    if (aParseResult) {
      *aParseResult = NS_OK;
    }
  } else {
    
    return nsSMILAnimationFunction::SetAttr(aAttribute, aValue,
                                            aResult, aParseResult);
  }

  return true;
}

bool
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

  return true;
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








static SVGMPathElement*
GetFirstMPathChild(nsIContent* aElem)
{
  for (nsIContent* child = aElem->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsSVG(nsGkAtoms::mpath)) {
      return static_cast<SVGMPathElement*>(child);
    }
  }

  return nullptr;
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

  if (!aContextElem->IsSVG()) {
    NS_ERROR("Uh oh, SVG animateMotion element targeting a non-SVG node");
    return;
  }

  SVGMotionSMILPathUtils::PathGenerator
    pathGenerator(static_cast<const nsSVGElement*>(aContextElem));

  bool success = false;
  if (HasAttr(nsGkAtoms::values)) {
    
    mPathSourceType = ePathSourceType_ValuesAttr;
    const nsAString& valuesStr = GetAttr(nsGkAtoms::values)->GetStringValue();
    SVGMotionSMILPathUtils::MotionValueParser parser(&pathGenerator,
                                                     &mPathVertices);
    success = nsSMILParserUtils::ParseValuesGeneric(valuesStr, parser);
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
      success = true;
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
  RebuildPathAndVerticesFromMpathElem(SVGMPathElement* aMpathElem)
{
  mPathSourceType = ePathSourceType_Mpath;

  
  SVGPathElement* pathElem = aMpathElem->GetReferencedPath();
  if (pathElem) {
    const SVGPathData &path = pathElem->GetAnimPathSegList()->GetAnimValue();
    
    
    if (path.Length()) {
      bool ok =
        path.GetDistancesFromOriginToEndsOfVisibleSegments(&mPathVertices);
      if (ok && mPathVertices.Length()) {
        mPath = pathElem->GetPathForLengthOrPositionMeasuring();
      }
    }
  }
}

void
SVGMotionSMILAnimationFunction::RebuildPathAndVerticesFromPathAttr()
{
  const nsAString& pathSpec = GetAttr(nsGkAtoms::path)->GetStringValue();
  mPathSourceType = ePathSourceType_PathAttr;

  
  SVGPathData path;
  nsSVGPathDataParser pathParser(pathSpec, &path);

  
  
  
  
  pathParser.Parse();
  if (!path.Length()) {
    return;
  }

  mPath = path.ToPathForLengthOrPositionMeasuring();
  bool ok = path.GetDistancesFromOriginToEndsOfVisibleSegments(&mPathVertices);
  if (!ok || !mPathVertices.Length()) {
    mPath = nullptr;
  }
}


void
SVGMotionSMILAnimationFunction::
  RebuildPathAndVertices(const nsIContent* aTargetElement)
{
  NS_ABORT_IF_FALSE(mIsPathStale, "rebuilding path when it isn't stale");

  
  mPath = nullptr;
  mPathVertices.Clear();
  mPathSourceType = ePathSourceType_None;

  
  
  SVGMPathElement* firstMpathChild = GetFirstMPathChild(mAnimationElement);

  if (firstMpathChild) {
    RebuildPathAndVerticesFromMpathElem(firstMpathChild);
    mValueNeedsReparsingEverySample = false;
  } else if (HasAttr(nsGkAtoms::path)) {
    RebuildPathAndVerticesFromPathAttr();
    mValueNeedsReparsingEverySample = false;
  } else {
    

    RebuildPathAndVerticesFromBasicAttrs(aTargetElement);
    mValueNeedsReparsingEverySample = true;
  }
  mIsPathStale = false;
}

bool
SVGMotionSMILAnimationFunction::
  GenerateValuesForPathAndPoints(Path* aPath,
                                 bool aIsKeyPoints,
                                 FallibleTArray<double>& aPointDistances,
                                 nsSMILValueArray& aResult)
{
  NS_ABORT_IF_FALSE(aResult.IsEmpty(), "outparam is non-empty");

  
  
  double distanceMultiplier = aIsKeyPoints ? aPath->ComputeLength() : 1.0;
  const uint32_t numPoints = aPointDistances.Length();
  for (uint32_t i = 0; i < numPoints; ++i) {
    double curDist = aPointDistances[i] * distanceMultiplier;
    if (!aResult.AppendElement(
          SVGMotionSMILType::ConstructSMILValue(aPath, curDist,
                                                mRotateType, mRotateAngle))) {
      return false;
    }
  }
  return true;
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

  
  bool isUsingKeyPoints = !mKeyPoints.IsEmpty();
  bool success = GenerateValuesForPathAndPoints(mPath, isUsingKeyPoints,
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
  CheckValueListDependentAttrs(uint32_t aNumValues)
{
  
  nsSMILAnimationFunction::CheckValueListDependentAttrs(aNumValues);

  
  CheckKeyPoints();
}

bool
SVGMotionSMILAnimationFunction::IsToAnimation() const
{
  
  
  
  
  return !GetFirstMPathChild(mAnimationElement) &&
    !HasAttr(nsGkAtoms::path) &&
    nsSMILAnimationFunction::IsToAnimation();
}

void
SVGMotionSMILAnimationFunction::CheckKeyPoints()
{
  if (!HasAttr(nsGkAtoms::keyPoints))
    return;

  
  if (GetCalcMode() == CALC_PACED) {
    SetKeyPointsErrorFlag(false);
  }

  if (mKeyPoints.Length() != mKeyTimes.Length()) {
    
    SetKeyPointsErrorFlag(true);
    return;
  }

  
  
  
  
}

nsresult
SVGMotionSMILAnimationFunction::SetKeyPoints(const nsAString& aKeyPoints,
                                             nsAttrValue& aResult)
{
  mKeyPoints.Clear();
  aResult.SetTo(aKeyPoints);

  mHasChanged = true;

  if (!nsSMILParserUtils::ParseSemicolonDelimitedProgressList(aKeyPoints, false,
                                                              mKeyPoints)) {
    mKeyPoints.Clear();
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

void
SVGMotionSMILAnimationFunction::UnsetKeyPoints()
{
  mKeyPoints.Clear();
  SetKeyPointsErrorFlag(false);
  mHasChanged = true;
}

nsresult
SVGMotionSMILAnimationFunction::SetRotate(const nsAString& aRotate,
                                          nsAttrValue& aResult)
{
  mHasChanged = true;

  aResult.SetTo(aRotate);
  if (aRotate.EqualsLiteral("auto")) {
    mRotateType = eRotateType_Auto;
  } else if (aRotate.EqualsLiteral("auto-reverse")) {
    mRotateType = eRotateType_AutoReverse;
  } else {
    mRotateType = eRotateType_Explicit;

    
    nsSVGAngle svgAngle;
    svgAngle.Init();
    nsresult rv = svgAngle.SetBaseValueString(aRotate, nullptr, false);
    if (NS_FAILED(rv)) { 
      mRotateAngle = 0.0f; 
      
      return rv;
    }

    mRotateAngle = svgAngle.GetBaseValInSpecifiedUnits();

    
    uint8_t angleUnit = svgAngle.GetBaseValueUnit();
    if (angleUnit != SVG_ANGLETYPE_RAD) {
      mRotateAngle *= nsSVGAngle::GetDegreesPerUnit(angleUnit) /
        nsSVGAngle::GetDegreesPerUnit(SVG_ANGLETYPE_RAD);
    }
  }
  return NS_OK;
}

void
SVGMotionSMILAnimationFunction::UnsetRotate()
{
  mRotateAngle = 0.0f; 
  mRotateType = eRotateType_Explicit;
  mHasChanged = true;
}

} 
