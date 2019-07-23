













































#include "nscore.h"
#include "prlog.h"
#include "nsCSSDeclaration.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsCSSProps.h"
#include "nsFont.h"
#include "nsReadableUtils.h"
#include "nsStyleUtil.h"

#include "nsStyleConsts.h"

#include "nsCOMPtr.h"

nsCSSDeclaration::nsCSSDeclaration() 
  : mData(nsnull),
    mImportantData(nsnull)
{
  
  
  PR_STATIC_ASSERT(eCSSProperty_COUNT_no_shorthands - 1 <= PR_UINT8_MAX);

  MOZ_COUNT_CTOR(nsCSSDeclaration);
}

nsCSSDeclaration::nsCSSDeclaration(const nsCSSDeclaration& aCopy)
  : mOrder(aCopy.mOrder),
    mData(aCopy.mData ? aCopy.mData->Clone() : nsnull),
    mImportantData(aCopy.mImportantData ? aCopy.mImportantData->Clone()
                                         : nsnull)
{
  MOZ_COUNT_CTOR(nsCSSDeclaration);
}

nsCSSDeclaration::~nsCSSDeclaration(void)
{
  if (mData) {
    mData->Destroy();
  }
  if (mImportantData) {
    mImportantData->Destroy();
  }

  MOZ_COUNT_DTOR(nsCSSDeclaration);
}

nsresult
nsCSSDeclaration::ValueAppended(nsCSSProperty aProperty)
{
  NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty),
                    "shorthands forbidden");
  
  mOrder.RemoveElement(aProperty);
  mOrder.AppendElement(aProperty);
  return NS_OK;
}

nsresult
nsCSSDeclaration::RemoveProperty(nsCSSProperty aProperty)
{
  nsCSSExpandedDataBlock data;
  data.Expand(&mData, &mImportantData);
  NS_ASSERTION(!mData && !mImportantData, "Expand didn't null things out");

  if (nsCSSProps::IsShorthand(aProperty)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aProperty) {
      data.ClearProperty(*p);
      mOrder.RemoveElement(*p);
    }
  } else {
    data.ClearProperty(aProperty);
    mOrder.RemoveElement(aProperty);
  }

  data.Compress(&mData, &mImportantData);
  return NS_OK;
}

nsresult
nsCSSDeclaration::AppendComment(const nsAString& aComment)
{
  return  NS_OK;
}

PRBool nsCSSDeclaration::AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const
{
  nsCSSCompressedDataBlock *data = GetValueIsImportant(aProperty)
                                      ? mImportantData : mData;
  const void *storage = data->StorageFor(aProperty);
  if (storage) {
    switch (nsCSSProps::kTypeTable[aProperty]) {
      case eCSSType_Value: {
        const nsCSSValue *val = static_cast<const nsCSSValue*>(storage);
        AppendCSSValueToString(aProperty, *val, aResult);
      } break;
      case eCSSType_Rect: {
        const nsCSSRect *rect = static_cast<const nsCSSRect*>(storage);
        const nsCSSUnit topUnit = rect->mTop.GetUnit();
        if (topUnit == eCSSUnit_Inherit ||
            topUnit == eCSSUnit_Initial ||
            topUnit == eCSSUnit_RectIsAuto) {
          NS_ASSERTION(rect->mRight.GetUnit() == topUnit &&
                       rect->mBottom.GetUnit() == topUnit &&
                       rect->mLeft.GetUnit() == topUnit,
                       "parser should make all sides have the same unit");
          if (topUnit == eCSSUnit_RectIsAuto)
            aResult.AppendLiteral("auto");
          else
            AppendCSSValueToString(aProperty, rect->mTop, aResult);
        } else {
          aResult.AppendLiteral("rect(");
          AppendCSSValueToString(aProperty, rect->mTop, aResult);
          NS_NAMED_LITERAL_STRING(comma, ", ");
          aResult.Append(comma);
          AppendCSSValueToString(aProperty, rect->mRight, aResult);
          aResult.Append(comma);
          AppendCSSValueToString(aProperty, rect->mBottom, aResult);
          aResult.Append(comma);
          AppendCSSValueToString(aProperty, rect->mLeft, aResult);
          aResult.Append(PRUnichar(')'));
        }
      } break;
      case eCSSType_ValuePair: {
        const nsCSSValuePair *pair = static_cast<const nsCSSValuePair*>(storage);
        AppendCSSValueToString(aProperty, pair->mXValue, aResult);
        if (pair->mYValue != pair->mXValue ||
            ((aProperty == eCSSProperty_background_position ||
              aProperty == eCSSProperty__moz_transform_origin) &&
             pair->mXValue.GetUnit() != eCSSUnit_Inherit &&
             pair->mXValue.GetUnit() != eCSSUnit_Initial) ||
            (aProperty == eCSSProperty__moz_background_size &&
             pair->mXValue.GetUnit() != eCSSUnit_Inherit &&
             pair->mXValue.GetUnit() != eCSSUnit_Initial &&
             pair->mXValue.GetUnit() != eCSSUnit_Enumerated)) {
          
          
          
          
          
          aResult.Append(PRUnichar(' '));
          AppendCSSValueToString(aProperty, pair->mYValue, aResult);
        }
      } break;
      case eCSSType_ValueList: {
        const nsCSSValueList* val =
            *static_cast<nsCSSValueList*const*>(storage);
        do {
          AppendCSSValueToString(aProperty, val->mValue, aResult);
          val = val->mNext;
          if (val) {
            if (nsCSSProps::PropHasFlags(aProperty,
                                         CSS_PROPERTY_VALUE_LIST_USES_COMMAS))
              aResult.Append(PRUnichar(','));
            aResult.Append(PRUnichar(' '));
          }
        } while (val);
      } break;
      case eCSSType_ValuePairList: {
        const nsCSSValuePairList* item =
            *static_cast<nsCSSValuePairList*const*>(storage);
        do {
          NS_ASSERTION(item->mXValue.GetUnit() != eCSSUnit_Null,
                       "unexpected null unit");
          AppendCSSValueToString(aProperty, item->mXValue, aResult);
          if (item->mXValue.GetUnit() != eCSSUnit_Inherit &&
              item->mXValue.GetUnit() != eCSSUnit_Initial &&
              item->mYValue.GetUnit() != eCSSUnit_Null) {
            aResult.Append(PRUnichar(' '));
            AppendCSSValueToString(aProperty, item->mYValue, aResult);
          }
          item = item->mNext;
          if (item) {
            if (nsCSSProps::PropHasFlags(aProperty,
                                         CSS_PROPERTY_VALUE_LIST_USES_COMMAS))
              aResult.Append(PRUnichar(','));
            aResult.Append(PRUnichar(' '));
          }
        } while (item);
      } break;
    }
  }
  return storage != nsnull;
}

 PRBool
nsCSSDeclaration::AppendCSSValueToString(nsCSSProperty aProperty,
                                         const nsCSSValue& aValue,
                                         nsAString& aResult)
{
  nsCSSUnit unit = aValue.GetUnit();

  if (eCSSUnit_Null == unit) {
    return PR_FALSE;
  }

  if (eCSSUnit_String <= unit && unit <= eCSSUnit_Attr) {
    if (unit == eCSSUnit_Attr) {
      aResult.AppendLiteral("attr(");
    }
    nsAutoString  buffer;
    aValue.GetStringValue(buffer);
    if (unit == eCSSUnit_String) {
      nsStyleUtil::AppendEscapedCSSString(buffer, aResult);
    } else {
      aResult.Append(buffer);
    }
  }
  else if (eCSSUnit_Array <= unit && unit <= eCSSUnit_Cubic_Bezier) {
    switch (unit) {
      case eCSSUnit_Counter:  aResult.AppendLiteral("counter(");  break;
      case eCSSUnit_Counters: aResult.AppendLiteral("counters("); break;
      case eCSSUnit_Cubic_Bezier: aResult.AppendLiteral("cubic-bezier("); break;
      default: break;
    }

    nsCSSValue::Array *array = aValue.GetArrayValue();
    PRBool mark = PR_FALSE;
    for (PRUint16 i = 0, i_end = array->Count(); i < i_end; ++i) {
      if (aProperty == eCSSProperty_border_image && i >= 5) {
        if (array->Item(i).GetUnit() == eCSSUnit_Null) {
          continue;
        }
        if (i == 5) {
          aResult.AppendLiteral(" /");
        }
      }
      if (mark && array->Item(i).GetUnit() != eCSSUnit_Null) {
        if (unit == eCSSUnit_Array &&
            eCSSProperty_transition_timing_function != aProperty)
          aResult.AppendLiteral(" ");
        else
          aResult.AppendLiteral(", ");
      }
      nsCSSProperty prop =
        ((eCSSUnit_Counter <= unit && unit <= eCSSUnit_Counters) &&
         i == array->Count() - 1)
        ? eCSSProperty_list_style_type : aProperty;
      if (AppendCSSValueToString(prop, array->Item(i), aResult)) {
        mark = PR_TRUE;
      }
    }
    if (eCSSUnit_Array == unit && aProperty == eCSSProperty_transition_timing_function) {
      aResult.AppendLiteral(")");
    }
  }
  


  else if (eCSSUnit_Function == unit) {
    const nsCSSValue::Array* array = aValue.GetArrayValue();
    NS_ASSERTION(array->Count() >= 1, "Functions must have at least one element for the name.");

    
    const nsCSSValue& functionName = array->Item(0);
    if (functionName.GetUnit() == eCSSUnit_Enumerated) {
      
      const nsCSSKeyword functionId =
        static_cast<nsCSSKeyword>(functionName.GetIntValue());
      AppendASCIItoUTF16(nsCSSKeywords::GetStringValue(functionId), aResult);
    } else {
      AppendCSSValueToString(aProperty, functionName, aResult);
    }
    aResult.AppendLiteral("(");

    
    for (PRUint16 index = 1; index < array->Count(); ++index) {
      AppendCSSValueToString(aProperty, array->Item(index), aResult);

      
      if (index + 1 != array->Count())
        aResult.AppendLiteral(", ");
    }

    
    aResult.AppendLiteral(")");
  }
  else if (eCSSUnit_Integer == unit) {
    nsAutoString tmpStr;
    tmpStr.AppendInt(aValue.GetIntValue(), 10);
    aResult.Append(tmpStr);
  }
  else if (eCSSUnit_Enumerated == unit) {
    if (eCSSProperty_text_decoration == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      NS_ABORT_IF_FALSE(NS_STYLE_TEXT_DECORATION_NONE != intValue,
                        "none should be parsed as eCSSUnit_None");
      PRInt32 mask;
      for (mask = NS_STYLE_TEXT_DECORATION_UNDERLINE;
           mask <= NS_STYLE_TEXT_DECORATION_PREF_ANCHORS; 
           mask <<= 1) {
        if ((mask & intValue) == mask) {
          AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, mask), aResult);
          intValue &= ~mask;
          if (0 != intValue) { 
            aResult.Append(PRUnichar(' '));
          }
        }
      }
    }
    else if (eCSSProperty_azimuth == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, (intValue & ~NS_STYLE_AZIMUTH_BEHIND)), aResult);
      if ((NS_STYLE_AZIMUTH_BEHIND & intValue) != 0) {
        aResult.Append(PRUnichar(' '));
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_AZIMUTH_BEHIND), aResult);
      }
    }
    else if (eCSSProperty_marks == aProperty) {
      PRInt32 intValue = aValue.GetIntValue();
      if ((NS_STYLE_PAGE_MARKS_CROP & intValue) != 0) {
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_PAGE_MARKS_CROP), aResult);
      }
      if ((NS_STYLE_PAGE_MARKS_REGISTER & intValue) != 0) {
        if ((NS_STYLE_PAGE_MARKS_CROP & intValue) != 0) {
          aResult.Append(PRUnichar(' '));
        }
        AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, NS_STYLE_PAGE_MARKS_REGISTER), aResult);
      }
    }
    else if (eCSSProperty_transition_property == aProperty) {
      AppendASCIItoUTF16(nsCSSProps::GetStringValue((nsCSSProperty) aValue.GetIntValue()), aResult);
    }
    else {
      const nsAFlatCString& name = nsCSSProps::LookupPropertyValue(aProperty, aValue.GetIntValue());
      AppendASCIItoUTF16(name, aResult);
    }
  }
  else if (eCSSUnit_EnumColor == unit) {
    
    
    nsCAutoString str;
    if (nsCSSProps::GetColorName(aValue.GetIntValue(), str)){
      AppendASCIItoUTF16(str, aResult);
    } else {
      NS_NOTREACHED("bad color value");
    }
  }
  else if (eCSSUnit_Color == unit) {
    nscolor color = aValue.GetColorValue();
    if (color == NS_RGBA(0, 0, 0, 0)) {
      
      
      aResult.AppendLiteral("transparent");
    } else {
      nsAutoString tmpStr;
      PRUint8 a = NS_GET_A(color);
      if (a < 255) {
        tmpStr.AppendLiteral("rgba(");
      } else {
        tmpStr.AppendLiteral("rgb(");
      }

      NS_NAMED_LITERAL_STRING(comma, ", ");

      tmpStr.AppendInt(NS_GET_R(color), 10);
      tmpStr.Append(comma);
      tmpStr.AppendInt(NS_GET_G(color), 10);
      tmpStr.Append(comma);
      tmpStr.AppendInt(NS_GET_B(color), 10);
      if (a < 255) {
        tmpStr.Append(comma);
        tmpStr.AppendFloat(nsStyleUtil::ColorComponentToFloat(a));
      }
      tmpStr.Append(PRUnichar(')'));

      aResult.Append(tmpStr);
    }
  }
  else if (eCSSUnit_URL == unit || eCSSUnit_Image == unit) {
    aResult.Append(NS_LITERAL_STRING("url("));
    nsStyleUtil::AppendEscapedCSSString(
      nsDependentString(aValue.GetOriginalURLValue()), aResult);
    aResult.Append(NS_LITERAL_STRING(")"));
  }
  else if (eCSSUnit_Percent == unit) {
    nsAutoString tmpStr;
    tmpStr.AppendFloat(aValue.GetPercentValue() * 100.0f);
    aResult.Append(tmpStr);
  }
  else if (eCSSUnit_Percent < unit) {  
    nsAutoString tmpStr;
    tmpStr.AppendFloat(aValue.GetFloatValue());
    aResult.Append(tmpStr);
  }
  else if (eCSSUnit_Gradient == unit) {
    nsCSSValueGradient* gradient = aValue.GetGradientValue();

    if (gradient->mIsRadial)
      aResult.AppendLiteral("-moz-radial-gradient(");
    else
      aResult.AppendLiteral("-moz-linear-gradient(");

    AppendCSSValueToString(eCSSProperty_background_position,
                           gradient->mStartX, aResult);
    aResult.AppendLiteral(" ");

    AppendCSSValueToString(eCSSProperty_background_position,
                           gradient->mStartY, aResult);
    aResult.AppendLiteral(", ");

    if (gradient->mIsRadial) {
      AppendCSSValueToString(aProperty, gradient->mStartRadius, aResult);
      aResult.AppendLiteral(", ");
    }

    AppendCSSValueToString(eCSSProperty_background_position,
                           gradient->mEndX, aResult);
    aResult.AppendLiteral(" ");

    AppendCSSValueToString(eCSSProperty_background_position,
                           gradient->mEndY, aResult);

    if (gradient->mIsRadial) {
      aResult.AppendLiteral(", ");
      AppendCSSValueToString(aProperty, gradient->mEndRadius, aResult);
    }

    for (PRUint32 i = 0; i < gradient->mStops.Length(); i++) {
      aResult.AppendLiteral(", color-stop(");
      AppendCSSValueToString(aProperty, gradient->mStops[i].mLocation, aResult);
      aResult.AppendLiteral(", ");
      AppendCSSValueToString(aProperty, gradient->mStops[i].mColor, aResult);
      aResult.AppendLiteral(")");
    }

    aResult.AppendLiteral(")");
  }

  switch (unit) {
    case eCSSUnit_Null:         break;
    case eCSSUnit_Auto:         aResult.AppendLiteral("auto");     break;
    case eCSSUnit_Inherit:      aResult.AppendLiteral("inherit");  break;
    case eCSSUnit_Initial:      aResult.AppendLiteral("-moz-initial"); break;
    case eCSSUnit_None:         aResult.AppendLiteral("none");     break;
    case eCSSUnit_Normal:       aResult.AppendLiteral("normal");   break;
    case eCSSUnit_System_Font:  aResult.AppendLiteral("-moz-use-system-font"); break;
    case eCSSUnit_All:          aResult.AppendLiteral("all"); break;
    case eCSSUnit_Dummy:
    case eCSSUnit_DummyInherit:
    case eCSSUnit_RectIsAuto:
      NS_NOTREACHED("should never serialize");
      break;

    case eCSSUnit_String:       break;
    case eCSSUnit_Ident:        break;
    case eCSSUnit_Families:     break;
    case eCSSUnit_URL:          break;
    case eCSSUnit_Image:        break;
    case eCSSUnit_Array:        break;
    case eCSSUnit_Attr:
    case eCSSUnit_Cubic_Bezier:
    case eCSSUnit_Counter:
    case eCSSUnit_Counters:     aResult.Append(PRUnichar(')'));    break;
    case eCSSUnit_Local_Font:   break;
    case eCSSUnit_Font_Format:  break;
    case eCSSUnit_Function:     break;
    case eCSSUnit_Integer:      break;
    case eCSSUnit_Enumerated:   break;
    case eCSSUnit_EnumColor:    break;
    case eCSSUnit_Color:        break;
    case eCSSUnit_Percent:      aResult.Append(PRUnichar('%'));    break;
    case eCSSUnit_Number:       break;
    case eCSSUnit_Gradient:     break;

    case eCSSUnit_Inch:         aResult.AppendLiteral("in");   break;
    case eCSSUnit_Millimeter:   aResult.AppendLiteral("mm");   break;
    case eCSSUnit_Centimeter:   aResult.AppendLiteral("cm");   break;
    case eCSSUnit_Point:        aResult.AppendLiteral("pt");   break;
    case eCSSUnit_Pica:         aResult.AppendLiteral("pc");   break;

    case eCSSUnit_EM:           aResult.AppendLiteral("em");   break;
    case eCSSUnit_XHeight:      aResult.AppendLiteral("ex");   break;
    case eCSSUnit_Char:         aResult.AppendLiteral("ch");   break;
    case eCSSUnit_RootEM:       aResult.AppendLiteral("rem");  break;

    case eCSSUnit_Pixel:        aResult.AppendLiteral("px");   break;

    case eCSSUnit_Degree:       aResult.AppendLiteral("deg");  break;
    case eCSSUnit_Grad:         aResult.AppendLiteral("grad"); break;
    case eCSSUnit_Radian:       aResult.AppendLiteral("rad");  break;

    case eCSSUnit_Hertz:        aResult.AppendLiteral("Hz");   break;
    case eCSSUnit_Kilohertz:    aResult.AppendLiteral("kHz");  break;

    case eCSSUnit_Seconds:      aResult.Append(PRUnichar('s'));    break;
    case eCSSUnit_Milliseconds: aResult.AppendLiteral("ms");   break;
  }

  return PR_TRUE;
}

nsresult
nsCSSDeclaration::GetValue(nsCSSProperty aProperty,
                           nsAString& aValue) const
{
  aValue.Truncate(0);

  
  if (!nsCSSProps::IsShorthand(aProperty)) {
    AppendValueToString(aProperty, aValue);
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRUint32 totalCount = 0, importantCount = 0,
           initialCount = 0, inheritCount = 0;
  CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aProperty) {
    if (*p == eCSSProperty__x_system_font ||
         nsCSSProps::PropHasFlags(*p, CSS_PROPERTY_DIRECTIONAL_SOURCE)) {
      
      continue;
    }
    ++totalCount;
    const void *storage = mData->StorageFor(*p);
    NS_ASSERTION(!storage || !mImportantData || !mImportantData->StorageFor(*p),
                 "can't be in both blocks");
    if (!storage && mImportantData) {
      ++importantCount;
      storage = mImportantData->StorageFor(*p);
    }
    if (!storage) {
      
      return NS_OK;
    }
    nsCSSUnit unit;
    switch (nsCSSProps::kTypeTable[*p]) {
      case eCSSType_Value: {
        const nsCSSValue *val = static_cast<const nsCSSValue*>(storage);
        unit = val->GetUnit();
      } break;
      case eCSSType_Rect: {
        const nsCSSRect *rect = static_cast<const nsCSSRect*>(storage);
        unit = rect->mTop.GetUnit();
      } break;
      case eCSSType_ValuePair: {
        const nsCSSValuePair *pair = static_cast<const nsCSSValuePair*>(storage);
        unit = pair->mXValue.GetUnit();
      } break;
      case eCSSType_ValueList: {
        const nsCSSValueList* item =
            *static_cast<nsCSSValueList*const*>(storage);
        NS_ABORT_IF_FALSE(item, "null not allowed in compressed block");
        unit = item->mValue.GetUnit();
      } break;
      case eCSSType_ValuePairList: {
        const nsCSSValuePairList* item =
            *static_cast<nsCSSValuePairList*const*>(storage);
        NS_ABORT_IF_FALSE(item, "null not allowed in compressed block");
        unit = item->mXValue.GetUnit();
      } break;
    }
    if (unit == eCSSUnit_Inherit) {
      ++inheritCount;
    } else if (unit == eCSSUnit_Initial) {
      ++initialCount;
    }
  }
  if (importantCount != 0 && importantCount != totalCount) {
    
    return NS_OK;
  }
  if (initialCount == totalCount) {
    
    AppendCSSValueToString(eCSSProperty_UNKNOWN, nsCSSValue(eCSSUnit_Initial),
                           aValue);
    return NS_OK;
  }
  if (inheritCount == totalCount) {
    
    AppendCSSValueToString(eCSSProperty_UNKNOWN, nsCSSValue(eCSSUnit_Inherit),
                           aValue);
    return NS_OK;
  }
  if (initialCount != 0 || inheritCount != 0) {
    
    return NS_OK;
  }

  nsCSSCompressedDataBlock *data = importantCount ? mImportantData : mData;
  switch (aProperty) {
    case eCSSProperty_margin: 
    case eCSSProperty_padding: 
    case eCSSProperty_border_color: 
    case eCSSProperty_border_style: 
    case eCSSProperty_border_width: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(nsCSSProps::kTypeTable[subprops[0]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[1]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[2]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[3]] == eCSSType_Value,
                   "type mismatch");
      NS_ASSERTION(nsCSSProps::GetStringValue(subprops[0]).Find("-top") !=
                     kNotFound, "first subprop must be top");
      NS_ASSERTION(nsCSSProps::GetStringValue(subprops[1]).Find("-right") !=
                     kNotFound, "second subprop must be right");
      NS_ASSERTION(nsCSSProps::GetStringValue(subprops[2]).Find("-bottom") !=
                     kNotFound, "third subprop must be bottom");
      NS_ASSERTION(nsCSSProps::GetStringValue(subprops[3]).Find("-left") !=
                     kNotFound, "fourth subprop must be left");
      const nsCSSValue &topValue = *data->ValueStorageFor(subprops[0]);
      const nsCSSValue &rightValue = *data->ValueStorageFor(subprops[1]);
      const nsCSSValue &bottomValue = *data->ValueStorageFor(subprops[2]);
      const nsCSSValue &leftValue = *data->ValueStorageFor(subprops[3]);
      PRBool haveValue;
      haveValue = AppendCSSValueToString(subprops[0], topValue, aValue);
      NS_ASSERTION(haveValue, "should have bailed before");
      if (topValue != rightValue || topValue != leftValue ||
          topValue != bottomValue) {
        aValue.Append(PRUnichar(' '));
        haveValue = AppendCSSValueToString(subprops[1], rightValue, aValue);
        NS_ASSERTION(haveValue, "should have bailed before");
        if (topValue != bottomValue || rightValue != leftValue) {
          aValue.Append(PRUnichar(' '));
          haveValue = AppendCSSValueToString(subprops[2], bottomValue, aValue);
          NS_ASSERTION(haveValue, "should have bailed before");
          if (rightValue != leftValue) {
            aValue.Append(PRUnichar(' '));
            haveValue = AppendCSSValueToString(subprops[3], leftValue, aValue);
            NS_ASSERTION(haveValue, "should have bailed before");
          }
        }
      }
      break;
    }
    case eCSSProperty__moz_border_radius: 
    case eCSSProperty__moz_outline_radius: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(nsCSSProps::kTypeTable[subprops[0]] == eCSSType_ValuePair &&
                   nsCSSProps::kTypeTable[subprops[1]] == eCSSType_ValuePair &&
                   nsCSSProps::kTypeTable[subprops[2]] == eCSSType_ValuePair &&
                   nsCSSProps::kTypeTable[subprops[3]] == eCSSType_ValuePair,
                   "type mismatch");
      const nsCSSValuePair* vals[4] = {
        data->ValuePairStorageFor(subprops[0]),
        data->ValuePairStorageFor(subprops[1]),
        data->ValuePairStorageFor(subprops[2]),
        data->ValuePairStorageFor(subprops[3])
      };

      AppendCSSValueToString(aProperty, vals[0]->mXValue, aValue);
      aValue.Append(PRUnichar(' '));
      AppendCSSValueToString(aProperty, vals[1]->mXValue, aValue);
      aValue.Append(PRUnichar(' '));
      AppendCSSValueToString(aProperty, vals[2]->mXValue, aValue);
      aValue.Append(PRUnichar(' '));
      AppendCSSValueToString(aProperty, vals[3]->mXValue, aValue);
        
      
      
      if (vals[0]->mXValue != vals[0]->mYValue ||
          vals[1]->mXValue != vals[1]->mYValue ||
          vals[2]->mXValue != vals[2]->mYValue ||
          vals[3]->mXValue != vals[3]->mYValue) {
        aValue.AppendLiteral(" / ");
        AppendCSSValueToString(aProperty, vals[0]->mYValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(aProperty, vals[1]->mYValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(aProperty, vals[2]->mYValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(aProperty, vals[3]->mYValue, aValue);
      }
      break;
    }
    case eCSSProperty_border: {
      const nsCSSProperty* subproptables[3] = {
        nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_color),
        nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_style),
        nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_width)
      };
      PRBool match = PR_TRUE;
      for (const nsCSSProperty** subprops = subproptables,
               **subprops_end = subproptables + NS_ARRAY_LENGTH(subproptables);
           subprops < subprops_end; ++subprops) {
        
        
        const nsCSSValue *firstSide = data->ValueStorageFor((*subprops)[0]);
        for (PRInt32 side = 1; side < 4; ++side) {
          const nsCSSValue *otherSide =
            data->ValueStorageFor((*subprops)[side]);
          if (*firstSide != *otherSide)
            match = PR_FALSE;
        }
      }
      if (!match) {
        
        break;
      }
      
      aProperty = eCSSProperty_border_top;
    }
    case eCSSProperty_border_top:
    case eCSSProperty_border_right:
    case eCSSProperty_border_bottom:
    case eCSSProperty_border_left:
    case eCSSProperty_border_start:
    case eCSSProperty_border_end:
    case eCSSProperty__moz_column_rule:
    case eCSSProperty_outline: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(nsCSSProps::kTypeTable[subprops[0]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[1]] == eCSSType_Value &&
                   nsCSSProps::kTypeTable[subprops[2]] == eCSSType_Value,
                   "type mismatch");
      NS_ASSERTION(StringEndsWith(nsCSSProps::GetStringValue(subprops[2]),
                                  NS_LITERAL_CSTRING("-color")) ||
                   StringEndsWith(nsCSSProps::GetStringValue(subprops[2]),
                                  NS_LITERAL_CSTRING("-color-value")),
                   "third subprop must be the color property");
      const nsCSSValue *colorValue = data->ValueStorageFor(subprops[2]);
      PRBool isMozUseTextColor =
        colorValue->GetUnit() == eCSSUnit_Enumerated &&
        colorValue->GetIntValue() == NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR;
      if (!AppendValueToString(subprops[0], aValue) ||
          !(aValue.Append(PRUnichar(' ')),
            AppendValueToString(subprops[1], aValue)) ||
          
          !(isMozUseTextColor ||
            (aValue.Append(PRUnichar(' ')),
             AppendValueToString(subprops[2], aValue)))) {
        aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_margin_left:
    case eCSSProperty_margin_right:
    case eCSSProperty_margin_start:
    case eCSSProperty_margin_end:
    case eCSSProperty_padding_left:
    case eCSSProperty_padding_right:
    case eCSSProperty_padding_start:
    case eCSSProperty_padding_end:
    case eCSSProperty_border_left_color:
    case eCSSProperty_border_left_style:
    case eCSSProperty_border_left_width:
    case eCSSProperty_border_right_color:
    case eCSSProperty_border_right_style:
    case eCSSProperty_border_right_width:
    case eCSSProperty_border_start_color:
    case eCSSProperty_border_start_style:
    case eCSSProperty_border_start_width:
    case eCSSProperty_border_end_color:
    case eCSSProperty_border_end_style:
    case eCSSProperty_border_end_width: {
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
      NS_ASSERTION(subprops[3] == eCSSProperty_UNKNOWN,
                   "not box property with physical vs. logical cascading");
      AppendValueToString(subprops[0], aValue);
      break;
    }
    case eCSSProperty_background: {
      
      
      
      
      
      
      
      const nsCSSValueList *image =
        * data->ValueListStorageFor(eCSSProperty_background_image);
      const nsCSSValueList *repeat =
        * data->ValueListStorageFor(eCSSProperty_background_repeat);
      const nsCSSValueList *attachment =
        * data->ValueListStorageFor(eCSSProperty_background_attachment);
      const nsCSSValuePairList *position =
        * data->ValuePairListStorageFor(eCSSProperty_background_position);
      const nsCSSValueList *clip =
        * data->ValueListStorageFor(eCSSProperty__moz_background_clip);
      const nsCSSValueList *origin =
        * data->ValueListStorageFor(eCSSProperty__moz_background_origin);
      const nsCSSValuePairList *size =
        * data->ValuePairListStorageFor(eCSSProperty__moz_background_size);
      for (;;) {
        AppendCSSValueToString(eCSSProperty_background_image,
                               image->mValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(eCSSProperty_background_repeat,
                               repeat->mValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(eCSSProperty_background_attachment,
                               attachment->mValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(eCSSProperty_background_position,
                               position->mXValue, aValue);
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(eCSSProperty_background_position,
                               position->mYValue, aValue);
        NS_ASSERTION(clip->mValue.GetUnit() == eCSSUnit_Enumerated &&
                     origin->mValue.GetUnit() == eCSSUnit_Enumerated,
                     "should not be inherit/initial within list and "
                     "should have returned early for real inherit/initial");
        if (clip->mValue.GetIntValue() != NS_STYLE_BG_CLIP_BORDER ||
            origin->mValue.GetIntValue() != NS_STYLE_BG_ORIGIN_PADDING) {
#if 0
    
    
    
    
          PR_STATIC_ASSERT(NS_STYLE_BG_CLIP_BORDER ==
                           NS_STYLE_BG_ORIGIN_BORDER);
          PR_STATIC_ASSERT(NS_STYLE_BG_CLIP_PADDING ==
                           NS_STYLE_BG_ORIGIN_PADDING);
          
          
          if (clip->mValue != origin->mValue) {
            aValue.Truncate();
            return NS_OK;
          }

          aValue.Append(PRUnichar(' '));
          AppendCSSValueToString(eCSSProperty__moz_background_clip,
                                 clip->mValue, aValue);
#else
          aValue.Truncate();
          return NS_OK;
#endif
        }

        image = image->mNext;
        repeat = repeat->mNext;
        attachment = attachment->mNext;
        position = position->mNext;
        clip = clip->mNext;
        origin = origin->mNext;
        size = size->mNext;

        if (!image) {
          if (repeat || attachment || position || clip || origin || size) {
            
            aValue.Truncate();
            return NS_OK;
          }
          break;
        }
        if (!repeat || !attachment || !position || !clip || !origin || !size) {
          
          aValue.Truncate();
          return NS_OK;
        }
        aValue.Append(PRUnichar(','));
        aValue.Append(PRUnichar(' '));
      }

      aValue.Append(PRUnichar(' '));
      AppendValueToString(eCSSProperty_background_color, aValue);
      break;
    }
    case eCSSProperty_cue: {
      if (AppendValueToString(eCSSProperty_cue_before, aValue)) {
        aValue.Append(PRUnichar(' '));
        if (!AppendValueToString(eCSSProperty_cue_after, aValue))
          aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_font: {
      
      
      const nsCSSValue *systemFont =
        data->ValueStorageFor(eCSSProperty__x_system_font);
      const nsCSSValue &style =
        *data->ValueStorageFor(eCSSProperty_font_style);
      const nsCSSValue &variant =
        *data->ValueStorageFor(eCSSProperty_font_variant);
      const nsCSSValue &weight =
        *data->ValueStorageFor(eCSSProperty_font_weight);
      const nsCSSValue &size =
        *data->ValueStorageFor(eCSSProperty_font_size);
      const nsCSSValue &lh =
        *data->ValueStorageFor(eCSSProperty_line_height);
      const nsCSSValue &family =
        *data->ValueStorageFor(eCSSProperty_font_family);
      const nsCSSValue &stretch =
        *data->ValueStorageFor(eCSSProperty_font_stretch);
      const nsCSSValue &sizeAdjust =
        *data->ValueStorageFor(eCSSProperty_font_size_adjust);

      if (systemFont &&
          systemFont->GetUnit() != eCSSUnit_None &&
          systemFont->GetUnit() != eCSSUnit_Null) {
        if (style.GetUnit() != eCSSUnit_System_Font ||
            variant.GetUnit() != eCSSUnit_System_Font ||
            weight.GetUnit() != eCSSUnit_System_Font ||
            size.GetUnit() != eCSSUnit_System_Font ||
            lh.GetUnit() != eCSSUnit_System_Font ||
            family.GetUnit() != eCSSUnit_System_Font ||
            stretch.GetUnit() != eCSSUnit_System_Font ||
            sizeAdjust.GetUnit() != eCSSUnit_System_Font) {
          
          return NS_OK;
        }
        AppendCSSValueToString(eCSSProperty__x_system_font, *systemFont,
                               aValue);
      } else {
        
        
        
        if (stretch != nsCSSValue(eCSSUnit_Normal) ||
            sizeAdjust != nsCSSValue(eCSSUnit_None)) {
          return NS_OK;
        }

        if (style.GetUnit() != eCSSUnit_Normal) {
          AppendCSSValueToString(eCSSProperty_font_style, style, aValue);
          aValue.Append(PRUnichar(' '));
        }
        if (variant.GetUnit() != eCSSUnit_Normal) {
          AppendCSSValueToString(eCSSProperty_font_variant, variant, aValue);
          aValue.Append(PRUnichar(' '));
        }
        if (weight.GetUnit() != eCSSUnit_Normal) {
          AppendCSSValueToString(eCSSProperty_font_weight, weight, aValue);
          aValue.Append(PRUnichar(' '));
        }
        AppendCSSValueToString(eCSSProperty_font_size, size, aValue);
        if (lh.GetUnit() != eCSSUnit_Normal) {
          aValue.Append(PRUnichar('/'));
          AppendCSSValueToString(eCSSProperty_line_height, lh, aValue);
        }
        aValue.Append(PRUnichar(' '));
        AppendCSSValueToString(eCSSProperty_font_family, family, aValue);
      }
      break;
    }
    case eCSSProperty_list_style:
      if (AppendValueToString(eCSSProperty_list_style_type, aValue))
        aValue.Append(PRUnichar(' '));
      if (AppendValueToString(eCSSProperty_list_style_position, aValue))
        aValue.Append(PRUnichar(' '));
      AppendValueToString(eCSSProperty_list_style_image, aValue);
      break;
    case eCSSProperty_overflow: {
      const nsCSSValue &xValue =
        *data->ValueStorageFor(eCSSProperty_overflow_x);
      const nsCSSValue &yValue =
        *data->ValueStorageFor(eCSSProperty_overflow_y);
      if (xValue == yValue)
        AppendCSSValueToString(eCSSProperty_overflow_x, xValue, aValue);
      break;
    }
    case eCSSProperty_pause: {
      if (AppendValueToString(eCSSProperty_pause_before, aValue)) {
        aValue.Append(PRUnichar(' '));
        if (!AppendValueToString(eCSSProperty_pause_after, aValue))
          aValue.Truncate();
      }
      break;
    }
    case eCSSProperty_transition: {
#define NUM_TRANSITION_SUBPROPS 4
      const nsCSSProperty* subprops =
        nsCSSProps::SubpropertyEntryFor(aProperty);
#ifdef DEBUG
      for (int i = 0; i < NUM_TRANSITION_SUBPROPS; ++i) {
        NS_ASSERTION(nsCSSProps::kTypeTable[subprops[i]] == eCSSType_ValueList,
                     "type mismatch");
      }
      NS_ASSERTION(subprops[NUM_TRANSITION_SUBPROPS] == eCSSProperty_UNKNOWN,
                   "length mismatch");
#endif
      const nsCSSValueList* val[NUM_TRANSITION_SUBPROPS];
      for (int i = 0; i < NUM_TRANSITION_SUBPROPS; ++i) {
        val[i] = *data->ValueListStorageFor(subprops[i]);
      }
      
      for (;;) {
        for (int i = 0; i < NUM_TRANSITION_SUBPROPS; ++i) {
          AppendCSSValueToString(subprops[i], val[i]->mValue, aValue);
          aValue.Append(PRUnichar(' '));
          val[i] = val[i]->mNext;
        }
        
        aValue.Truncate(aValue.Length() - 1);

        PR_STATIC_ASSERT(NUM_TRANSITION_SUBPROPS == 4);
        if (!val[0] || !val[1] || !val[2] || !val[3]) {
          break;
        }
        aValue.AppendLiteral(", ");
      }

      PR_STATIC_ASSERT(NUM_TRANSITION_SUBPROPS == 4);
      if (val[0] || val[1] || val[2] || val[3]) {
        
        
        aValue.Truncate();
      }
#undef NUM_TRANSITION_SUBPROPS
      break;
    }

#ifdef MOZ_SVG
    case eCSSProperty_marker: {
      const nsCSSValue &endValue =
        *data->ValueStorageFor(eCSSProperty_marker_end);
      const nsCSSValue &midValue =
        *data->ValueStorageFor(eCSSProperty_marker_mid);
      const nsCSSValue &startValue =
        *data->ValueStorageFor(eCSSProperty_marker_start);
      if (endValue == midValue && midValue == startValue)
        AppendValueToString(eCSSProperty_marker_end, aValue);
      break;
    }
#endif
    default:
      NS_NOTREACHED("no other shorthands");
      break;
  }
  return NS_OK;
}

PRBool
nsCSSDeclaration::GetValueIsImportant(const nsAString& aProperty) const
{
  nsCSSProperty propID = nsCSSProps::LookupProperty(aProperty);
  return GetValueIsImportant(propID);
}

PRBool
nsCSSDeclaration::GetValueIsImportant(nsCSSProperty aProperty) const
{
  if (!mImportantData)
    return PR_FALSE;

  
  

  if (nsCSSProps::IsShorthand(aProperty)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aProperty) {
      if (*p == eCSSProperty__x_system_font) {
        
        continue;
      }
      if (!mImportantData->StorageFor(*p)) {
        return PR_FALSE;
      }
    }
    return PR_TRUE;
  }

  return mImportantData->StorageFor(aProperty) != nsnull;
}

 void
nsCSSDeclaration::AppendImportanceToString(PRBool aIsImportant,
                                           nsAString& aString)
{
  if (aIsImportant) {
   aString.AppendLiteral(" ! important");
  }
}

void
nsCSSDeclaration::AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                                 nsAutoString& aValue,
                                                 nsAString& aResult) const
{
  NS_ASSERTION(0 <= aProperty && aProperty < eCSSProperty_COUNT,
               "property enum out of range");
  NS_ASSERTION((aProperty < eCSSProperty_COUNT_no_shorthands) ==
                 aValue.IsEmpty(),
               "aValue should be given for shorthands but not longhands");
  AppendASCIItoUTF16(nsCSSProps::GetStringValue(aProperty), aResult);
  aResult.AppendLiteral(": ");
  if (aValue.IsEmpty())
    AppendValueToString(aProperty, aResult);
  else
    aResult.Append(aValue);
  PRBool  isImportant = GetValueIsImportant(aProperty);
  AppendImportanceToString(isImportant, aResult);
  aResult.AppendLiteral("; ");
}

nsresult
nsCSSDeclaration::ToString(nsAString& aString) const
{
  nsCSSCompressedDataBlock *systemFontData =
    GetValueIsImportant(eCSSProperty__x_system_font) ? mImportantData : mData;
  const nsCSSValue *systemFont = 
    systemFontData->ValueStorageFor(eCSSProperty__x_system_font);
  const PRBool haveSystemFont = systemFont &&
                                systemFont->GetUnit() != eCSSUnit_None &&
                                systemFont->GetUnit() != eCSSUnit_Null;
  PRBool didSystemFont = PR_FALSE;

  PRInt32 count = mOrder.Length();
  PRInt32 index;
  nsAutoTArray<nsCSSProperty, 16> shorthandsUsed;
  for (index = 0; index < count; index++) {
    nsCSSProperty property = OrderValueAt(index);
    PRBool doneProperty = PR_FALSE;

    
    if (shorthandsUsed.Length() > 0) {
      for (const nsCSSProperty *shorthands =
             nsCSSProps::ShorthandsContaining(property);
           *shorthands != eCSSProperty_UNKNOWN; ++shorthands) {
        if (shorthandsUsed.Contains(*shorthands)) {
          doneProperty = PR_TRUE;
          break;
        }
      }
      if (doneProperty)
        continue;
    }

    
    nsAutoString value;
    for (const nsCSSProperty *shorthands =
           nsCSSProps::ShorthandsContaining(property);
         *shorthands != eCSSProperty_UNKNOWN; ++shorthands) {
      
      
      
      nsCSSProperty shorthand = *shorthands;

      
      
      GetValue(shorthand, value);
      if (!value.IsEmpty()) {
        AppendPropertyAndValueToString(shorthand, value, aString);
        shorthandsUsed.AppendElement(shorthand);
        doneProperty = PR_TRUE;
        break;
      }

      NS_ASSERTION(shorthand != eCSSProperty_font ||
                   *(shorthands + 1) == eCSSProperty_UNKNOWN,
                   "font should always be the only containing shorthand");
      if (shorthand == eCSSProperty_font) {
        if (haveSystemFont && !didSystemFont) {
          
          
          
          AppendCSSValueToString(eCSSProperty__x_system_font, *systemFont,
                                 value);
          AppendPropertyAndValueToString(eCSSProperty_font, value, aString);
          value.Truncate();
          didSystemFont = PR_TRUE;
        }

        
        
        
        
        
        
        NS_ASSERTION(nsCSSProps::kTypeTable[property] == eCSSType_Value,
                     "not a value typed subproperty");
        const nsCSSValue *val =
          systemFontData->ValueStorageFor(property);
        if (property == eCSSProperty__x_system_font ||
            (haveSystemFont && val && val->GetUnit() == eCSSUnit_System_Font)) {
          doneProperty = PR_TRUE;
        }
      }
    }
    if (doneProperty)
      continue;
    
    NS_ASSERTION(value.IsEmpty(), "value should be empty now");
    AppendPropertyAndValueToString(property, value, aString);
  }
  if (! aString.IsEmpty()) {
    
    aString.Truncate(aString.Length() - 1);
  }
  return NS_OK;
}

#ifdef DEBUG
void nsCSSDeclaration::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("{ ", out);
  nsAutoString s;
  ToString(s);
  fputs(NS_ConvertUTF16toUTF8(s).get(), out);
  fputs("}", out);
}
#endif

nsresult
nsCSSDeclaration::GetNthProperty(PRUint32 aIndex, nsAString& aReturn) const
{
  aReturn.Truncate();
  if (aIndex < mOrder.Length()) {
    nsCSSProperty property = OrderValueAt(aIndex);
    if (0 <= property) {
      AppendASCIItoUTF16(nsCSSProps::GetStringValue(property), aReturn);
    }
  }
  
  return NS_OK;
}

nsCSSDeclaration*
nsCSSDeclaration::Clone() const
{
  return new nsCSSDeclaration(*this);
}

PRBool
nsCSSDeclaration::InitializeEmpty()
{
  NS_ASSERTION(!mData && !mImportantData, "already initialized");
  mData = nsCSSCompressedDataBlock::CreateEmptyBlock();
  return mData != nsnull;
}
