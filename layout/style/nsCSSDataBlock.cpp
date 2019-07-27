









#include "nsCSSDataBlock.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/css/Declaration.h"
#include "mozilla/css/ImageLoader.h"
#include "nsRuleData.h"
#include "nsStyleSet.h"
#include "nsStyleContext.h"
#include "nsIDocument.h"
#include "WritingModes.h"

using namespace mozilla;







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
TryToStartImageLoadOnValue(const nsCSSValue& aValue, nsIDocument* aDocument,
                           nsCSSValueTokenStream* aTokenStream)
{
  MOZ_ASSERT(aDocument);

  if (aValue.GetUnit() == eCSSUnit_URL) {
    aValue.StartImageLoad(aDocument);
    if (aTokenStream) {
      aTokenStream->mImageValues.PutEntry(aValue.GetImageStructValue());
    }
  }
  else if (aValue.GetUnit() == eCSSUnit_Image) {
    
    imgIRequest* request = aValue.GetImageValue(nullptr);

    if (request) {
      mozilla::css::ImageValue* imageValue = aValue.GetImageStructValue();
      aDocument->StyleImageLoader()->MaybeRegisterCSSImage(imageValue);
      if (aTokenStream) {
        aTokenStream->mImageValues.PutEntry(imageValue);
      }
    }
  }
  else if (aValue.EqualsFunction(eCSSKeyword__moz_image_rect)) {
    nsCSSValue::Array* arguments = aValue.GetArrayValue();
    NS_ABORT_IF_FALSE(arguments->Count() == 6, "unexpected num of arguments");

    const nsCSSValue& image = arguments->Item(1);
    TryToStartImageLoadOnValue(image, aDocument, aTokenStream);
  }
}

static void
TryToStartImageLoad(const nsCSSValue& aValue, nsIDocument* aDocument,
                    nsCSSProperty aProperty,
                    nsCSSValueTokenStream* aTokenStream)
{
  if (aValue.GetUnit() == eCSSUnit_List) {
    for (const nsCSSValueList* l = aValue.GetListValue(); l; l = l->mNext) {
      TryToStartImageLoad(l->mValue, aDocument, aProperty, aTokenStream);
    }
  } else if (nsCSSProps::PropHasFlags(aProperty,
                                      CSS_PROPERTY_IMAGE_IS_IN_ARRAY_0)) {
    if (aValue.GetUnit() == eCSSUnit_Array) {
      TryToStartImageLoadOnValue(aValue.GetArrayValue()->Item(0), aDocument,
                                 aTokenStream);
    }
  } else {
    TryToStartImageLoadOnValue(aValue, aDocument, aTokenStream);
  }
}

static inline bool
ShouldStartImageLoads(nsRuleData *aRuleData, nsCSSProperty aProperty)
{
  
  
  
  
  
  
  
  return !aRuleData->mStyleContext->IsStyleIfVisited() &&
         nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_START_IMAGE_LOADS);
}

static void
MapSinglePropertyInto(nsCSSProperty aProp,
                      const nsCSSValue* aValue,
                      nsCSSValue* aTarget,
                      nsRuleData* aRuleData)
{
    NS_ABORT_IF_FALSE(aValue->GetUnit() != eCSSUnit_Null, "oops");

    
    
    
    
    
    
    
    
    NS_ABORT_IF_FALSE(aTarget->GetUnit() == eCSSUnit_TokenStream ||
                      aTarget->GetUnit() == eCSSUnit_Null,
                      "aTarget must only be a token stream (when re-parsing "
                      "properties with variable references) or null");

    nsCSSValueTokenStream* tokenStream =
        aTarget->GetUnit() == eCSSUnit_TokenStream ?
            aTarget->GetTokenStreamValue() :
            nullptr;

    if (ShouldStartImageLoads(aRuleData, aProp)) {
        nsIDocument* doc = aRuleData->mPresContext->Document();
        TryToStartImageLoad(*aValue, doc, aProp, tokenStream);
    }
    *aTarget = *aValue;
    if (nsCSSProps::PropHasFlags(aProp,
            CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED) &&
        ShouldIgnoreColors(aRuleData))
    {
        if (aProp == eCSSProperty_background_color) {
            
            
            if (aTarget->IsNonTransparentColor()) {
                aTarget->SetColorValue(aRuleData->mPresContext->
                                       DefaultBackgroundColor());
            }
        } else {
            
            *aTarget = nsCSSValue();
        }
    }
}






static inline void
EnsurePhysicalProperty(nsCSSProperty& aProperty, nsRuleData* aRuleData)
{
  bool isBlock =
    nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_LOGICAL_BLOCK_AXIS);
  bool isEnd =
    nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_LOGICAL_END_EDGE);

  LogicalEdge edge = isEnd ? eLogicalEdgeEnd : eLogicalEdgeStart;

  
  
  
  mozilla::css::Side side;
  if (isBlock) {
    uint8_t wm = aRuleData->mStyleContext->StyleVisibility()->mWritingMode;
    side = WritingMode::PhysicalSideForBlockAxis(wm, edge);
  } else {
    WritingMode wm(aRuleData->mStyleContext);
    side = wm.PhysicalSideForInlineAxis(edge);
  }

  nsCSSProperty shorthand = nsCSSProps::BoxShorthandFor(aProperty);
  const nsCSSProperty* subprops = nsCSSProps::SubpropertyEntryFor(shorthand);
  MOZ_ASSERT(subprops[0] != eCSSProperty_UNKNOWN &&
             subprops[1] != eCSSProperty_UNKNOWN &&
             subprops[2] != eCSSProperty_UNKNOWN &&
             subprops[3] != eCSSProperty_UNKNOWN &&
             subprops[4] == eCSSProperty_UNKNOWN,
             "expected four-element subproperty table");
  aProperty = subprops[side];
}

void
nsCSSCompressedDataBlock::MapRuleInfoInto(nsRuleData *aRuleData) const
{
    
    
    
    
    if (!(aRuleData->mSIDs & mStyleBits))
        return;

    
    
    
    for (uint32_t i = mNumProps; i-- > 0; ) {
        nsCSSProperty iProp = PropertyAtIndex(i);
        if (nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[iProp]) &
            aRuleData->mSIDs) {
            if (nsCSSProps::PropHasFlags(iProp, CSS_PROPERTY_LOGICAL)) {
                EnsurePhysicalProperty(iProp, aRuleData);
                
                
                
                aRuleData->mCanStoreInRuleTree = false;
            }
            nsCSSValue* target = aRuleData->ValueFor(iProp);
            if (target->GetUnit() == eCSSUnit_Null) {
                const nsCSSValue *val = ValueAtIndex(i);
                MapSinglePropertyInto(iProp, val, target, aRuleData);
            }
        }
    }
}

const nsCSSValue*
nsCSSCompressedDataBlock::ValueFor(nsCSSProperty aProperty) const
{
    NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty),
                      "Don't call for shorthands");

    
    
    
    
    if (!(nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[aProperty]) &
          mStyleBits))
        return nullptr;

    for (uint32_t i = 0; i < mNumProps; i++) {
        if (PropertyAtIndex(i) == aProperty) {
            return ValueAtIndex(i);
        }
    }

    return nullptr;
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
    nsAutoPtr<nsCSSCompressedDataBlock>
        result(new(mNumProps) nsCSSCompressedDataBlock(mNumProps));

    result->mStyleBits = mStyleBits;

    for (uint32_t i = 0; i < mNumProps; i++) {
        result->SetPropertyAtIndex(i, PropertyAtIndex(i));
        result->CopyValueToIndex(i, ValueAtIndex(i));
    }

    return result.forget();
}

nsCSSCompressedDataBlock::~nsCSSCompressedDataBlock()
{
    for (uint32_t i = 0; i < mNumProps; i++) {
#ifdef DEBUG
        (void)PropertyAtIndex(i);   
#endif
        const nsCSSValue* val = ValueAtIndex(i);
        NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null, "oops");
        val->~nsCSSValue();
    }
}

 nsCSSCompressedDataBlock*
nsCSSCompressedDataBlock::CreateEmptyBlock()
{
    nsCSSCompressedDataBlock *result = new(0) nsCSSCompressedDataBlock(0);
    return result;
}

size_t
nsCSSCompressedDataBlock::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t n = aMallocSizeOf(this);
    for (uint32_t i = 0; i < mNumProps; i++) {
        n += ValueAtIndex(i)->SizeOfExcludingThis(aMallocSizeOf);
    }
    return n;
}

bool
nsCSSCompressedDataBlock::HasDefaultBorderImageSlice() const
{
  const nsCSSValueList *slice =
    ValueFor(eCSSProperty_border_image_slice)->GetListValue();
  return !slice->mNext &&
         slice->mValue.GetRectValue().AllSidesEqualTo(
           nsCSSValue(1.0f, eCSSUnit_Percent));
}

bool
nsCSSCompressedDataBlock::HasDefaultBorderImageWidth() const
{
  const nsCSSRect &width =
    ValueFor(eCSSProperty_border_image_width)->GetRectValue();
  return width.AllSidesEqualTo(nsCSSValue(1.0f, eCSSUnit_Number));
}

bool
nsCSSCompressedDataBlock::HasDefaultBorderImageOutset() const
{
  const nsCSSRect &outset =
    ValueFor(eCSSProperty_border_image_outset)->GetRectValue();
  return outset.AllSidesEqualTo(nsCSSValue(0.0f, eCSSUnit_Number));
}

bool
nsCSSCompressedDataBlock::HasDefaultBorderImageRepeat() const
{
  const nsCSSValuePair &repeat =
    ValueFor(eCSSProperty_border_image_repeat)->GetPairValue();
  return repeat.BothValuesEqualTo(
    nsCSSValue(NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH, eCSSUnit_Enumerated));
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
    



    for (uint32_t i = 0; i < aBlock->mNumProps; i++) {
        nsCSSProperty iProp = aBlock->PropertyAtIndex(i);
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
        NS_ABORT_IF_FALSE(!HasPropertyBit(iProp),
                          "compressed block has property multiple times");
        SetPropertyBit(iProp);
        if (aImportant)
            SetImportantBit(iProp);

        const nsCSSValue* val = aBlock->ValueAtIndex(i);
        nsCSSValue* dest = PropertyAt(iProp);
        NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null, "oops");
        NS_ABORT_IF_FALSE(dest->GetUnit() == eCSSUnit_Null,
                          "expanding into non-empty block");
#ifdef NS_BUILD_REFCNT_LOGGING
        dest->~nsCSSValue();
#endif
        memcpy(dest, val, sizeof(nsCSSValue));
    }

    
    
    aBlock->SetNumPropsToZero();
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

void
nsCSSExpandedDataBlock::ComputeNumProps(uint32_t* aNumPropsNormal,
                                        uint32_t* aNumPropsImportant)
{
    *aNumPropsNormal = *aNumPropsImportant = 0;
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
                (*aNumPropsImportant)++;
            else
                (*aNumPropsNormal)++;
        }
    }
}

void
nsCSSExpandedDataBlock::Compress(nsCSSCompressedDataBlock **aNormalBlock,
                                 nsCSSCompressedDataBlock **aImportantBlock,
                                 const nsTArray<uint32_t>& aOrder)
{
    nsAutoPtr<nsCSSCompressedDataBlock> result_normal, result_important;
    uint32_t i_normal = 0, i_important = 0;

    uint32_t numPropsNormal, numPropsImportant;
    ComputeNumProps(&numPropsNormal, &numPropsImportant);

    result_normal =
        new(numPropsNormal) nsCSSCompressedDataBlock(numPropsNormal);

    if (numPropsImportant != 0) {
        result_important =
            new(numPropsImportant) nsCSSCompressedDataBlock(numPropsImportant);
    } else {
        result_important = nullptr;
    }

    




    for (size_t i = 0; i < aOrder.Length(); i++) {
        nsCSSProperty iProp = static_cast<nsCSSProperty>(aOrder[i]);
        if (iProp >= eCSSProperty_COUNT) {
            
            continue;
        }
        NS_ABORT_IF_FALSE(mPropertiesSet.HasProperty(iProp),
                          "aOrder identifies a property not in the expanded "
                          "data block");
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(iProp), "out of range");
        bool important = mPropertiesImportant.HasProperty(iProp);
        nsCSSCompressedDataBlock *result =
            important ? result_important : result_normal;
        uint32_t* ip = important ? &i_important : &i_normal;
        nsCSSValue* val = PropertyAt(iProp);
        NS_ABORT_IF_FALSE(val->GetUnit() != eCSSUnit_Null,
                          "Null value while compressing");
        result->SetPropertyAtIndex(*ip, iProp);
        result->RawCopyValueToIndex(*ip, val);
        new (val) nsCSSValue();
        (*ip)++;
        result->mStyleBits |=
            nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[iProp]);
    }

    NS_ABORT_IF_FALSE(numPropsNormal == i_normal, "bad numProps");

    if (result_important) {
        NS_ABORT_IF_FALSE(numPropsImportant == i_important, "bad numProps");
    }

#ifdef DEBUG
    {
      
      
      uint32_t numPropsInSet = 0;
      for (size_t iHigh = 0; iHigh < nsCSSPropertySet::kChunkCount; iHigh++) {
          if (!mPropertiesSet.HasPropertyInChunk(iHigh)) {
              continue;
          }
          for (size_t iLow = 0; iLow < nsCSSPropertySet::kBitsInChunk; iLow++) {
              if (mPropertiesSet.HasPropertyAt(iHigh, iLow)) {
                  numPropsInSet++;
              }
          }
      }
      NS_ABORT_IF_FALSE(numPropsNormal + numPropsImportant == numPropsInSet,
                        "aOrder missing properties from the expanded data "
                        "block");
    }
#endif

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

void
nsCSSExpandedDataBlock::MapRuleInfoInto(nsCSSProperty aPropID,
                                        nsRuleData* aRuleData) const
{
  MOZ_ASSERT(!nsCSSProps::IsShorthand(aPropID));

  const nsCSSValue* src = PropertyAt(aPropID);
  MOZ_ASSERT(src->GetUnit() != eCSSUnit_Null);

  nsCSSProperty physicalProp = aPropID;
  if (nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_LOGICAL)) {
    EnsurePhysicalProperty(physicalProp, aRuleData);
    aRuleData->mCanStoreInRuleTree = false;
  }

  nsCSSValue* dest = aRuleData->ValueFor(physicalProp);
  MOZ_ASSERT(dest->GetUnit() == eCSSUnit_TokenStream &&
             dest->GetTokenStreamValue()->mPropertyID == aPropID);

  MapSinglePropertyInto(physicalProp, src, dest, aRuleData);
}

#ifdef DEBUG
void
nsCSSExpandedDataBlock::DoAssertInitialState()
{
    mPropertiesSet.AssertIsEmpty("not initial state");
    mPropertiesImportant.AssertIsEmpty("not initial state");

    for (uint32_t i = 0; i < eCSSProperty_COUNT_no_shorthands; ++i) {
        nsCSSProperty prop = nsCSSProperty(i);
        NS_ABORT_IF_FALSE(PropertyAt(prop)->GetUnit() == eCSSUnit_Null,
                          "not initial state");
    }
}
#endif
