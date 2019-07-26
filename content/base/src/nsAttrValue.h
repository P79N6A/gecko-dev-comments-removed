









#ifndef nsAttrValue_h___
#define nsAttrValue_h___

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsStringBuffer.h"
#include "nsColor.h"
#include "nsCaseTreatment.h"
#include "nsMargin.h"
#include "nsCOMPtr.h"
#include "SVGAttrValueWrapper.h"

typedef uintptr_t PtrBits;
class nsAString;
class nsIAtom;
class nsIDocument;
template<class E, class A> class nsTArray;
struct nsTArrayDefaultAllocator;
class nsStyledElementNotElementCSSInlineStyle;
struct MiscContainer;

namespace mozilla {
namespace css {
class StyleRule;
struct URLValue;
struct ImageValue;
}
}

#define NS_ATTRVALUE_MAX_STRINGLENGTH_ATOM 12

#define NS_ATTRVALUE_BASETYPE_MASK (PtrBits(3))
#define NS_ATTRVALUE_POINTERVALUE_MASK (~NS_ATTRVALUE_BASETYPE_MASK)

#define NS_ATTRVALUE_INTEGERTYPE_BITS 4
#define NS_ATTRVALUE_INTEGERTYPE_MASK (PtrBits((1 << NS_ATTRVALUE_INTEGERTYPE_BITS) - 1))
#define NS_ATTRVALUE_INTEGERTYPE_MULTIPLIER (1 << NS_ATTRVALUE_INTEGERTYPE_BITS)
#define NS_ATTRVALUE_INTEGERTYPE_MAXVALUE ((1 << (31 - NS_ATTRVALUE_INTEGERTYPE_BITS)) - 1)
#define NS_ATTRVALUE_INTEGERTYPE_MINVALUE (-NS_ATTRVALUE_INTEGERTYPE_MAXVALUE - 1)

#define NS_ATTRVALUE_ENUMTABLEINDEX_BITS (32 - 16 - NS_ATTRVALUE_INTEGERTYPE_BITS)
#define NS_ATTRVALUE_ENUMTABLE_VALUE_NEEDS_TO_UPPER (1 << (NS_ATTRVALUE_ENUMTABLEINDEX_BITS - 1))
#define NS_ATTRVALUE_ENUMTABLEINDEX_MAXVALUE (NS_ATTRVALUE_ENUMTABLE_VALUE_NEEDS_TO_UPPER - 1)
#define NS_ATTRVALUE_ENUMTABLEINDEX_MASK \
  (PtrBits((((1 << NS_ATTRVALUE_ENUMTABLEINDEX_BITS) - 1) &~ NS_ATTRVALUE_ENUMTABLE_VALUE_NEEDS_TO_UPPER)))





class nsCheapString : public nsString {
public:
  nsCheapString(nsStringBuffer* aBuf)
  {
    if (aBuf)
      aBuf->ToString(aBuf->StorageSize()/2 - 1, *this);
  }
};

class nsAttrValue {
  friend struct MiscContainer;
public:
  typedef nsTArray< nsCOMPtr<nsIAtom> > AtomArray;

  
  enum ValueType {
    eString =       0x00, 
                          
    eAtom =         0x02, 
    eInteger =      0x03, 
    eColor =        0x07, 
    eEnum =         0x0B, 
    ePercent =      0x0F, 
    
    
    eCSSStyleRule =    0x10
    ,eURL =            0x11
    ,eImage =          0x12
    ,eAtomArray =      0x13
    ,eDoubleValue  =   0x14
    ,eIntMarginValue = 0x15
    ,eSVGTypesBegin =  0x16
    ,eSVGAngle =       eSVGTypesBegin
    ,eSVGIntegerPair = 0x17
    ,eSVGLength =      0x18
    ,eSVGLengthList =  0x19
    ,eSVGNumberList =  0x20
    ,eSVGNumberPair =  0x21
    ,eSVGPathData   =  0x22
    ,eSVGPointList  =  0x23
    ,eSVGPreserveAspectRatio = 0x24
    ,eSVGStringList =  0x25
    ,eSVGTransformList = 0x26
    ,eSVGViewBox =     0x27
    ,eSVGTypesEnd =    0x34
  };

  nsAttrValue();
  nsAttrValue(const nsAttrValue& aOther);
  explicit nsAttrValue(const nsAString& aValue);
  explicit nsAttrValue(nsIAtom* aValue);
  nsAttrValue(mozilla::css::StyleRule* aValue, const nsAString* aSerialized);
  explicit nsAttrValue(const nsIntMargin& aValue);
  ~nsAttrValue();

  inline const nsAttrValue& operator=(const nsAttrValue& aOther);

  static nsresult Init();
  static void Shutdown();

  ValueType Type() const;

  void Reset();

  void SetTo(const nsAttrValue& aOther);
  void SetTo(const nsAString& aValue);
  void SetTo(nsIAtom* aValue);
  void SetTo(int16_t aInt);
  void SetTo(int32_t aInt, const nsAString* aSerialized);
  void SetTo(double aValue, const nsAString* aSerialized);
  void SetTo(mozilla::css::StyleRule* aValue, const nsAString* aSerialized);
  void SetTo(mozilla::css::URLValue* aValue, const nsAString* aSerialized);
  void SetTo(const nsIntMargin& aValue);
  void SetTo(const nsSVGAngle& aValue, const nsAString* aSerialized);
  void SetTo(const nsSVGIntegerPair& aValue, const nsAString* aSerialized);
  void SetTo(const nsSVGLength2& aValue, const nsAString* aSerialized);
  void SetTo(const mozilla::SVGLengthList& aValue,
             const nsAString* aSerialized);
  void SetTo(const mozilla::SVGNumberList& aValue,
             const nsAString* aSerialized);
  void SetTo(const nsSVGNumberPair& aValue, const nsAString* aSerialized);
  void SetTo(const mozilla::SVGPathData& aValue, const nsAString* aSerialized);
  void SetTo(const mozilla::SVGPointList& aValue, const nsAString* aSerialized);
  void SetTo(const mozilla::SVGAnimatedPreserveAspectRatio& aValue,
             const nsAString* aSerialized);
  void SetTo(const mozilla::SVGStringList& aValue,
             const nsAString* aSerialized);
  void SetTo(const mozilla::SVGTransformList& aValue,
             const nsAString* aSerialized);
  void SetTo(const nsSVGViewBox& aValue, const nsAString* aSerialized);

  






  void SetToSerialized(const nsAttrValue& aValue);

  void SwapValueWith(nsAttrValue& aOther);

  void ToString(nsAString& aResult) const;
  



  already_AddRefed<nsIAtom> GetAsAtom() const;

  
  
  inline bool IsEmptyString() const;
  const nsCheapString GetStringValue() const;
  inline nsIAtom* GetAtomValue() const;
  inline int32_t GetIntegerValue() const;
  bool GetColorValue(nscolor& aColor) const;
  inline int16_t GetEnumValue() const;
  inline float GetPercentValue() const;
  inline AtomArray* GetAtomArrayValue() const;
  inline mozilla::css::StyleRule* GetCSSStyleRuleValue() const;
  inline mozilla::css::URLValue* GetURLValue() const;
  inline mozilla::css::ImageValue* GetImageValue() const;
  inline double GetDoubleValue() const;
  bool GetIntMarginValue(nsIntMargin& aMargin) const;

  





  void GetEnumString(nsAString& aResult, bool aRealTag) const;

  
  
  
  uint32_t GetAtomCount() const;
  
  
  nsIAtom* AtomAt(int32_t aIndex) const;

  uint32_t HashValue() const;
  bool Equals(const nsAttrValue& aOther) const;
  bool Equals(const nsAString& aValue, nsCaseTreatment aCaseSensitive) const;
  bool Equals(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const;

  






  bool EqualsAsStrings(const nsAttrValue& aOther) const;

  



  bool Contains(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const;
  




  bool Contains(const nsAString& aValue) const;

  void ParseAtom(const nsAString& aValue);
  void ParseAtomArray(const nsAString& aValue);
  void ParseStringOrAtom(const nsAString& aValue);

  









  struct EnumTable {
    
    const char* tag;
    
    int16_t value;
  };

  










  bool ParseEnumValue(const nsAString& aValue,
                        const EnumTable* aTable,
                        bool aCaseSensitive,
                        const EnumTable* aDefaultValue = nullptr);

  









  bool ParseSpecialIntValue(const nsAString& aString);


  





  bool ParseIntValue(const nsAString& aString) {
    return ParseIntWithBounds(aString, INT32_MIN, INT32_MAX);
  }

  







  bool ParseIntWithBounds(const nsAString& aString, int32_t aMin,
                            int32_t aMax = INT32_MAX);

  







  bool ParseNonNegativeIntValue(const nsAString& aString);

  












  bool ParsePositiveIntValue(const nsAString& aString);

  






  bool ParseColor(const nsAString& aString);

  





  bool ParseDoubleValue(const nsAString& aString);

  



  bool ParseLazyURIValue(const nsAString& aString);

  






  bool ParseIntMarginValue(const nsAString& aString);

  




  void LoadImage(nsIDocument* aDocument);

  





  bool ParseStyleAttribute(const nsAString& aString,
                           nsStyledElementNotElementCSSInlineStyle* aElement);

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  
  enum ValueBaseType {
    eStringBase =    eString,    
    eOtherBase =     0x01,       
    eAtomBase =      eAtom,      
    eIntegerBase =   0x03        
  };

  inline ValueBaseType BaseType() const;
  inline bool IsSVGType(ValueType aType) const;

  






  int16_t  GetEnumTableIndex(const EnumTable* aTable);

  inline void SetPtrValueAndType(void* aValue, ValueBaseType aType);
  void SetIntValueAndType(int32_t aValue, ValueType aType,
                          const nsAString* aStringValue);
  void SetColorValue(nscolor aColor, const nsAString& aString);
  void SetMiscAtomOrString(const nsAString* aValue);
  void ResetMiscAtomOrString();
  void SetSVGType(ValueType aType, const void* aValue,
                  const nsAString* aSerialized);
  inline void ResetIfSet();

  inline void* GetPtr() const;
  inline MiscContainer* GetMiscContainer() const;
  inline int32_t GetIntInternal() const;

  
  
  MiscContainer* ClearMiscContainer();
  
  
  MiscContainer* EnsureEmptyMiscContainer();
  bool EnsureEmptyAtomArray();
  nsStringBuffer* GetStringBuffer(const nsAString& aValue) const;
  
  
  int32_t StringToInteger(const nsAString& aValue,
                          bool* aStrict,
                          nsresult* aErrorCode,
                          bool aCanBePercent = false,
                          bool* aIsPercent = nullptr) const;
  
  
  int32_t EnumTableEntryToValue(const EnumTable* aEnumTable,
                                const EnumTable* aTableEntry);  

  static nsTArray<const EnumTable*, nsTArrayDefaultAllocator>* sEnumTableArray;

  PtrBits mBits;
};

inline const nsAttrValue&
nsAttrValue::operator=(const nsAttrValue& aOther)
{
  SetTo(aOther);
  return *this;
}

inline nsIAtom*
nsAttrValue::GetAtomValue() const
{
  NS_PRECONDITION(Type() == eAtom, "wrong type");
  return reinterpret_cast<nsIAtom*>(GetPtr());
}

inline nsAttrValue::ValueBaseType
nsAttrValue::BaseType() const
{
  return static_cast<ValueBaseType>(mBits & NS_ATTRVALUE_BASETYPE_MASK);
}

inline void*
nsAttrValue::GetPtr() const
{
  NS_ASSERTION(BaseType() != eIntegerBase,
               "getting pointer from non-pointer");
  return reinterpret_cast<void*>(mBits & NS_ATTRVALUE_POINTERVALUE_MASK);
}

inline bool
nsAttrValue::IsEmptyString() const
{
  return !mBits;
}

#endif
