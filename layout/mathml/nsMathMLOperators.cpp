







































#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsHashtable.h"
#include "nsTArray.h"

#include "nsIComponentManager.h"
#include "nsIPersistentProperties2.h"
#include "nsNetUtil.h"
#include "nsCRT.h"

#include "nsMathMLOperators.h"


struct OperatorData {
  OperatorData(void)
    : mFlags(0),
      mLeftSpace(0.0f),
      mRightSpace(0.0f)
  {
  }

  
  nsString        mStr;
  nsOperatorFlags mFlags;
  float           mLeftSpace;   
  float           mRightSpace;  
};

static PRInt32         gTableRefCount = 0;
static PRUint32        gOperatorCount = 0;
static OperatorData*   gOperatorArray = nsnull;
static nsHashtable*    gOperatorTable = nsnull;
static bool            gInitialized   = false;
static nsTArray<nsString>*      gInvariantCharArray    = nsnull;

static const PRUnichar kNullCh  = PRUnichar('\0');
static const PRUnichar kDashCh  = PRUnichar('#');
static const PRUnichar kColonCh = PRUnichar(':');

static const char* const kMathVariant_name[] = {
  "normal",
  "bold",
  "italic",
  "bold-italic",
  "sans-serif",
  "bold-sans-serif",
  "sans-serif-italic",
  "sans-serif-bold-italic",
  "monospace",
  "script",
  "bold-script",
  "fraktur",
  "bold-fraktur",
  "double-struck"
};

static void
SetBooleanProperty(OperatorData* aOperatorData,
                   nsString      aName)
{
  if (aName.IsEmpty())
    return;

  if (aName.EqualsLiteral("stretchy") && (1 == aOperatorData->mStr.Length()))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_STRETCHY;
  else if (aName.EqualsLiteral("fence"))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_FENCE;
  else if (aName.EqualsLiteral("accent"))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_ACCENT;
  else if (aName.EqualsLiteral("largeop"))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_LARGEOP;
  else if (aName.EqualsLiteral("separator"))
    aOperatorData->mFlags |=  NS_MATHML_OPERATOR_SEPARATOR;
  else if (aName.EqualsLiteral("movablelimits"))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_MOVABLELIMITS;
  else if (aName.EqualsLiteral("symmetric"))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_SYMMETRIC;
  else if (aName.EqualsLiteral("integral"))
    aOperatorData->mFlags |= NS_MATHML_OPERATOR_INTEGRAL;
}

static void
SetProperty(OperatorData* aOperatorData,
            nsString      aName,
            nsString      aValue)
{
  if (aName.IsEmpty() || aValue.IsEmpty())
    return;

  
  
  
  

  if (aName.EqualsLiteral("direction")) {
    if (aValue.EqualsLiteral("vertical"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_DIRECTION_VERTICAL;
    else if (aValue.EqualsLiteral("horizontal"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_DIRECTION_HORIZONTAL;
    else return; 
  } else {
    bool isLeftSpace;
    if (aName.EqualsLiteral("lspace"))
      isLeftSpace = true;
    else if (aName.EqualsLiteral("rspace"))
      isLeftSpace = false;
    else return;  

    
    PRInt32 error = 0;
    float space = aValue.ToFloat(&error) / 18.0;
    if (error) return;

    if (isLeftSpace)
      aOperatorData->mLeftSpace = space;
    else
      aOperatorData->mRightSpace = space;
  }
}

static bool
SetOperator(OperatorData*   aOperatorData,
            nsOperatorFlags aForm,
            const nsCString& aOperator,
            nsString&        aAttributes)

{
  
  
  PRInt32 i = 0;
  nsAutoString name, value;
  PRInt32 len = aOperator.Length();
  PRUnichar c = aOperator[i++];
  PRUint32 state  = 0;
  PRUnichar uchar = 0;
  while (i <= len) {
    if (0 == state) {
      if (c != '\\')
        return false;
      if (i < len)
        c = aOperator[i];
      i++;
      if (('u' != c) && ('U' != c))
        return false;
      if (i < len)
        c = aOperator[i];
      i++;
      state++;
    }
    else {
      if (('0' <= c) && (c <= '9'))
         uchar = (uchar << 4) | (c - '0');
      else if (('a' <= c) && (c <= 'f'))
         uchar = (uchar << 4) | (c - 'a' + 0x0a);
      else if (('A' <= c) && (c <= 'F'))
         uchar = (uchar << 4) | (c - 'A' + 0x0a);
      else return false;
      if (i < len)
        c = aOperator[i];
      i++;
      state++;
      if (5 == state) {
        value.Append(uchar);
        uchar = 0;
        state = 0;
      }
    }
  }
  if (0 != state) return false;

  
  
  
  if (!aForm) return true;

  
  aOperatorData->mFlags |= aForm;
  aOperatorData->mStr.Assign(value);
  value.AppendInt(aForm, 10);
  nsStringKey key(value);
  gOperatorTable->Put(&key, aOperatorData);

#ifdef NS_DEBUG
  NS_LossyConvertUTF16toASCII str(aAttributes);
#endif
  
  aAttributes.Append(kNullCh);  
  PRUnichar* start = aAttributes.BeginWriting();
  PRUnichar* end   = start;
  while ((kNullCh != *start) && (kDashCh != *start)) {
    name.SetLength(0);
    value.SetLength(0);
    
    while ((kNullCh!=*start) && (kDashCh!=*start) && nsCRT::IsAsciiSpace(*start)) {
      ++start;
    }
    end = start;
    
    while ((kNullCh!=*end) && (kDashCh!=*end) && !nsCRT::IsAsciiSpace(*end) &&
           (kColonCh!=*end)) {
      ++end;
    }
    
    bool IsBooleanProperty = (kColonCh != *end);
    *end = kNullCh; 
    
    if (start < end) {
      name.Assign(start);
    }
    if (IsBooleanProperty) {
      SetBooleanProperty(aOperatorData, name);
    } else {
      start = ++end;
      
      while ((kNullCh!=*end) && (kDashCh!=*end) &&
             !nsCRT::IsAsciiSpace(*end)) {
        ++end;
      }
      *end = kNullCh; 
      if (start < end) {
        
        value.Assign(start);
      }
      SetProperty(aOperatorData, name, value);
    }
    start = ++end;
  }
  return true;
}

static nsresult
InitOperators(void)
{
  
  nsresult rv;
  nsCOMPtr<nsIPersistentProperties> mathfontProp;
  rv = NS_LoadPersistentPropertiesFromURISpec(getter_AddRefs(mathfontProp),
       NS_LITERAL_CSTRING("resource://gre/res/fonts/mathfont.properties"));
  if (NS_FAILED(rv)) return rv;

  
  for (PRInt32 i = 0; i < eMATHVARIANT_COUNT; ++i) {
    nsCAutoString key(NS_LITERAL_CSTRING("mathvariant."));
    key.Append(kMathVariant_name[i]);
    nsAutoString value;
    mathfontProp->GetStringProperty(key, value);
    gInvariantCharArray->AppendElement(value); 
  }

  
  
  
  for (PRInt32 pass = 1; pass <= 2; pass++) {
    OperatorData dummyData;
    OperatorData* operatorData = &dummyData;
    nsCOMPtr<nsISimpleEnumerator> iterator;
    if (NS_SUCCEEDED(mathfontProp->Enumerate(getter_AddRefs(iterator)))) {
      bool more;
      PRUint32 index = 0;
      nsCAutoString name;
      nsAutoString attributes;
      while ((NS_SUCCEEDED(iterator->HasMoreElements(&more))) && more) {
        nsCOMPtr<nsIPropertyElement> element;
        if (NS_SUCCEEDED(iterator->GetNext(getter_AddRefs(element)))) {
          if (NS_SUCCEEDED(element->GetKey(name)) &&
              NS_SUCCEEDED(element->GetValue(attributes))) {
            
            if ((21 <= name.Length()) && (0 == name.Find("operator.\\u"))) {
              name.Cut(0, 9); 
              PRInt32 len = name.Length();
              nsOperatorFlags form = 0;
              if (kNotFound != name.RFind(".infix")) {
                form = NS_MATHML_OPERATOR_FORM_INFIX;
                len -= 6;  
              }
              else if (kNotFound != name.RFind(".postfix")) {
                form = NS_MATHML_OPERATOR_FORM_POSTFIX;
                len -= 8; 
              }
              else if (kNotFound != name.RFind(".prefix")) {
                form = NS_MATHML_OPERATOR_FORM_PREFIX;
                len -= 7; 
              }
              else continue; 
              name.SetLength(len);
              if (2 == pass) { 
                if (!gOperatorArray) {
                  if (0 == gOperatorCount) return NS_ERROR_UNEXPECTED;
                  gOperatorArray = new OperatorData[gOperatorCount];
                  if (!gOperatorArray) return NS_ERROR_OUT_OF_MEMORY;
                }
                operatorData = &gOperatorArray[index];
              }
              else {
                form = 0; 
              }
              
              if (SetOperator(operatorData, form, name, attributes)) {
                index++;
                if (1 == pass) gOperatorCount = index;
              }
            }
          }
        }
      }
    }
  }
  return NS_OK;
}

static nsresult
InitGlobals()
{
  gInitialized = true;
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  gInvariantCharArray = new nsTArray<nsString>();
  if (gInvariantCharArray) {
    gOperatorTable = new nsHashtable();
    if (gOperatorTable) {
      rv = InitOperators();
    }
  }
  if (NS_FAILED(rv))
    nsMathMLOperators::CleanUp();
  return rv;
}

void
nsMathMLOperators::CleanUp()
{
  if (gInvariantCharArray) {
    delete gInvariantCharArray;
    gInvariantCharArray = nsnull;
  }
  if (gOperatorArray) {
    delete[] gOperatorArray;
    gOperatorArray = nsnull;
  }
  if (gOperatorTable) {
    delete gOperatorTable;
    gOperatorTable = nsnull;
  }
}

void
nsMathMLOperators::AddRefTable(void)
{
  gTableRefCount++;
}

void
nsMathMLOperators::ReleaseTable(void)
{
  if (0 == --gTableRefCount) {
    CleanUp();
  }
}

static OperatorData*
GetOperatorData(const nsString& aOperator, nsOperatorFlags aForm)
{
  nsAutoString key(aOperator);
  key.AppendInt(aForm);
  nsStringKey hkey(key);
  return (OperatorData*)gOperatorTable->Get(&hkey);
}

bool
nsMathMLOperators::LookupOperator(const nsString&       aOperator,
                                  const nsOperatorFlags aForm,
                                  nsOperatorFlags*      aFlags,
                                  float*                aLeftSpace,
                                  float*                aRightSpace)
{
  if (!gInitialized) {
    InitGlobals();
  }
  if (gOperatorTable) {
    NS_ASSERTION(aFlags && aLeftSpace && aRightSpace, "bad usage");
    NS_ASSERTION(aForm > 0 && aForm < 4, "*** invalid call ***");

    
    
    
    

    OperatorData* found;
    PRInt32 form = NS_MATHML_OPERATOR_GET_FORM(aForm);
    if (!(found = GetOperatorData(aOperator, form))) {
      if (form == NS_MATHML_OPERATOR_FORM_INFIX ||
          !(found =
            GetOperatorData(aOperator, NS_MATHML_OPERATOR_FORM_INFIX))) {
        if (form == NS_MATHML_OPERATOR_FORM_POSTFIX ||
            !(found =
              GetOperatorData(aOperator, NS_MATHML_OPERATOR_FORM_POSTFIX))) {
          if (form != NS_MATHML_OPERATOR_FORM_PREFIX) {
            found = GetOperatorData(aOperator, NS_MATHML_OPERATOR_FORM_PREFIX);
          }
        }
      }
    }
    if (found) {
      NS_ASSERTION(found->mStr.Equals(aOperator), "bad setup");
      *aLeftSpace = found->mLeftSpace;
      *aRightSpace = found->mRightSpace;
      *aFlags &= ~NS_MATHML_OPERATOR_FORM; 
      *aFlags |= found->mFlags; 
      return true;
    }
  }
  return false;
}

void
nsMathMLOperators::LookupOperators(const nsString&       aOperator,
                                   nsOperatorFlags*      aFlags,
                                   float*                aLeftSpace,
                                   float*                aRightSpace)
{
  if (!gInitialized) {
    InitGlobals();
  }

  aFlags[NS_MATHML_OPERATOR_FORM_INFIX] = 0;
  aLeftSpace[NS_MATHML_OPERATOR_FORM_INFIX] = 0.0f;
  aRightSpace[NS_MATHML_OPERATOR_FORM_INFIX] = 0.0f;

  aFlags[NS_MATHML_OPERATOR_FORM_POSTFIX] = 0;
  aLeftSpace[NS_MATHML_OPERATOR_FORM_POSTFIX] = 0.0f;
  aRightSpace[NS_MATHML_OPERATOR_FORM_POSTFIX] = 0.0f;

  aFlags[NS_MATHML_OPERATOR_FORM_PREFIX] = 0;
  aLeftSpace[NS_MATHML_OPERATOR_FORM_PREFIX] = 0.0f;
  aRightSpace[NS_MATHML_OPERATOR_FORM_PREFIX] = 0.0f;

  if (gOperatorTable) {
    OperatorData* found;
    found = GetOperatorData(aOperator, NS_MATHML_OPERATOR_FORM_INFIX);
    if (found) {
      aFlags[NS_MATHML_OPERATOR_FORM_INFIX] = found->mFlags;
      aLeftSpace[NS_MATHML_OPERATOR_FORM_INFIX] = found->mLeftSpace;
      aRightSpace[NS_MATHML_OPERATOR_FORM_INFIX] = found->mRightSpace;
    }
    found = GetOperatorData(aOperator, NS_MATHML_OPERATOR_FORM_POSTFIX);
    if (found) {
      aFlags[NS_MATHML_OPERATOR_FORM_POSTFIX] = found->mFlags;
      aLeftSpace[NS_MATHML_OPERATOR_FORM_POSTFIX] = found->mLeftSpace;
      aRightSpace[NS_MATHML_OPERATOR_FORM_POSTFIX] = found->mRightSpace;
    }
    found = GetOperatorData(aOperator, NS_MATHML_OPERATOR_FORM_PREFIX);
    if (found) {
      aFlags[NS_MATHML_OPERATOR_FORM_PREFIX] = found->mFlags;
      aLeftSpace[NS_MATHML_OPERATOR_FORM_PREFIX] = found->mLeftSpace;
      aRightSpace[NS_MATHML_OPERATOR_FORM_PREFIX] = found->mRightSpace;
    }
  }
}

bool
nsMathMLOperators::IsMutableOperator(const nsString& aOperator)
{
  if (!gInitialized) {
    InitGlobals();
  }
  
  
  nsOperatorFlags flags[4];
  float lspace[4], rspace[4];
  nsMathMLOperators::LookupOperators(aOperator, flags, lspace, rspace);
  nsOperatorFlags allFlags =
    flags[NS_MATHML_OPERATOR_FORM_INFIX] |
    flags[NS_MATHML_OPERATOR_FORM_POSTFIX] |
    flags[NS_MATHML_OPERATOR_FORM_PREFIX];
  return NS_MATHML_OPERATOR_IS_STRETCHY(allFlags) ||
         NS_MATHML_OPERATOR_IS_LARGEOP(allFlags);
}

 nsStretchDirection
nsMathMLOperators::GetStretchyDirection(const nsString& aOperator)
{
  
  
  
  nsOperatorFlags flags = 0;
  float dummy;
  nsMathMLOperators::LookupOperator(aOperator,
                                    NS_MATHML_OPERATOR_FORM_INFIX,
                                    &flags, &dummy, &dummy);

  if (NS_MATHML_OPERATOR_IS_DIRECTION_VERTICAL(flags)) {
      return NS_STRETCH_DIRECTION_VERTICAL;
  } else if (NS_MATHML_OPERATOR_IS_DIRECTION_HORIZONTAL(flags)) {
    return NS_STRETCH_DIRECTION_HORIZONTAL;
  } else {
    return NS_STRETCH_DIRECTION_UNSUPPORTED;
  }
}

 eMATHVARIANT
nsMathMLOperators::LookupInvariantChar(const nsAString& aChar)
{
  if (!gInitialized) {
    InitGlobals();
  }
  if (gInvariantCharArray) {
    for (PRInt32 i = gInvariantCharArray->Length()-1; i >= 0; --i) {
      const nsString& list = gInvariantCharArray->ElementAt(i);
      nsString::const_iterator start, end;
      list.BeginReading(start);
      list.EndReading(end);
      
      if (FindInReadable(aChar, start, end) &&
          start.size_backward() % 3 == 1) {
        return eMATHVARIANT(i);
      }
    }
  }
  return eMATHVARIANT_NONE;
}

 const nsDependentSubstring
nsMathMLOperators::TransformVariantChar(const PRUnichar& aChar,
                                        eMATHVARIANT aVariant)
{
  if (!gInitialized) {
    InitGlobals();
  }
  if (gInvariantCharArray) {
    nsString list = gInvariantCharArray->ElementAt(aVariant);
    PRInt32 index = list.FindChar(aChar);
    
    if (index != kNotFound && index % 3 == 0 && list.Length() - index >= 2 ) {
      
      
      ++index;
      PRUint32 len = NS_IS_HIGH_SURROGATE(list.CharAt(index)) ? 2 : 1;
      return nsDependentSubstring(list, index, len);
    }
  }
  return nsDependentSubstring(&aChar, &aChar + 1);  
}
