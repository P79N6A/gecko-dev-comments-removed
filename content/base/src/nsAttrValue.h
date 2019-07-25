










































#ifndef nsAttrValue_h___
#define nsAttrValue_h___

#include "nscore.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "nsColor.h"
#include "nsCaseTreatment.h"
#include "nsMargin.h"
#include "nsCOMPtr.h"

typedef PRUptrdiff PtrBits;
class nsAString;
class nsIAtom;
class nsIDocument;
template<class E, class A> class nsTArray;
struct nsTArrayDefaultAllocator;

namespace mozilla {
namespace css {
class StyleRule;
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
public:
  typedef nsTArray< nsCOMPtr<nsIAtom> > AtomArray;

  nsAttrValue();
  nsAttrValue(const nsAttrValue& aOther);
  explicit nsAttrValue(const nsAString& aValue);
  nsAttrValue(mozilla::css::StyleRule* aValue, const nsAString* aSerialized);
  explicit nsAttrValue(const nsIntMargin& aValue);
  ~nsAttrValue();

  static nsresult Init();
  static void Shutdown();

  
  enum ValueType {
    eString =       0x00, 
                          
    eAtom =         0x02, 
    eInteger =      0x03, 
    eColor =        0x07, 
    eEnum =         0x0B, 
    ePercent =      0x0F, 
    
    
    eCSSStyleRule =    0x10
    ,eAtomArray =      0x11 
    ,eDoubleValue  =   0x12
    ,eIntMarginValue = 0x13
  };

  ValueType Type() const;

  void Reset();

  void SetTo(const nsAttrValue& aOther);
  void SetTo(const nsAString& aValue);
  void SetTo(PRInt16 aInt);
  void SetTo(mozilla::css::StyleRule* aValue, const nsAString* aSerialized);
  void SetTo(const nsIntMargin& aValue);

  void SwapValueWith(nsAttrValue& aOther);

  void ToString(nsAString& aResult) const;

  
  
  inline bool IsEmptyString() const;
  const nsCheapString GetStringValue() const;
  inline nsIAtom* GetAtomValue() const;
  inline PRInt32 GetIntegerValue() const;
  bool GetColorValue(nscolor& aColor) const;
  inline PRInt16 GetEnumValue() const;
  inline float GetPercentValue() const;
  inline AtomArray* GetAtomArrayValue() const;
  inline mozilla::css::StyleRule* GetCSSStyleRuleValue() const;
  inline double GetDoubleValue() const;
  bool GetIntMarginValue(nsIntMargin& aMargin) const;

  





  void GetEnumString(nsAString& aResult, bool aRealTag) const;

  
  
  
  PRUint32 GetAtomCount() const;
  
  
  nsIAtom* AtomAt(PRInt32 aIndex) const;

  PRUint32 HashValue() const;
  bool Equals(const nsAttrValue& aOther) const;
  bool Equals(const nsAString& aValue, nsCaseTreatment aCaseSensitive) const;
  bool Equals(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const;

  



  bool Contains(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const;
  




  bool Contains(const nsAString& aValue) const;

  void ParseAtom(const nsAString& aValue);
  void ParseAtomArray(const nsAString& aValue);
  void ParseStringOrAtom(const nsAString& aValue);

  









  struct EnumTable {
    
    const char* tag;
    
    PRInt16 value;
  };

  










  bool ParseEnumValue(const nsAString& aValue,
                        const EnumTable* aTable,
                        bool aCaseSensitive,
                        const EnumTable* aDefaultValue = nsnull);

  









  bool ParseSpecialIntValue(const nsAString& aString);


  





  bool ParseIntValue(const nsAString& aString) {
    return ParseIntWithBounds(aString, PR_INT32_MIN, PR_INT32_MAX);
  }

  







  bool ParseIntWithBounds(const nsAString& aString, PRInt32 aMin,
                            PRInt32 aMax = PR_INT32_MAX);

  







  bool ParseNonNegativeIntValue(const nsAString& aString);

  












  bool ParsePositiveIntValue(const nsAString& aString);

  






  bool ParseColor(const nsAString& aString);

  





  bool ParseDoubleValue(const nsAString& aString);

  



  bool ParseLazyURIValue(const nsAString& aString);

  






  bool ParseIntMarginValue(const nsAString& aString);

  PRInt64 SizeOf() const;

private:
  
  enum ValueBaseType {
    eStringBase =    eString,    
    eOtherBase =     0x01,       
    eAtomBase =      eAtom,      
    eIntegerBase =   0x03        
  };

  struct MiscContainer
  {
    ValueType mType;
    
    
    
    
    PtrBits mStringBits;
    union {
      PRInt32 mInteger;
      nscolor mColor;
      PRUint32 mEnumValue;
      PRInt32 mPercent;
      mozilla::css::StyleRule* mCSSStyleRule;
      AtomArray* mAtomArray;
      double mDoubleValue;
      nsIntMargin* mIntMargin;
    };
  };

  inline ValueBaseType BaseType() const;

  






  PRInt16  GetEnumTableIndex(const EnumTable* aTable);

  inline void SetPtrValueAndType(void* aValue, ValueBaseType aType);
  void SetIntValueAndType(PRInt32 aValue, ValueType aType,
                          const nsAString* aStringValue);
  void SetColorValue(nscolor aColor, const nsAString& aString);
  void SetMiscAtomOrString(const nsAString* aValue);
  void ResetMiscAtomOrString();
  inline void ResetIfSet();

  inline void* GetPtr() const;
  inline MiscContainer* GetMiscContainer() const;
  inline PRInt32 GetIntInternal() const;

  bool EnsureEmptyMiscContainer();
  bool EnsureEmptyAtomArray();
  nsStringBuffer* GetStringBuffer(const nsAString& aValue) const;
  
  
  PRInt32 StringToInteger(const nsAString& aValue,
                          bool* aStrict,
                          PRInt32* aErrorCode,
                          bool aCanBePercent = false,
                          bool* aIsPercent = nsnull) const;
  
  
  PRInt32 EnumTableEntryToValue(const EnumTable* aEnumTable,
                                const EnumTable* aTableEntry);  

  static nsTArray<const EnumTable*, nsTArrayDefaultAllocator>* sEnumTableArray;

  PtrBits mBits;
};





inline nsIAtom*
nsAttrValue::GetAtomValue() const
{
  NS_PRECONDITION(Type() == eAtom, "wrong type");
  return reinterpret_cast<nsIAtom*>(GetPtr());
}

inline PRInt32
nsAttrValue::GetIntegerValue() const
{
  NS_PRECONDITION(Type() == eInteger, "wrong type");
  return (BaseType() == eIntegerBase)
         ? GetIntInternal()
         : GetMiscContainer()->mInteger;
}

inline PRInt16
nsAttrValue::GetEnumValue() const
{
  NS_PRECONDITION(Type() == eEnum, "wrong type");
  
  
  return static_cast<PRInt16>((
    (BaseType() == eIntegerBase)
    ? static_cast<PRUint32>(GetIntInternal())
    : GetMiscContainer()->mEnumValue)
      >> NS_ATTRVALUE_ENUMTABLEINDEX_BITS);
}

inline float
nsAttrValue::GetPercentValue() const
{
  NS_PRECONDITION(Type() == ePercent, "wrong type");
  return ((BaseType() == eIntegerBase)
          ? GetIntInternal()
          : GetMiscContainer()->mPercent)
            / 100.0f;
}

inline nsAttrValue::AtomArray*
nsAttrValue::GetAtomArrayValue() const
{
  NS_PRECONDITION(Type() == eAtomArray, "wrong type");
  return GetMiscContainer()->mAtomArray;
}

inline mozilla::css::StyleRule*
nsAttrValue::GetCSSStyleRuleValue() const
{
  NS_PRECONDITION(Type() == eCSSStyleRule, "wrong type");
  return GetMiscContainer()->mCSSStyleRule;
}

inline double
nsAttrValue::GetDoubleValue() const
{
  NS_PRECONDITION(Type() == eDoubleValue, "wrong type");
  return GetMiscContainer()->mDoubleValue;
}

inline bool
nsAttrValue::GetIntMarginValue(nsIntMargin& aMargin) const
{
  NS_PRECONDITION(Type() == eIntMarginValue, "wrong type");
  nsIntMargin* m = GetMiscContainer()->mIntMargin;
  if (!m)
    return false;
  aMargin = *m;
  return true;
}

inline nsAttrValue::ValueBaseType
nsAttrValue::BaseType() const
{
  return static_cast<ValueBaseType>(mBits & NS_ATTRVALUE_BASETYPE_MASK);
}

inline void
nsAttrValue::SetPtrValueAndType(void* aValue, ValueBaseType aType)
{
  NS_ASSERTION(!(NS_PTR_TO_INT32(aValue) & ~NS_ATTRVALUE_POINTERVALUE_MASK),
               "pointer not properly aligned, this will crash");
  mBits = reinterpret_cast<PtrBits>(aValue) | aType;
}

inline void
nsAttrValue::ResetIfSet()
{
  if (mBits) {
    Reset();
  }
}

inline void*
nsAttrValue::GetPtr() const
{
  NS_ASSERTION(BaseType() != eIntegerBase,
               "getting pointer from non-pointer");
  return reinterpret_cast<void*>(mBits & NS_ATTRVALUE_POINTERVALUE_MASK);
}

inline nsAttrValue::MiscContainer*
nsAttrValue::GetMiscContainer() const
{
  NS_ASSERTION(BaseType() == eOtherBase, "wrong type");
  return static_cast<MiscContainer*>(GetPtr());
}

inline PRInt32
nsAttrValue::GetIntInternal() const
{
  NS_ASSERTION(BaseType() == eIntegerBase,
               "getting integer from non-integer");
  
  
  
  return static_cast<PRInt32>(mBits & ~NS_ATTRVALUE_INTEGERTYPE_MASK) /
         NS_ATTRVALUE_INTEGERTYPE_MULTIPLIER;
}

inline bool
nsAttrValue::IsEmptyString() const
{
  return !mBits;
}

#endif
