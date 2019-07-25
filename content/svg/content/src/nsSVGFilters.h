



































#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsIFrame.h"
#include "gfxRect.h"
#include "gfxImageSurface.h"
#include "nsIDOMSVGFilters.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMSVGURIReference.h"
#include "SVGAnimatedPreserveAspectRatio.h"

class nsSVGFilterResource;
class nsSVGNumberPair;
class nsSVGFilterInstance;

struct nsSVGStringInfo {
  nsSVGStringInfo(const nsSVGString* aString,
                  nsSVGElement *aElement) :
    mString(aString), mElement(aElement) {}

  const nsSVGString* mString;
  nsSVGElement* mElement;
};

typedef nsSVGStylableElement nsSVGFEBase;

#define NS_SVG_FE_CID \
{ 0x60483958, 0xd229, 0x4a77, \
  { 0x96, 0xb2, 0x62, 0x3e, 0x69, 0x95, 0x1e, 0x0e } }






class nsSVGFE : public nsSVGFEBase

{
  friend class nsSVGFilterInstance;

public:
  class ColorModel {
  public:
    enum ColorSpace { SRGB, LINEAR_RGB };
    enum AlphaChannel { UNPREMULTIPLIED, PREMULTIPLIED };

    ColorModel(ColorSpace aColorSpace, AlphaChannel aAlphaChannel) :
      mColorSpace(aColorSpace), mAlphaChannel(aAlphaChannel) {}
    ColorModel() :
      mColorSpace(SRGB), mAlphaChannel(PREMULTIPLIED) {}
    bool operator==(const ColorModel& aOther) const {
      return mColorSpace == aOther.mColorSpace &&
             mAlphaChannel == aOther.mAlphaChannel;
    }
    ColorSpace   mColorSpace;
    AlphaChannel mAlphaChannel;
  };

  struct Image {
    
    nsRefPtr<gfxImageSurface> mImage;
    
    gfxRect                   mFilterPrimitiveSubregion;
    ColorModel                mColorModel;
    
    bool                      mConstantColorChannels;
    
    Image() : mConstantColorChannels(false) {}
  };

protected:
  nsSVGFE(already_AddRefed<nsINodeInfo> aNodeInfo) : nsSVGFEBase(aNodeInfo) {}

  struct ScaleInfo {
    nsRefPtr<gfxImageSurface> mRealTarget;
    nsRefPtr<gfxImageSurface> mSource;
    nsRefPtr<gfxImageSurface> mTarget;
    nsIntRect mDataRect; 
    bool mRescaling;
  };

  ScaleInfo SetupScalingFilter(nsSVGFilterInstance *aInstance,
                               const Image *aSource,
                               const Image *aTarget,
                               const nsIntRect& aDataRect,
                               nsSVGNumberPair *aUnit);

  void FinishScalingFilter(ScaleInfo *aScaleInfo);

public:
  ColorModel
  GetInputColorModel(nsSVGFilterInstance* aInstance, PRInt32 aInputIndex,
                     Image* aImage) {
    return ColorModel(
          (OperatesOnSRGB(aInstance, aInputIndex, aImage) ?
             ColorModel::SRGB : ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha(aInputIndex) ?
             ColorModel::PREMULTIPLIED : ColorModel::UNPREMULTIPLIED));
  }

  ColorModel
  GetOutputColorModel(nsSVGFilterInstance* aInstance) {
    return ColorModel(
          (OperatesOnSRGB(aInstance, -1, nsnull) ?
             ColorModel::SRGB : ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha(-1) ?
             ColorModel::PREMULTIPLIED : ColorModel::UNPREMULTIPLIED));
  }

  
  virtual bool SubregionIsUnionOfRegions() { return true; }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVG_FE_CID)
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsSVGString& GetResultImageName() = 0;
  
  
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  
  
  
  
  
  
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  
  
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect) = 0;

  
  
  virtual bool AttributeAffectsRendering(
          PRInt32 aNameSpaceID, nsIAtom* aAttribute) const;

  static nsIntRect GetMaxRect() {
    
    
    return nsIntRect(PR_INT32_MIN/2, PR_INT32_MIN/2, PR_INT32_MAX, PR_INT32_MAX);
  }

  operator nsISupports*() { return static_cast<nsIContent*>(this); }
  
protected:
  virtual bool OperatesOnPremultipledAlpha(PRInt32) { return true; }

  
  
  
  virtual bool OperatesOnSRGB(nsSVGFilterInstance* aInstance,
                                PRInt32 aInputIndex, Image* aImage) {
    nsIFrame* frame = GetPrimaryFrame();
    if (!frame) return false;

    nsStyleContext* style = frame->GetStyleContext();
    return style->GetStyleSVG()->mColorInterpolationFilters ==
             NS_STYLE_COLOR_INTERPOLATION_SRGB;
  }

  
  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

typedef nsSVGFE nsSVGFEImageElementBase;

class nsSVGFEImageElement : public nsSVGFEImageElementBase,
                            public nsIDOMSVGFEImageElement,
                            public nsIDOMSVGURIReference,
                            public nsImageLoadingContent
{
  friend class SVGFEImageFrame;

protected:
  friend nsresult NS_NewSVGFEImageElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGFEImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsSVGFEImageElement();

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEImageElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          PRInt32 aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_DECL_NSIDOMSVGFEIMAGEELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEImageElementBase::)

  NS_FORWARD_NSIDOMNODE(nsSVGFEImageElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGFEImageElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, bool aNotify);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual nsEventStates IntrinsicState() const;

  
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest, nsresult status,
                          const PRUnichar *statusArg);
  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer,
                          const nsIntRect *aDirtyRect);
  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest,
                              imgIContainer *aContainer);

  void MaybeLoadSVGImage();

  virtual nsXPCClassInfo* GetClassInfo();
private:
  
  void Invalidate();

  nsresult LoadSVGImage(bool aForce, bool aNotify);

protected:
  virtual bool OperatesOnSRGB(nsSVGFilterInstance*,
                                PRInt32, Image*) { return true; }

  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT, HREF };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

typedef nsSVGElement SVGFEUnstyledElementBase;

class SVGFEUnstyledElement : public SVGFEUnstyledElementBase
{
protected:
  SVGFEUnstyledElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEUnstyledElementBase(aNodeInfo) {}

public:
  
  
  virtual bool AttributeAffectsRendering(
          PRInt32 aNameSpaceID, nsIAtom* aAttribute) const = 0;
};

#endif
