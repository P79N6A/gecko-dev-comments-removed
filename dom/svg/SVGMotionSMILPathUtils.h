







#ifndef MOZILLA_SVGMOTIONSMILPATHUTILS_H_
#define MOZILLA_SVGMOTIONSMILPATHUTILS_H_

#include "mozilla/Attributes.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsDebug.h"
#include "nsSMILParserUtils.h"
#include "nsTArray.h"

class nsAString;
class nsSVGElement;

namespace mozilla {

class SVGMotionSMILPathUtils
{
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Path Path;
  typedef mozilla::gfx::PathBuilder PathBuilder;

public:
  
  
  class PathGenerator {
  public:
    explicit PathGenerator(const nsSVGElement* aSVGElement)
      : mSVGElement(aSVGElement),
        mHaveReceivedCommands(false)
    {
      RefPtr<DrawTarget> drawTarget =
        gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
      NS_ASSERTION(gfxPlatform::GetPlatform()->
                     SupportsAzureContentForDrawTarget(drawTarget),
                   "Should support Moz2D content drawing");
      
      mPathBuilder = drawTarget->CreatePathBuilder();
    }

    
    
    
    
    
    void   MoveToOrigin();
    bool MoveToAbsolute(const nsAString& aCoordPairStr);
    bool LineToAbsolute(const nsAString& aCoordPairStr,
                          double& aSegmentDistance);
    bool LineToRelative(const nsAString& aCoordPairStr,
                          double& aSegmentDistance);

    
    inline bool HaveReceivedCommands() { return mHaveReceivedCommands; }
    
    mozilla::TemporaryRef<Path> GetResultingPath();

  protected:
    
    bool ParseCoordinatePair(const nsAString& aStr,
                               float& aXVal, float& aYVal);

    
    const nsSVGElement* mSVGElement; 
    RefPtr<PathBuilder> mPathBuilder;
    bool          mHaveReceivedCommands;
  };

  
  
  class MOZ_STACK_CLASS MotionValueParser :
    public nsSMILParserUtils::GenericValueParser
  {
  public:
    MotionValueParser(PathGenerator* aPathGenerator,
                      FallibleTArray<double>* aPointDistances)
      : mPathGenerator(aPathGenerator),
        mPointDistances(aPointDistances),
        mDistanceSoFar(0.0)
    {
      NS_ABORT_IF_FALSE(mPointDistances->IsEmpty(),
                        "expecting point distances array to start empty");
    }

    
    virtual bool Parse(const nsAString& aValueStr) MOZ_OVERRIDE;

  protected:
    PathGenerator*          mPathGenerator;
    FallibleTArray<double>* mPointDistances;
    double                  mDistanceSoFar;
  };

};

} 

#endif 
