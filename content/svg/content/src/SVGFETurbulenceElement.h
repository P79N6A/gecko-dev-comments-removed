




#ifndef mozilla_dom_SVGFETurbulenceElement_h
#define mozilla_dom_SVGFETurbulenceElement_h

#include "nsSVGEnum.h"
#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"
#include "nsSVGInteger.h"
#include "nsSVGString.h"

nsresult NS_NewSVGFETurbulenceElement(nsIContent **aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFETurbulenceElementBase;

class SVGFETurbulenceElement : public SVGFETurbulenceElementBase,
                               public nsIDOMSVGElement
{
  friend nsresult (::NS_NewSVGFETurbulenceElement(nsIContent **aResult,
                                                  already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFETurbulenceElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFETurbulenceElementBase(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_FORWARD_NSIDOMSVGELEMENT(SVGFETurbulenceElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedNumber> BaseFrequencyX();
  already_AddRefed<nsIDOMSVGAnimatedNumber> BaseFrequencyY();
  already_AddRefed<nsIDOMSVGAnimatedInteger> NumOctaves();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Seed();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> StitchTiles();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> Type();

protected:
  virtual NumberAttributesInfo GetNumberInfo();
  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual IntegerAttributesInfo GetIntegerInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { SEED }; 
  nsSVGNumber2 mNumberAttributes[1];
  static NumberInfo sNumberInfo[1];

  enum { BASE_FREQ };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { OCTAVES };
  nsSVGInteger mIntegerAttributes[1];
  static IntegerInfo sIntegerInfo[1];

  enum { TYPE, STITCHTILES };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sTypeMap[];
  static nsSVGEnumMapping sStitchTilesMap[];
  static EnumInfo sEnumInfo[2];

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

private:

  




  






#define RAND_M 2147483647	/* 2**31 - 1 */
#define RAND_A 16807		/* 7**5; primitive root of m */
#define RAND_Q 127773		/* m / a */
#define RAND_R 2836		/* m % a */

  int32_t SetupSeed(int32_t aSeed) {
    if (aSeed <= 0)
      aSeed = -(aSeed % (RAND_M - 1)) + 1;
    if (aSeed > RAND_M - 1)
      aSeed = RAND_M - 1;
    return aSeed;
  }

  uint32_t Random(uint32_t aSeed) {
    int32_t result = RAND_A * (aSeed % RAND_Q) - RAND_R * (aSeed / RAND_Q);
    if (result <= 0)
      result += RAND_M;
    return result;
  }
#undef RAND_M
#undef RAND_A
#undef RAND_Q
#undef RAND_R

  const static int sBSize = 0x100;
  const static int sBM = 0xff;
  const static int sPerlinN = 0x1000;
  const static int sNP = 12;			
  const static int sNM = 0xfff;

  int32_t mLatticeSelector[sBSize + sBSize + 2];
  double mGradient[4][sBSize + sBSize + 2][2];
  struct StitchInfo {
    int mWidth;			
    int mHeight;
    int mWrapX;			
    int mWrapY;
  };

  void InitSeed(int32_t aSeed);
  double Noise2(int aColorChannel, double aVec[2], StitchInfo *aStitchInfo);
  double
  Turbulence(int aColorChannel, double *aPoint, double aBaseFreqX,
             double aBaseFreqY, int aNumOctaves, bool aFractalSum,
             bool aDoStitching, double aTileX, double aTileY,
             double aTileWidth, double aTileHeight);
};

} 
} 

#endif 
