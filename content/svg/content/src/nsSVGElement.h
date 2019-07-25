





































#ifndef __NS_SVGELEMENT_H__
#define __NS_SVGELEMENT_H__






#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIDOMSVGElement.h"
#include "nsGenericElement.h"
#include "nsStyledElement.h"
#include "nsISVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#include "nsSMILAnimationController.h"
#endif

class nsSVGSVGElement;
class nsSVGLength2;
class nsSVGNumber2;
class nsSVGInteger;
class nsSVGAngle;
class nsSVGBoolean;
class nsSVGEnum;
struct nsSVGEnumMapping;
class nsSVGViewBox;
class nsSVGString;
class nsSVGClass;
struct gfxMatrix;
namespace mozilla {
class SVGAnimatedNumberList;
class SVGNumberList;
class SVGAnimatedLengthList;
class SVGUserUnitList;
class SVGAnimatedPointList;
class SVGAnimatedPathSegList;
class SVGAnimatedPreserveAspectRatio;
}

typedef nsStyledElementNotElementCSSInlineStyle nsSVGElementBase;

class nsSVGElement : public nsSVGElementBase,    
                     public nsISVGValueObserver  
{
protected:
  nsSVGElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  nsresult Init();
  virtual ~nsSVGElement();

public:
  typedef mozilla::SVGNumberList SVGNumberList;
  typedef mozilla::SVGAnimatedNumberList SVGAnimatedNumberList;
  typedef mozilla::SVGUserUnitList SVGUserUnitList;
  typedef mozilla::SVGAnimatedLengthList SVGAnimatedLengthList;
  typedef mozilla::SVGAnimatedPointList SVGAnimatedPointList;
  typedef mozilla::SVGAnimatedPathSegList SVGAnimatedPathSegList;
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;

  
  NS_DECL_ISUPPORTS_INHERITED

  

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;

  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);

  static const MappedAttributeEntry sFillStrokeMap[];
  static const MappedAttributeEntry sGraphicsMap[];
  static const MappedAttributeEntry sTextContentElementsMap[];
  static const MappedAttributeEntry sFontSpecificationMap[];
  static const MappedAttributeEntry sGradientStopMap[];
  static const MappedAttributeEntry sViewportsMap[];
  static const MappedAttributeEntry sMarkersMap[];
  static const MappedAttributeEntry sColorMap[];
  static const MappedAttributeEntry sFiltersMap[];
  static const MappedAttributeEntry sFEFloodMap[];
  static const MappedAttributeEntry sLightingEffectsMap[];

  
  NS_IMETHOD IsSupported(const nsAString& aFeature, const nsAString& aVersion,
                         PRBool* aReturn);
  
  
  NS_IMETHOD GetId(nsAString & aId);
  NS_IMETHOD SetId(const nsAString & aId);
  NS_IMETHOD GetOwnerSVGElement(nsIDOMSVGSVGElement** aOwnerSVGElement);
  NS_IMETHOD GetViewportElement(nsIDOMSVGElement** aViewportElement);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  

  
  
  
  nsSVGSVGElement* GetCtx() const;

  



  virtual gfxMatrix PrependLocalTransformTo(const gfxMatrix &aMatrix) const;

  
  
  
  virtual void SetAnimateMotionTransform(const gfxMatrix* aMatrix) {}

  PRBool IsStringAnimatable(PRUint8 aAttrEnum) {
    return GetStringInfo().mStringInfo[aAttrEnum].mIsAnimatable;
  }
  PRBool NumberAttrAllowsPercentage(PRUint8 aAttrEnum) {
    return GetNumberInfo().mNumberInfo[aAttrEnum].mPercentagesAllowed;
  }
  virtual void DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeNumber(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeInteger(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeAngle(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeBoolean(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeEnum(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeViewBox(PRBool aDoSetAttr);
  virtual void DidChangePreserveAspectRatio(PRBool aDoSetAttr);
  virtual void DidChangeNumberList(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeLengthList(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangePointList(PRBool aDoSetAttr);
  virtual void DidChangePathSegList(PRBool aDoSetAttr);
  virtual void DidChangeString(PRUint8 aAttrEnum) {}

  virtual void DidAnimateLength(PRUint8 aAttrEnum);
  virtual void DidAnimateNumber(PRUint8 aAttrEnum);
  virtual void DidAnimateInteger(PRUint8 aAttrEnum);
  virtual void DidAnimateAngle(PRUint8 aAttrEnum);
  virtual void DidAnimateBoolean(PRUint8 aAttrEnum);
  virtual void DidAnimateEnum(PRUint8 aAttrEnum);
  virtual void DidAnimateViewBox();
  virtual void DidAnimatePreserveAspectRatio();
  virtual void DidAnimateNumberList(PRUint8 aAttrEnum);
  virtual void DidAnimateLengthList(PRUint8 aAttrEnum);
  virtual void DidAnimatePointList();
  virtual void DidAnimatePathSegList();
  virtual void DidAnimateTransform();
  virtual void DidAnimateString(PRUint8 aAttrEnum);
  virtual void DidAnimateClass();

  void GetAnimatedLengthValues(float *aFirst, ...);
  void GetAnimatedNumberValues(float *aFirst, ...);
  void GetAnimatedIntegerValues(PRInt32 *aFirst, ...);
  SVGAnimatedNumberList* GetAnimatedNumberList(PRUint8 aAttrEnum);
  SVGAnimatedNumberList* GetAnimatedNumberList(nsIAtom *aAttrName);
  void GetAnimatedLengthListValues(SVGUserUnitList *aFirst, ...);
  SVGAnimatedLengthList* GetAnimatedLengthList(PRUint8 aAttrEnum);
  virtual SVGAnimatedPointList* GetAnimatedPointList() {
    return nsnull;
  }
  virtual SVGAnimatedPathSegList* GetAnimPathSegList() {
    
    
    
    
    return nsnull;
  }

#ifdef MOZ_SMIL
  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 aNamespaceID, nsIAtom* aName);
  void AnimationNeedsResample();
  void FlushAnimations();
#else
  void AnimationNeedsResample() {  }
  void FlushAnimations() {  }
#endif

  virtual void RecompileScriptEventListeners();

  void GetStringBaseValue(PRUint8 aAttrEnum, nsAString& aResult) const;
  void SetStringBaseValue(PRUint8 aAttrEnum, const nsAString& aValue);

  virtual nsIAtom* GetPointListAttrName() const {
    return nsnull;
  }
  virtual nsIAtom* GetPathDataAttrName() const {
    return nsnull;
  }

protected:
  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult);
  static nsresult ReportAttributeParseFailure(nsIDocument* aDocument,
                                              nsIAtom* aAttribute,
                                              const nsAString& aValue);

  
  virtual PRBool IsEventName(nsIAtom* aName);

  void UpdateContentStyleRule();
#ifdef MOZ_SMIL
  void UpdateAnimatedContentStyleRule();
  mozilla::css::StyleRule* GetAnimatedContentStyleRule();
#endif 

  nsISVGValue* GetMappedAttribute(PRInt32 aNamespaceID, nsIAtom* aName);
  nsresult AddMappedSVGValue(nsIAtom* aName, nsISupports* aValue,
                             PRInt32 aNamespaceID = kNameSpaceID_None);
  
  static nsIAtom* GetEventNameForAttr(nsIAtom* aAttr);

  struct LengthInfo {
    nsIAtom** mName;
    float     mDefaultValue;
    PRUint8   mDefaultUnitType;
    PRUint8   mCtxType;
  };

  struct LengthAttributesInfo {
    nsSVGLength2* mLengths;
    LengthInfo*   mLengthInfo;
    PRUint32      mLengthCount;

    LengthAttributesInfo(nsSVGLength2 *aLengths,
                         LengthInfo *aLengthInfo,
                         PRUint32 aLengthCount) :
      mLengths(aLengths), mLengthInfo(aLengthInfo), mLengthCount(aLengthCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct NumberInfo {
    nsIAtom** mName;
    float     mDefaultValue;
    PRPackedBool mPercentagesAllowed;
  };

  struct NumberAttributesInfo {
    nsSVGNumber2* mNumbers;
    NumberInfo*   mNumberInfo;
    PRUint32      mNumberCount;

    NumberAttributesInfo(nsSVGNumber2 *aNumbers,
                         NumberInfo *aNumberInfo,
                         PRUint32 aNumberCount) :
      mNumbers(aNumbers), mNumberInfo(aNumberInfo), mNumberCount(aNumberCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct IntegerInfo {
    nsIAtom** mName;
    PRInt32   mDefaultValue;
  };

  struct IntegerAttributesInfo {
    nsSVGInteger* mIntegers;
    IntegerInfo*  mIntegerInfo;
    PRUint32      mIntegerCount;

    IntegerAttributesInfo(nsSVGInteger *aIntegers,
                          IntegerInfo *aIntegerInfo,
                          PRUint32 aIntegerCount) :
      mIntegers(aIntegers), mIntegerInfo(aIntegerInfo), mIntegerCount(aIntegerCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct AngleInfo {
    nsIAtom** mName;
    float     mDefaultValue;
    PRUint8   mDefaultUnitType;
  };

  struct AngleAttributesInfo {
    nsSVGAngle* mAngles;
    AngleInfo*  mAngleInfo;
    PRUint32    mAngleCount;

    AngleAttributesInfo(nsSVGAngle *aAngles,
                        AngleInfo *aAngleInfo,
                        PRUint32 aAngleCount) :
      mAngles(aAngles), mAngleInfo(aAngleInfo), mAngleCount(aAngleCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct BooleanInfo {
    nsIAtom**    mName;
    PRPackedBool mDefaultValue;
  };

  struct BooleanAttributesInfo {
    nsSVGBoolean* mBooleans;
    BooleanInfo*  mBooleanInfo;
    PRUint32      mBooleanCount;

    BooleanAttributesInfo(nsSVGBoolean *aBooleans,
                          BooleanInfo *aBooleanInfo,
                          PRUint32 aBooleanCount) :
      mBooleans(aBooleans), mBooleanInfo(aBooleanInfo), mBooleanCount(aBooleanCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  friend class nsSVGEnum;

  struct EnumInfo {
    nsIAtom**         mName;
    nsSVGEnumMapping* mMapping;
    PRUint16          mDefaultValue;
  };

  struct EnumAttributesInfo {
    nsSVGEnum* mEnums;
    EnumInfo*  mEnumInfo;
    PRUint32   mEnumCount;

    EnumAttributesInfo(nsSVGEnum *aEnums,
                       EnumInfo *aEnumInfo,
                       PRUint32 aEnumCount) :
      mEnums(aEnums), mEnumInfo(aEnumInfo), mEnumCount(aEnumCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct NumberListInfo {
    nsIAtom** mName;
  };

  struct NumberListAttributesInfo {
    SVGAnimatedNumberList* mNumberLists;
    NumberListInfo*        mNumberListInfo;
    PRUint32               mNumberListCount;

    NumberListAttributesInfo(SVGAnimatedNumberList *aNumberLists,
                             NumberListInfo *aNumberListInfo,
                             PRUint32 aNumberListCount)
      : mNumberLists(aNumberLists)
      , mNumberListInfo(aNumberListInfo)
      , mNumberListCount(aNumberListCount)
    {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct LengthListInfo {
    nsIAtom** mName;
    PRUint8   mAxis;
    







    PRPackedBool mCouldZeroPadList;
  };

  struct LengthListAttributesInfo {
    SVGAnimatedLengthList* mLengthLists;
    LengthListInfo*        mLengthListInfo;
    PRUint32               mLengthListCount;

    LengthListAttributesInfo(SVGAnimatedLengthList *aLengthLists,
                             LengthListInfo *aLengthListInfo,
                             PRUint32 aLengthListCount)
      : mLengthLists(aLengthLists)
      , mLengthListInfo(aLengthListInfo)
      , mLengthListCount(aLengthListCount)
    {}

    void Reset(PRUint8 aAttrEnum);
  };

  struct StringInfo {
    nsIAtom**    mName;
    PRInt32      mNamespaceID;
    PRPackedBool mIsAnimatable;
  };

  struct StringAttributesInfo {
    nsSVGString*  mStrings;
    StringInfo*   mStringInfo;
    PRUint32      mStringCount;

    StringAttributesInfo(nsSVGString *aStrings,
                         StringInfo *aStringInfo,
                         PRUint32 aStringCount) :
      mStrings(aStrings), mStringInfo(aStringInfo), mStringCount(aStringCount)
      {}

    void Reset(PRUint8 aAttrEnum);
  };

  virtual LengthAttributesInfo GetLengthInfo();
  virtual NumberAttributesInfo GetNumberInfo();
  virtual IntegerAttributesInfo GetIntegerInfo();
  virtual AngleAttributesInfo GetAngleInfo();
  virtual BooleanAttributesInfo GetBooleanInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  
  
  virtual nsSVGViewBox *GetViewBox();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual NumberListAttributesInfo GetNumberListInfo();
  virtual LengthListAttributesInfo GetLengthListInfo();
  virtual StringAttributesInfo GetStringInfo();
  virtual nsSVGClass *GetClass();

  static nsSVGEnumMapping sSVGUnitTypesMap[];

private:
  
  nsresult
  ParseNumberOptionalNumber(const nsAString& aValue,
                            PRUint32 aIndex1, PRUint32 aIndex2);

  
  nsresult
  ParseIntegerOptionalInteger(const nsAString& aValue,
                              PRUint32 aIndex1, PRUint32 aIndex2);

  void ResetOldStyleBaseType(nsISVGValue *svg_value);

  struct ObservableModificationData {
    
    
    
    ObservableModificationData(const nsAttrName* aName, PRUint32 aModType):
      name(aName), modType(aModType)
    {}
    const nsAttrName* name;
    PRUint8 modType;
  };
  ObservableModificationData
    GetModificationDataForObservable(nsISVGValue* aObservable,
                                     nsISVGValue::modificationType aModType);

  nsRefPtr<mozilla::css::StyleRule> mContentStyleRule;
  nsAttrAndChildArray mMappedAttributes;

  PRPackedBool mSuppressNotification;
};




#define NS_IMPL_NS_NEW_SVG_ELEMENT(_elementName)                             \
nsresult                                                                     \
NS_NewSVG##_elementName##Element(nsIContent **aResult,                       \
                                 already_AddRefed<nsINodeInfo> aNodeInfo)    \
{                                                                            \
  nsRefPtr<nsSVG##_elementName##Element> it =                                \
    new nsSVG##_elementName##Element(aNodeInfo);                             \
  if (!it)                                                                   \
    return NS_ERROR_OUT_OF_MEMORY;                                           \
                                                                             \
  nsresult rv = it->Init();                                                  \
                                                                             \
  if (NS_FAILED(rv)) {                                                       \
    return rv;                                                               \
  }                                                                          \
                                                                             \
  *aResult = it.forget().get();                                              \
                                                                             \
  return rv;                                                                 \
}

#define NS_IMPL_NS_NEW_SVG_ELEMENT_CHECK_PARSER(_elementName)                \
nsresult                                                                     \
NS_NewSVG##_elementName##Element(nsIContent **aResult,                       \
                                 already_AddRefed<nsINodeInfo> aNodeInfo,    \
                                 FromParser aFromParser)                     \
{                                                                            \
  nsRefPtr<nsSVG##_elementName##Element> it =                                \
    new nsSVG##_elementName##Element(aNodeInfo, aFromParser);                \
  if (!it)                                                                   \
    return NS_ERROR_OUT_OF_MEMORY;                                           \
                                                                             \
  nsresult rv = it->Init();                                                  \
                                                                             \
  if (NS_FAILED(rv)) {                                                       \
    return rv;                                                               \
  }                                                                          \
                                                                             \
  *aResult = it.forget().get();                                              \
                                                                             \
  return rv;                                                                 \
}

 

#define NS_SVG_VAL_IMPL_CYCLE_COLLECTION(_val, _element)                     \
NS_IMPL_CYCLE_COLLECTION_CLASS(_val)                                         \
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_val)                                \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(_element, nsIContent) \
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                                        \
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(_val)


#endif 
