







































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
    PathGenerator(nsSVGElement* aSVGElement)
      : mSVGElement(aSVGElement),
        mGfxContext(gfxPlatform::GetPlatform()->ScreenReferenceSurface()),
        mHaveReceivedCommands(PR_FALSE)
    {}

    
    
    
    
    
    void   MoveToOrigin();
    PRBool MoveToAbsolute(const nsAString& aCoordPairStr);
    PRBool LineToAbsolute(const nsAString& aCoordPairStr,
                          double& aSegmentDistance);
    PRBool LineToRelative(const nsAString& aCoordPairStr,
                          double& aSegmentDistance);

    
    inline PRBool HaveReceivedCommands() { return mHaveReceivedCommands; }
    
    already_AddRefed<gfxFlattenedPath> GetResultingPath();

  protected:
    
    PRBool ParseCoordinatePair(const nsAString& aStr,
                               float& aXVal, float& aYVal);

    
    nsSVGElement* mSVGElement; 
    gfxContext    mGfxContext;
    PRPackedBool  mHaveReceivedCommands;
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
