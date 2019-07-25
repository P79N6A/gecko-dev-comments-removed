







































#ifndef MOZILLA_SVGMOTIONSMILPATHUTILS_H_
#define MOZILLA_SVGMOTIONSMILPATHUTILS_H_

#include "nsSMILParserUtils.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsDebug.h"
#include "gfxContext.h"
#include "nsSVGUtils.h"
#include "gfxPlatform.h"

class nsSVGElement;
class nsIContent;
class nsIDocument;
class nsAString;

namespace mozilla {

class SVGMotionSMILPathUtils {
public:
  
  
  class PathGenerator {
  public:
    PathGenerator(const nsSVGElement* aSVGElement)
      : mSVGElement(aSVGElement),
        mGfxContext(gfxPlatform::GetPlatform()->ScreenReferenceSurface()),
        mHaveReceivedCommands(PR_FALSE)
    {}

    
    
    
    
    
    void   MoveToOrigin();
    bool MoveToAbsolute(const nsAString& aCoordPairStr);
    bool LineToAbsolute(const nsAString& aCoordPairStr,
                          double& aSegmentDistance);
    bool LineToRelative(const nsAString& aCoordPairStr,
                          double& aSegmentDistance);

    
    inline bool HaveReceivedCommands() { return mHaveReceivedCommands; }
    
    already_AddRefed<gfxFlattenedPath> GetResultingPath();

  protected:
    
    bool ParseCoordinatePair(const nsAString& aStr,
                               float& aXVal, float& aYVal);

    
    const nsSVGElement* mSVGElement; 
    gfxContext    mGfxContext;
    bool          mHaveReceivedCommands;
  };

  
  
  class MotionValueParser : public nsSMILParserUtils::GenericValueParser
  {
  public:
    MotionValueParser(PathGenerator* aPathGenerator,
                      nsTArray<double>* aPointDistances)
      : mPathGenerator(aPathGenerator),
        mPointDistances(aPointDistances),
        mDistanceSoFar(0.0)
    {
      NS_ABORT_IF_FALSE(mPointDistances->IsEmpty(),
                        "expecting point distances array to start empty");
    }

    
    virtual nsresult Parse(const nsAString& aValueStr);

  protected:
    PathGenerator*    mPathGenerator;
    nsTArray<double>* mPointDistances;
    double            mDistanceSoFar;
  };

};

} 

#endif 
