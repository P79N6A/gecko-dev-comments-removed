




































#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsHashtable.h"
#include "nsVoidArray.h"

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










static OperatorData*   gOperatorFound[4];

static PRInt32         gTableRefCount = 0;
static PRInt32         gOperatorCount = 0;
static OperatorData*   gOperatorArray = nsnull;
static nsHashtable*    gOperatorTable = nsnull;
static nsVoidArray*    gStretchyOperatorArray = nsnull;
static nsStringArray*  gInvariantCharArray = nsnull;
static PRBool          gInitialized   = PR_FALSE;

static const PRUnichar kNullCh  = PRUnichar('\0');
static const PRUnichar kDashCh  = PRUnichar('#');
static const PRUnichar kEqualCh = PRUnichar('=');
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

void
SetProperty(OperatorData* aOperatorData,
            nsString      aName,
            nsString      aValue)
{
  if (!aName.Length() || !aValue.Length())
    return;

  
  
  
  

  if (aValue.EqualsLiteral("true")) {
    
    if (aName.EqualsLiteral("fence"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_FENCE;
    else if (aName.EqualsLiteral("accent"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_ACCENT;
    else if (aName.EqualsLiteral("largeop"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_LARGEOP;
    else if (aName.EqualsLiteral("separator"))
      aOperatorData->mFlags |=  NS_MATHML_OPERATOR_SEPARATOR;
    else if (aName.EqualsLiteral("movablelimits"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_MOVABLELIMITS;
  }
  else if (aValue.EqualsLiteral("false")) {
    
    if (aName.EqualsLiteral("symmetric"))
      aOperatorData->mFlags &= ~NS_MATHML_OPERATOR_SYMMETRIC;
  }
  else if (aName.EqualsLiteral("stretchy") &&
          (1 == aOperatorData->mStr.Length())) {
    if (aValue.EqualsLiteral("vertical"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_STRETCHY_VERT;
    else if (aValue.EqualsLiteral("horizontal"))
      aOperatorData->mFlags |= NS_MATHML_OPERATOR_STRETCHY_HORIZ;
    else return; 
    if (kNotFound == nsMathMLOperators::FindStretchyOperator(aOperatorData->mStr[0])) {
      gStretchyOperatorArray->AppendElement(aOperatorData);
    }
  }
  else {
    PRInt32 i = 0;
    float space = 0.0f;
    PRBool isLeftSpace;
    if (aName.EqualsLiteral("lspace"))
      isLeftSpace = PR_TRUE;
    else if (aName.EqualsLiteral("rspace"))
      isLeftSpace = PR_FALSE;
    else return;  

    
    if (nsCRT::IsAsciiDigit(aValue[0])) {
      PRInt32 error = 0;
      space = aValue.ToFloat(&error);
      if (error) return;
    }
    
    else if (aValue.EqualsLiteral("veryverythinmathspace"))  i = 1;
    else if (aValue.EqualsLiteral("verythinmathspace"))      i = 2;
    else if (aValue.EqualsLiteral("thinmathspace"))          i = 3;
    else if (aValue.EqualsLiteral("mediummathspace"))        i = 4;
    else if (aValue.EqualsLiteral("thickmathspace"))         i = 5;
    else if (aValue.EqualsLiteral("verythickmathspace"))     i = 6;
    else if (aValue.EqualsLiteral("veryverythickmathspace")) i = 7;

    if (0 != i) 
      space = float(i)/float(18);

    if (isLeftSpace)
      aOperatorData->mLeftSpace = space;
    else
      aOperatorData->mRightSpace = space;
  }
}

PRBool
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
        return PR_FALSE;
      if (i < len)
        c = aOperator[i];
      i++;
      if (('u' != c) && ('U' != c))
        return PR_FALSE;
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
      else return PR_FALSE;
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
  if (0 != state) return PR_FALSE;

  
  
  
  if (!aForm) return PR_TRUE;

  
  aOperatorData->mFlags |= aForm | NS_MATHML_OPERATOR_SYMMETRIC;
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
    
    while ((kNullCh!=*end) && (kDashCh!=*end) && (kColonCh!=*end) && (kEqualCh!=*end)) {
      ++end;
    }
    if ((kColonCh!=*end) && (kEqualCh!=*end)) {
#ifdef NS_DEBUG
      printf("Bad MathML operator: %s\n", str.get());
#endif
      return PR_TRUE;
    }
    *end = kNullCh; 
    
    if (start < end) {
      name.Assign(start);
    }
    start = ++end;
    
    while ((kNullCh!=*end) && (kDashCh!=*start) && !nsCRT::IsAsciiSpace(*end)) {
      ++end;
    }
    *end = kNullCh; 
    
    if (start < end) {
      value.Assign(start);
    }
    SetProperty(aOperatorData, name, value);
    start = ++end;
  }
  return PR_TRUE;
}

nsresult
InitOperators(void)
{
  
  nsresult rv;
  nsCOMPtr<nsIPersistentProperties> mathfontProp;
  rv = NS_LoadPersistentPropertiesFromURISpec(getter_AddRefs(mathfontProp),
       NS_LITERAL_CSTRING("resource://gre/res/fonts/mathfont.properties"));
  if (NS_FAILED(rv)) return rv;

  
  for (PRInt32 i = 0; i < nsMathMLOperators::eMATHVARIANT_COUNT; ++i) {
    nsCAutoString key(NS_LITERAL_CSTRING("mathvariant."));
    key.Append(kMathVariant_name[i]);
    nsAutoString value;
    mathfontProp->GetStringProperty(key, value);
    gInvariantCharArray->AppendString(value); 
  }

  
  
  
  for (PRInt32 pass = 1; pass <= 2; pass++) {
    OperatorData dummyData;
    OperatorData* operatorData = &dummyData;
    nsCOMPtr<nsISimpleEnumerator> iterator;
    if (NS_SUCCEEDED(mathfontProp->Enumerate(getter_AddRefs(iterator)))) {
      PRBool more;
      PRInt32 index = 0;
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

nsresult
InitGlobals()
{
  gInitialized = PR_TRUE;
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  gInvariantCharArray = new nsStringArray();
  gStretchyOperatorArray = new nsVoidArray();
  if (gInvariantCharArray && gStretchyOperatorArray) {
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
  if (gStretchyOperatorArray) {
    delete gStretchyOperatorArray;
    gStretchyOperatorArray = nsnull;
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

PRBool
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
    NS_ASSERTION(aForm>=0 && aForm<4, "*** invalid call ***");

    OperatorData* found;
    PRInt32 form = NS_MATHML_OPERATOR_GET_FORM(aForm);
    gOperatorFound[NS_MATHML_OPERATOR_FORM_INFIX] = nsnull;
    gOperatorFound[NS_MATHML_OPERATOR_FORM_POSTFIX] = nsnull;
    gOperatorFound[NS_MATHML_OPERATOR_FORM_PREFIX] = nsnull;

    nsAutoString key(aOperator);
    key.AppendInt(form, 10);
    nsStringKey hkey(key);
    gOperatorFound[form] = found = (OperatorData*)gOperatorTable->Get(&hkey);

    
    
    if (!found) {
      if (form != NS_MATHML_OPERATOR_FORM_INFIX) {
        form = NS_MATHML_OPERATOR_FORM_INFIX;
        key.Assign(aOperator);
        key.AppendInt(form, 10);
        nsStringKey hashkey(key);
        gOperatorFound[form] = found = (OperatorData*)gOperatorTable->Get(&hashkey);
      }
      if (!found) {
        if (form != NS_MATHML_OPERATOR_FORM_POSTFIX) {
          form = NS_MATHML_OPERATOR_FORM_POSTFIX;
          key.Assign(aOperator);
          key.AppendInt(form, 10);
          nsStringKey hashkey(key);
          gOperatorFound[form] = found = (OperatorData*)gOperatorTable->Get(&hashkey);
        }
        if (!found) {
          if (form != NS_MATHML_OPERATOR_FORM_PREFIX) {
            form = NS_MATHML_OPERATOR_FORM_PREFIX;
            key.Assign(aOperator);
            key.AppendInt(form, 10);
            nsStringKey hashkey(key);
            gOperatorFound[form] = found = (OperatorData*)gOperatorTable->Get(&hashkey);
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
      return PR_TRUE;
    }
  }
  return PR_FALSE;
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
    
    float dummy;
    nsOperatorFlags flags = 0;
    LookupOperator(aOperator, 0, &flags, &dummy, &dummy);
    
    OperatorData* found;
    found = gOperatorFound[NS_MATHML_OPERATOR_FORM_INFIX];
    if (found) {
      aFlags[NS_MATHML_OPERATOR_FORM_INFIX] = found->mFlags;
      aLeftSpace[NS_MATHML_OPERATOR_FORM_INFIX] = found->mLeftSpace;
      aRightSpace[NS_MATHML_OPERATOR_FORM_INFIX] = found->mRightSpace;
    }
    found = gOperatorFound[NS_MATHML_OPERATOR_FORM_POSTFIX];
    if (found) {
      aFlags[NS_MATHML_OPERATOR_FORM_POSTFIX] = found->mFlags;
      aLeftSpace[NS_MATHML_OPERATOR_FORM_POSTFIX] = found->mLeftSpace;
      aRightSpace[NS_MATHML_OPERATOR_FORM_POSTFIX] = found->mRightSpace;
    }
    found = gOperatorFound[NS_MATHML_OPERATOR_FORM_PREFIX];
    if (found) {
      aFlags[NS_MATHML_OPERATOR_FORM_PREFIX] = found->mFlags;
      aLeftSpace[NS_MATHML_OPERATOR_FORM_PREFIX] = found->mLeftSpace;
      aRightSpace[NS_MATHML_OPERATOR_FORM_PREFIX] = found->mRightSpace;
    }
  }
}

PRBool
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

PRInt32
nsMathMLOperators::CountStretchyOperator()
{
  if (!gInitialized) {
    InitGlobals();
  }
  return (gStretchyOperatorArray) ? gStretchyOperatorArray->Count() : 0;
}

PRInt32
nsMathMLOperators::FindStretchyOperator(PRUnichar aOperator)
{
  if (!gInitialized) {
    InitGlobals();
  }
  if (gStretchyOperatorArray) {
    for (PRInt32 k = 0; k < gStretchyOperatorArray->Count(); k++) {
      OperatorData* data = (OperatorData*)gStretchyOperatorArray->ElementAt(k);
      if (data && (aOperator == data->mStr[0])) {
        return k;
      }
    }
  }
  return kNotFound;
}

nsStretchDirection
nsMathMLOperators::GetStretchyDirectionAt(PRInt32 aIndex)
{
  NS_ASSERTION(gStretchyOperatorArray, "invalid call");
  if (gStretchyOperatorArray) {
    NS_ASSERTION(aIndex < gStretchyOperatorArray->Count(), "invalid call");
    OperatorData* data = (OperatorData*)gStretchyOperatorArray->ElementAt(aIndex);
    if (data) {
      if (NS_MATHML_OPERATOR_IS_STRETCHY_VERT(data->mFlags))
        return NS_STRETCH_DIRECTION_VERTICAL;
      else if (NS_MATHML_OPERATOR_IS_STRETCHY_HORIZ(data->mFlags))
        return NS_STRETCH_DIRECTION_HORIZONTAL;
      NS_ASSERTION(PR_FALSE, "*** bad setup ***");
    }
  }
  return NS_STRETCH_DIRECTION_UNSUPPORTED;
}

void
nsMathMLOperators::DisableStretchyOperatorAt(PRInt32 aIndex)
{
  NS_ASSERTION(gStretchyOperatorArray, "invalid call");
  if (gStretchyOperatorArray) {
    NS_ASSERTION(aIndex < gStretchyOperatorArray->Count(), "invalid call");
    gStretchyOperatorArray->ReplaceElementAt(nsnull, aIndex);
  }
}

PRBool
nsMathMLOperators::LookupInvariantChar(PRUnichar     aChar,
                                       eMATHVARIANT* aType)
{
  if (!gInitialized) {
    InitGlobals();
  }
  if (aType) *aType = eMATHVARIANT_NONE;
  if (gInvariantCharArray) {
    for (PRInt32 i = gInvariantCharArray->Count()-1; i >= 0; --i) {
      nsString* list = gInvariantCharArray->StringAt(i);
      if (kNotFound != list->FindChar(aChar)) {
        if (aType) *aType = eMATHVARIANT(i);
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}
