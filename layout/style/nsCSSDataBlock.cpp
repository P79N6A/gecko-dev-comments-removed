









































#include "nsCSSDataBlock.h"
#include "mozilla/css/Declaration.h"
#include "nsRuleData.h"
#include "nsStyleSet.h"
#include "nsStyleContext.h"

namespace css = mozilla::css;

enum {
    CDBValueStorage_advance = sizeof(CDBValueStorage)
};









inline nsCSSProperty& PropertyAtCursor(char *aCursor) {
    return *reinterpret_cast<nsCSSProperty*>(aCursor);
}

inline nsCSSProperty PropertyAtCursor(const char *aCursor) {
    return *reinterpret_cast<const nsCSSProperty*>(aCursor);
}

inline nsCSSValue* ValueAtCursor(char *aCursor) {
    return & reinterpret_cast<CDBValueStorage*>(aCursor)->value;
}

inline const nsCSSValue* ValueAtCursor(const char *aCursor) {
    return & reinterpret_cast<const CDBValueStorage*>(aCursor)->value;
}







static bool
MoveValue(nsCSSValue* aSource, nsCSSValue* aDest)
{
    bool changed = (*aSource != *aDest);
    aDest->~nsCSSValue();
    memcpy(aDest, aSource, sizeof(nsCSSValue));
    new (aSource) nsCSSValue();
    return changed;
}

static bool
ShouldIgnoreColors(nsRuleData *aRuleData)
{
    return aRuleData->mLevel != nsStyleSet::eAgentSheet &&
           aRuleData->mLevel != nsStyleSet::eUserSheet &&
           !aRuleData->mPresContext->UseDocumentColors();
}





static void
TryToStartImageLoadOnValue(const nsCSSValue& aValue, nsIDocument* aDocument)
{
  if (aValue.GetUnit() == eCSSUnit_URL) {
    aValue.StartImageLoad(aDocument);
  }
  else if (aValue.EqualsFunction(eCSSKeyword__moz_image_rect)) {
    nsCSSValue::Array* arguments = aValue.GetArrayValue();
    NS_ABORT_IF_FALSE(arguments->Count() == 6, "unexpected num of arguments");

    const nsCSSValue& image = arguments->Item(1);
    if (image.GetUnit() == eCSSUnit_URL)
      image.StartImageLoad(aDocument);
  }
}

static void
TryToStartImageLoad(const nsCSSValue& aValue, nsIDocument* aDocument,
                    nsCSSProperty aProperty)
{
  if (aValue.GetUnit() == eCSSUnit_List) {
    for (const nsCSSValueList* l = aValue.GetListValue(); l; l = l->mNext) {
      TryToStartImageLoad(l->mValue, aDocument, aProperty);
    }
  } else if (nsCSSProps::PropHasFlags(aProperty,
                                      CSS_PROPERTY_IMAGE_IS_IN_ARRAY_0)) {
    if (aValue.GetUnit() == eCSSUnit_Array) {
      TryToStartImageLoadOnValue(aValue.GetArrayValue()->Item(0), aDocument);
    }
  } else {
    TryToStartImageLoadOnValue(aValue, aDocument);
  }
}

static inline bool
ShouldStartImageLoads(nsRuleData *aRuleData, nsCSSProperty aProperty)
{
  
  
  
  
  
  
  
  return !aRuleData->mStyleContext->IsStyleIfVisited() &&
         nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_START_IMAGE_LOADS);
}

void
nsCSSCompressedDataBlock::MapRuleInfoInto(nsRuleData *aRuleData) const
{
    
    
    
    
    if (!(aRuleData->mSIDs & mStyleBits))
        return;

    nsIDocument* doc = aRuleData->mPresContext->Document();

    const char* cursor = Block();
    const char* cursor_end = BlockEnd();
    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
        if (nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[iProp]) &
            aRuleData->mSIDs) {
            nsCSSValue* target = aRuleData->ValueFor(iProp);
            if (target->GetUnit() == eCSSUnit_Null) {
                const nsCSSValue *val = ValueAtCursor(cursor);
                NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null, "oops");
                if (ShouldStartImageLoads(aRuleData, iProp)) {
                    TryToStartImageLoad(*val, doc, iProp);
                }
                *target = *val;
                if (nsCSSProps::PropHasFlags(iProp,
                        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED) &&
                    ShouldIgnoreColors(aRuleData))
                {
                    if (iProp == eCSSProperty_background_color) {
                        
                        
                        if (target->IsNonTransparentColor()) {
                            target->SetColorValue(aRuleData->mPresContext->
                                                  DefaultBackgroundColor());
                        }
                    } else {
                        
                        *target = nsCSSValue();
                    }
                }
            }
        }
        cursor += CDBValueStorage_advance;
    }
    NS_ABORT_IF_FALSE(cursor == cursor_end, "inconsistent data");
}

const nsCSSValue*
nsCSSCompressedDataBlock::ValueFor(nsCSSProperty aProperty) const
{
    NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty),
                      "Don't call for shorthands");

    
    
    
    
    if (!(nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[aProperty]) &
          mStyleBits))
        return nsnull;

    const char* cursor = Block();
    const char* cursor_end = BlockEnd();
    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
        if (iProp == aProperty) {
            return ValueAtCursor(cursor);
        }
        cursor += CDBValueStorage_advance;
    }
    NS_ABORT_IF_FALSE(cursor == cursor_end, "inconsistent data");

    return nsnull;
}

bool
nsCSSCompressedDataBlock::TryReplaceValue(nsCSSProperty aProperty,
                                          nsCSSExpandedDataBlock& aFromBlock,
                                          bool *aChanged)
{
    nsCSSValue* newValue = aFromBlock.PropertyAt(aProperty);
    NS_ABORT_IF_FALSE(newValue && newValue->GetUnit() != eCSSUnit_Null,
                      "cannot replace with empty value");

    const nsCSSValue* oldValue = ValueFor(aProperty);
    if (!oldValue) {
        *aChanged = false;
        return false;
    }

    *aChanged = MoveValue(newValue, const_cast<nsCSSValue*>(oldValue));
    aFromBlock.ClearPropertyBit(aProperty);
    return true;
}

nsCSSCompressedDataBlock*
nsCSSCompressedDataBlock::Clone() const
{
    const char *cursor = Block(), *cursor_end = BlockEnd();
    char *result_cursor;

    nsAutoPtr<nsCSSCompressedDataBlock> result
        (new(cursor_end - cursor) nsCSSCompressedDataBlock());
    if (!result)
        return nsnull;
    result_cursor = result->Block();

    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
        PropertyAtCursor(result_cursor) = iProp;

        const nsCSSValue* val = ValueAtCursor(cursor);
        nsCSSValue *result_val = ValueAtCursor(result_cursor);
        new (result_val) nsCSSValue(*val);
        cursor += CDBValueStorage_advance;
        result_cursor +=  CDBValueStorage_advance;
    }
    NS_ABORT_IF_FALSE(cursor == cursor_end, "inconsistent data");

    result->SetBlockEnd(result_cursor);
    result->mStyleBits = mStyleBits;
    NS_ABORT_IF_FALSE(result->DataSize() == DataSize(), "wrong size");

    return result.forget();
}

nsCSSCompressedDataBlock::~nsCSSCompressedDataBlock()
{
    const char* cursor = Block();
    const char* cursor_end = BlockEnd();
    while (cursor < cursor_end) {
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(PropertyAtCursor(cursor)),
                          "out of range");

        const nsCSSValue* val = ValueAtCursor(cursor);
        NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null, "oops");
        val->~nsCSSValue();
        cursor += CDBValueStorage_advance;
    }
    NS_ABORT_IF_FALSE(cursor == cursor_end, "inconsistent data");
}

 nsCSSCompressedDataBlock*
nsCSSCompressedDataBlock::CreateEmptyBlock()
{
    nsCSSCompressedDataBlock *result = new(0) nsCSSCompressedDataBlock();
    result->SetBlockEnd(result->Block());
    return result;
}



nsCSSExpandedDataBlock::nsCSSExpandedDataBlock()
{
    AssertInitialState();
}

nsCSSExpandedDataBlock::~nsCSSExpandedDataBlock()
{
    AssertInitialState();
}

void
nsCSSExpandedDataBlock::DoExpand(nsCSSCompressedDataBlock *aBlock,
                                 bool aImportant)
{
    



    const char* cursor = aBlock->Block();
    const char* cursor_end = aBlock->BlockEnd();
    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
        NS_ABORT_IF_FALSE(!HasPropertyBit(iProp),
                          "compressed block has property multiple times");
        SetPropertyBit(iProp);
        if (aImportant)
            SetImportantBit(iProp);

        const nsCSSValue* val = ValueAtCursor(cursor);
        nsCSSValue* dest = PropertyAt(iProp);
        NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null, "oops");
        NS_ABORT_IF_FALSE(dest->GetUnit() == eCSSUnit_Null,
                          "expanding into non-empty block");
#ifdef NS_BUILD_REFCNT_LOGGING
        dest->~nsCSSValue();
#endif
        memcpy(dest, val, sizeof(nsCSSValue));
        cursor += CDBValueStorage_advance;
    }
    NS_ABORT_IF_FALSE(cursor == cursor_end, "inconsistent data");

    
    aBlock->SetBlockEnd(aBlock->Block());
    delete aBlock;
}

void
nsCSSExpandedDataBlock::Expand(nsCSSCompressedDataBlock *aNormalBlock,
                               nsCSSCompressedDataBlock *aImportantBlock)
{
    NS_ABORT_IF_FALSE(aNormalBlock, "unexpected null block");
    AssertInitialState();

    DoExpand(aNormalBlock, false);
    if (aImportantBlock) {
        DoExpand(aImportantBlock, true);
    }
}

nsCSSExpandedDataBlock::ComputeSizeResult
nsCSSExpandedDataBlock::ComputeSize()
{
    ComputeSizeResult result = {0, 0};
    for (size_t iHigh = 0; iHigh < nsCSSPropertySet::kChunkCount; ++iHigh) {
        if (!mPropertiesSet.HasPropertyInChunk(iHigh))
            continue;
        for (size_t iLow = 0; iLow < nsCSSPropertySet::kBitsInChunk; ++iLow) {
            if (!mPropertiesSet.HasPropertyAt(iHigh, iLow))
                continue;
#ifdef DEBUG
            nsCSSProperty iProp = nsCSSPropertySet::CSSPropertyAt(iHigh, iLow);
#endif
            NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
            NS_ABORT_IF_FALSE(PropertyAt(iProp)->GetUnit() != eCSSUnit_Null,
                              "null value while computing size");
            if (mPropertiesImportant.HasPropertyAt(iHigh, iLow))
                result.important += CDBValueStorage_advance;
            else
                result.normal += CDBValueStorage_advance;
        }
    }
    return result;
}

void
nsCSSExpandedDataBlock::Compress(nsCSSCompressedDataBlock **aNormalBlock,
                                 nsCSSCompressedDataBlock **aImportantBlock)
{
    nsAutoPtr<nsCSSCompressedDataBlock> result_normal, result_important;
    char *cursor_normal, *cursor_important;

    ComputeSizeResult size = ComputeSize();

    result_normal = new(size.normal) nsCSSCompressedDataBlock();
    cursor_normal = result_normal->Block();

    if (size.important != 0) {
        result_important = new(size.important) nsCSSCompressedDataBlock();
        cursor_important = result_important->Block();
    } else {
        result_important = nsnull;
        cursor_important = nsnull;
    }

    




    for (size_t iHigh = 0; iHigh < nsCSSPropertySet::kChunkCount; ++iHigh) {
        if (!mPropertiesSet.HasPropertyInChunk(iHigh))
            continue;
        for (size_t iLow = 0; iLow < nsCSSPropertySet::kBitsInChunk; ++iLow) {
            if (!mPropertiesSet.HasPropertyAt(iHigh, iLow))
                continue;
            nsCSSProperty iProp = nsCSSPropertySet::CSSPropertyAt(iHigh, iLow);
            NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
            bool important =
                mPropertiesImportant.HasPropertyAt(iHigh, iLow);
            char *&cursor = important ? cursor_important : cursor_normal;
            nsCSSCompressedDataBlock *result =
                important ? result_important : result_normal;
            nsCSSValue* val = PropertyAt(iProp);
            NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null,
                              "Null value while compressing");
            CDBValueStorage *storage =
                reinterpret_cast<CDBValueStorage*>(cursor);
            storage->property = iProp;
            memcpy(&storage->value, val, sizeof(nsCSSValue));
            new (val) nsCSSValue();
            cursor += CDBValueStorage_advance;
            result->mStyleBits |=
                nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[iProp]);
        }
    }

    result_normal->SetBlockEnd(cursor_normal);
    NS_ABORT_IF_FALSE(result_normal->DataSize() == ptrdiff_t(size.normal),
                      "size miscalculation");

    if (result_important) {
        result_important->SetBlockEnd(cursor_important);
        NS_ABORT_IF_FALSE(result_important->DataSize() ==
                          ptrdiff_t(size.important),
                          "size miscalculation");
    }

    ClearSets();
    AssertInitialState();
    *aNormalBlock = result_normal.forget();
    *aImportantBlock = result_important.forget();
}

void
nsCSSExpandedDataBlock::AddLonghandProperty(nsCSSProperty aProperty,
                                            const nsCSSValue& aValue)
{
    NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty),
                      "property out of range");
    nsCSSValue& storage = *static_cast<nsCSSValue*>(PropertyAt(aProperty));
    storage = aValue;
    SetPropertyBit(aProperty);
}

void
nsCSSExpandedDataBlock::Clear()
{
    for (size_t iHigh = 0; iHigh < nsCSSPropertySet::kChunkCount; ++iHigh) {
        if (!mPropertiesSet.HasPropertyInChunk(iHigh))
            continue;
        for (size_t iLow = 0; iLow < nsCSSPropertySet::kBitsInChunk; ++iLow) {
            if (!mPropertiesSet.HasPropertyAt(iHigh, iLow))
                continue;
            nsCSSProperty iProp = nsCSSPropertySet::CSSPropertyAt(iHigh, iLow);
            ClearLonghandProperty(iProp);
        }
    }

    AssertInitialState();
}

void
nsCSSExpandedDataBlock::ClearProperty(nsCSSProperty aPropID)
{
  if (nsCSSProps::IsShorthand(aPropID)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aPropID) {
      ClearLonghandProperty(*p);
    }
  } else {
    ClearLonghandProperty(aPropID);
  }
}

void
nsCSSExpandedDataBlock::ClearLonghandProperty(nsCSSProperty aPropID)
{
    NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aPropID), "out of range");

    ClearPropertyBit(aPropID);
    ClearImportantBit(aPropID);
    PropertyAt(aPropID)->Reset();
}

bool
nsCSSExpandedDataBlock::TransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                                          nsCSSProperty aPropID,
                                          bool aIsImportant,
                                          bool aOverrideImportant,
                                          bool aMustCallValueAppended,
                                          css::Declaration* aDeclaration)
{
    if (!nsCSSProps::IsShorthand(aPropID)) {
        return DoTransferFromBlock(aFromBlock, aPropID,
                                   aIsImportant, aOverrideImportant,
                                   aMustCallValueAppended, aDeclaration);
    }

    bool changed = false;
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aPropID) {
        changed |= DoTransferFromBlock(aFromBlock, *p,
                                       aIsImportant, aOverrideImportant,
                                       aMustCallValueAppended, aDeclaration);
    }
    return changed;
}

bool
nsCSSExpandedDataBlock::DoTransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                                            nsCSSProperty aPropID,
                                            bool aIsImportant,
                                            bool aOverrideImportant,
                                            bool aMustCallValueAppended,
                                            css::Declaration* aDeclaration)
{
  bool changed = false;
  NS_ABORT_IF_FALSE(aFromBlock.HasPropertyBit(aPropID), "oops");
  if (aIsImportant) {
    if (!HasImportantBit(aPropID))
      changed = true;
    SetImportantBit(aPropID);
  } else {
    if (HasImportantBit(aPropID)) {
      
      
      
      
      
      if (!aOverrideImportant) {
        aFromBlock.ClearLonghandProperty(aPropID);
        return false;
      }
      changed = true;
      ClearImportantBit(aPropID);
    }
  }

  if (aMustCallValueAppended || !HasPropertyBit(aPropID)) {
    aDeclaration->ValueAppended(aPropID);
  }

  SetPropertyBit(aPropID);
  aFromBlock.ClearPropertyBit(aPropID);

  




  changed |= MoveValue(aFromBlock.PropertyAt(aPropID), PropertyAt(aPropID));
  return changed;
}

#ifdef DEBUG
void
nsCSSExpandedDataBlock::DoAssertInitialState()
{
    mPropertiesSet.AssertIsEmpty("not initial state");
    mPropertiesImportant.AssertIsEmpty("not initial state");

    for (PRUint32 i = 0; i < eCSSProperty_COUNT_no_shorthands; ++i) {
        nsCSSProperty prop = nsCSSProperty(i);
        NS_ABORT_IF_FALSE(PropertyAt(prop)->GetUnit() == eCSSUnit_Null,
                          "not initial state");
    }
}
#endif
