




#ifndef mozilla_dom_SVGFEColorMatrixElement_h
#define mozilla_dom_SVGFEColorMatrixElement_h

typedef nsSVGFE nsSVGFEColorMatrixElementBase;

namespace mozilla {
namespace dom {

class nsSVGFEColorMatrixElement : public nsSVGFEColorMatrixElementBase,
                                  public nsIDOMSVGFEColorMatrixElement
{
  friend nsresult NS_NewSVGFEColorMatrixElement(nsIContent **aResult,
                                                already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEColorMatrixElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEColorMatrixElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEColorMatrixElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);

  
  NS_DECL_NSIDOMSVGFECOLORMATRIXELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEColorMatrixElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnPremultipledAlpha(int32_t) { return false; }

  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();
  virtual NumberListAttributesInfo GetNumberListInfo();

  enum { TYPE };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sTypeMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  enum { VALUES };
  SVGAnimatedNumberList mNumberListAttributes[1];
  static NumberListInfo sNumberListInfo[1];
};

} 
} 

#endif 
