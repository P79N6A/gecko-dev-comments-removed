










































#ifndef nsAttrValue_h___
#define nsAttrValue_h___

#include "nscore.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "nsColor.h"
#include "nsCaseTreatment.h"

typedef PRUptrdiff PtrBits;
class nsAString;
class nsIAtom;
class nsICSSStyleRule;
class nsIURI;
class nsISVGValue;
class nsIDocument;
template<class E> class nsCOMArray;
template<class E> class nsTPtrArray;

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
  nsAttrValue();
  nsAttrValue(const nsAttrValue& aOther);
  explicit nsAttrValue(const nsAString& aValue);
  explicit nsAttrValue(nsICSSStyleRule* aValue);
#ifdef MOZ_SVG
  explicit nsAttrValue(nsISVGValue* aValue);
#endif
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
    
    
    eCSSStyleRule = 0x10,
    eAtomArray =    0x11 
#ifdef MOZ_SVG
    ,eSVGValue =    0x12
#endif
    ,eFloatValue  = 0x13
    ,eLazyURIValue = 0x14
  };

  ValueType Type() const;

  void Reset();

  void SetTo(const nsAttrValue& aOther);
  void SetTo(const nsAString& aValue);
  void SetTo(PRInt16 aInt);
  void SetTo(nsICSSStyleRule* aValue);
#ifdef MOZ_SVG
  void SetTo(nsISVGValue* aValue);
#endif

  void SwapValueWith(nsAttrValue& aOther);

  void ToString(nsAString& aResult) const;

  
  
  inline PRBool IsEmptyString() const;
  const nsCheapString GetStringValue() const;
  inline nsIAtom* GetAtomValue() const;
  inline PRInt32 GetIntegerValue() const;
  PRBool GetColorValue(nscolor& aColor) const;
  inline PRInt16 GetEnumValue() const;
  inline float GetPercentValue() const;
  inline nsCOMArray<nsIAtom>* GetAtomArrayValue() const;
  inline nsICSSStyleRule* GetCSSStyleRuleValue() const;
#ifdef MOZ_SVG
  inline nsISVGValue* GetSVGValue() const;
#endif
  inline float GetFloatValue() const;
  inline nsIURI* GetURIValue() const;
  const nsCheapString GetURIStringValue() const;
  void CacheURIValue(nsIURI* aURI);
  void DropCachedURI();

  
  
  
  PRInt32 GetAtomCount() const;
  
  
  nsIAtom* AtomAt(PRInt32 aIndex) const;

  PRUint32 HashValue() const;
  PRBool Equals(const nsAttrValue& aOther) const;
  PRBool Equals(const nsAString& aValue, nsCaseTreatment aCaseSensitive) const;
  PRBool Equals(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const;

  



  PRBool Contains(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const;

  void ParseAtom(const nsAString& aValue);
  void ParseAtomArray(const nsAString& aValue);
  void ParseStringOrAtom(const nsAString& aValue);

  









  struct EnumTable {
    
    const char* tag;
    
    PRInt16 value;
  };

  







  PRBool ParseEnumValue(const nsAString& aValue,
                        const EnumTable* aTable,
                        PRBool aCaseSensitive = PR_FALSE);

  








  PRBool ParseSpecialIntValue(const nsAString& aString,
                              PRBool aCanBePercent);


  





  PRBool ParseIntValue(const nsAString& aString) {
    return ParseIntWithBounds(aString, PR_INT32_MIN, PR_INT32_MAX);
  }

  







  PRBool ParseIntWithBounds(const nsAString& aString, PRInt32 aMin,
                            PRInt32 aMax = PR_INT32_MAX);

  






  PRBool ParseColor(const nsAString& aString, nsIDocument* aDocument);

  





  PRBool ParseFloatValue(const nsAString& aString);

  



  PRBool ParseLazyURIValue(const nsAString& aString);

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
      nsICSSStyleRule* mCSSStyleRule;
      nsCOMArray<nsIAtom>* mAtomArray;
#ifdef MOZ_SVG
      nsISVGValue* mSVGValue;
#endif
      float mFloatValue;
      nsIURI* mURI;
    };
  };

  inline ValueBaseType BaseType() const;

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

  PRBool EnsureEmptyMiscContainer();
  PRBool EnsureEmptyAtomArray();
  nsStringBuffer* GetStringBuffer(const nsAString& aValue) const;
  
  
  PRInt32 StringToInteger(const nsAString& aValue,
                          PRBool* aStrict,
                          PRInt32* aErrorCode,
                          PRBool aCanBePercent = PR_FALSE,
                          PRBool* aIsPercent = nsnull) const;

  static nsTPtrArray<const EnumTable>* sEnumTableArray;

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

inline nsCOMArray<nsIAtom>*
nsAttrValue::GetAtomArrayValue() const
{
  NS_PRECONDITION(Type() == eAtomArray, "wrong type");
  return GetMiscContainer()->mAtomArray;
}

inline nsICSSStyleRule*
nsAttrValue::GetCSSStyleRuleValue() const
{
  NS_PRECONDITION(Type() == eCSSStyleRule, "wrong type");
  return GetMiscContainer()->mCSSStyleRule;
}

#ifdef MOZ_SVG
inline nsISVGValue*
nsAttrValue::GetSVGValue() const
{
  NS_PRECONDITION(Type() == eSVGValue, "wrong type");
  return GetMiscContainer()->mSVGValue;
}
#endif

inline float
nsAttrValue::GetFloatValue() const
{
  NS_PRECONDITION(Type() == eFloatValue, "wrong type");
  return GetMiscContainer()->mFloatValue;
}

inline nsIURI*
nsAttrValue::GetURIValue() const
{
  NS_PRECONDITION(Type() == eLazyURIValue, "wrong type");
  return GetMiscContainer()->mURI;
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

inline PRBool
nsAttrValue::IsEmptyString() const
{
  return !mBits;
}

#endif
