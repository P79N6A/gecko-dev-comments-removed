









































#include "nsCSSDataBlock.h"
#include "nsCSSProps.h"
#include "nsRuleData.h"
#include "nsRuleNode.h"
#include "nsStyleSet.h"
#include "nsStyleContext.h"


















struct CDBValueStorage {
    nsCSSProperty property;
    nsCSSValue value;
};

struct CDBRectStorage {
    nsCSSProperty property;
    nsCSSRect value;

};

struct CDBValuePairStorage {
    nsCSSProperty property;
    nsCSSValuePair value;
};

struct CDBPointerStorage {
    nsCSSProperty property;
    void *value;
};

enum {
    CDBValueStorage_advance = sizeof(CDBValueStorage),
    CDBRectStorage_advance = sizeof(CDBRectStorage),
    CDBValuePairStorage_advance = sizeof(CDBValuePairStorage),
    
    
    CDBPointerStorage_advance = PR_ROUNDUP(sizeof(CDBPointerStorage),
                                sizeof(CDBValueStorage) - sizeof(nsCSSValue))
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

inline nsCSSRect* RectAtCursor(char *aCursor) {
    return & reinterpret_cast<CDBRectStorage*>(aCursor)->value;
}

inline const nsCSSRect* RectAtCursor(const char *aCursor) {
    return & reinterpret_cast<const CDBRectStorage*>(aCursor)->value;
}

inline nsCSSValuePair* ValuePairAtCursor(char *aCursor) {
  return & reinterpret_cast<CDBValuePairStorage*>(aCursor)->value;
}

inline const nsCSSValuePair* ValuePairAtCursor(const char *aCursor) {
  return & reinterpret_cast<const CDBValuePairStorage*>(aCursor)->value;
}

inline void*& PointerAtCursor(char *aCursor) {
    return reinterpret_cast<CDBPointerStorage*>(aCursor)->value;
}

inline void* PointerAtCursor(const char *aCursor) {
    return reinterpret_cast<const CDBPointerStorage*>(aCursor)->value;
}

inline nsCSSValueList*& ValueListAtCursor(char *aCursor) {
    return * reinterpret_cast<nsCSSValueList**>
                             (& reinterpret_cast<CDBPointerStorage*>(aCursor)->value);
}

inline nsCSSValueList* ValueListAtCursor(const char *aCursor) {
    return static_cast<nsCSSValueList*>
                      (reinterpret_cast<const CDBPointerStorage*>(aCursor)->value);
}

inline nsCSSValuePairList*& ValuePairListAtCursor(char *aCursor) {
    return * reinterpret_cast<nsCSSValuePairList**>
                             (& reinterpret_cast<CDBPointerStorage*>(aCursor)->value);
}

inline nsCSSValuePairList* ValuePairListAtCursor(const char *aCursor) {
    return static_cast<nsCSSValuePairList*>
                      (reinterpret_cast<const CDBPointerStorage*>(aCursor)->value);
}

static PRBool
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
  if (nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_IMAGE_IS_IN_ARRAY_0)) {
    if (aValue.GetUnit() == eCSSUnit_Array) {
      TryToStartImageLoadOnValue(aValue.GetArrayValue()->Item(0), aDocument);
    }
  } else {
    TryToStartImageLoadOnValue(aValue, aDocument);
  }
}

static inline PRBool
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
        NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                     "out of range");
        if (nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[iProp]) &
            aRuleData->mSIDs) {
            void *prop =
                nsCSSExpandedDataBlock::RuleDataPropertyAt(aRuleData, iProp);
            switch (nsCSSProps::kTypeTable[iProp]) {
                case eCSSType_Value: {
                    nsCSSValue* target = static_cast<nsCSSValue*>(prop);
                    if (target->GetUnit() == eCSSUnit_Null) {
                        const nsCSSValue *val = ValueAtCursor(cursor);
                        NS_ASSERTION(val->GetUnit() != eCSSUnit_Null, "oops");
                        if (ShouldStartImageLoads(aRuleData, iProp)) {
                            TryToStartImageLoad(*val, doc, iProp);
                        }
                        *target = *val;
                        if (iProp == eCSSProperty_font_family) {
                            
                            aRuleData->mFontData->mFamilyFromHTML = PR_FALSE;
                        }
                        if (nsCSSProps::PropHasFlags(iProp,
                                CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED) &&
                            ShouldIgnoreColors(aRuleData))
                        {
                            if (iProp == eCSSProperty_background_color) {
                                
                                
                                if (target->IsNonTransparentColor()) {
                                    target->SetColorValue(aRuleData->
                                        mPresContext->
                                        DefaultBackgroundColor());
                                }
                            } else {
                                
                                *target = nsCSSValue();
                            }
                        }
                    }
                    cursor += CDBValueStorage_advance;
                } break;

                case eCSSType_Rect: {
                    NS_ABORT_IF_FALSE(!nsCSSProps::PropHasFlags(iProp,
                        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED),
                        "this case needs to handle color properties");
                    const nsCSSRect* val = RectAtCursor(cursor);
                    NS_ASSERTION(val->HasValue(), "oops");
                    nsCSSRect* target = static_cast<nsCSSRect*>(prop);
                    if (target->mTop.GetUnit() == eCSSUnit_Null)
                        target->mTop = val->mTop;
                    if (target->mRight.GetUnit() == eCSSUnit_Null)
                        target->mRight = val->mRight;
                    if (target->mBottom.GetUnit() == eCSSUnit_Null)
                        target->mBottom = val->mBottom;
                    if (target->mLeft.GetUnit() == eCSSUnit_Null)
                        target->mLeft = val->mLeft;
                    cursor += CDBRectStorage_advance;
                } break;

                case eCSSType_ValuePair: {
                    NS_ABORT_IF_FALSE(!nsCSSProps::PropHasFlags(iProp,
                        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED),
                        "this case needs to handle color properties");
                    const nsCSSValuePair* val = ValuePairAtCursor(cursor);
                    NS_ASSERTION(val->mXValue.GetUnit() != eCSSUnit_Null ||
                                 val->mYValue.GetUnit() != eCSSUnit_Null, "oops");
                    nsCSSValuePair* target = static_cast<nsCSSValuePair*>(prop);
                    NS_ASSERTION((target->mXValue.GetUnit() == eCSSUnit_Null)
                              == (target->mYValue.GetUnit() == eCSSUnit_Null),
                                 "half a property?");
                    if (target->mXValue.GetUnit() == eCSSUnit_Null) {
                        target->mXValue = val->mXValue;
                        target->mYValue = val->mYValue;
                    }
                    cursor += CDBValuePairStorage_advance;
                } break;

                case eCSSType_ValueList:
                case eCSSType_ValuePairList: {
                    void** target = static_cast<void**>(prop);
                    if (!*target) {
                        if (ShouldStartImageLoads(aRuleData, iProp)) {
                            for (nsCSSValueList* l = ValueListAtCursor(cursor);
                                 l; l = l->mNext) {
                                TryToStartImageLoad(l->mValue, doc, iProp);
                            }
                        }

                        void* val = PointerAtCursor(cursor);
                        NS_ASSERTION(val, "oops");
                        *target = val;

                        if (nsCSSProps::PropHasFlags(iProp,
                                CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED) &&
                            ShouldIgnoreColors(aRuleData))
                        {
                            *target = nsnull;
                        }
                    }
                    cursor += CDBPointerStorage_advance;
                } break;
            }
        } else {
            switch (nsCSSProps::kTypeTable[iProp]) {
                case eCSSType_Value: {
                    cursor += CDBValueStorage_advance;
                } break;

                case eCSSType_Rect: {
                    cursor += CDBRectStorage_advance;
                } break;

                case eCSSType_ValuePair: {
                    cursor += CDBValuePairStorage_advance;
                } break;

                case eCSSType_ValueList:
                case eCSSType_ValuePairList: {
                    cursor += CDBPointerStorage_advance;
                } break;
            }
        }
    }
    NS_ASSERTION(cursor == cursor_end, "inconsistent data");
}

const void*
nsCSSCompressedDataBlock::StorageFor(nsCSSProperty aProperty) const
{
    NS_PRECONDITION(!nsCSSProps::IsShorthand(aProperty),
                    "Don't call for shorthands");

    
    
    
    
    if (!(nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[aProperty]) &
          mStyleBits))
        return nsnull;

    const char* cursor = Block();
    const char* cursor_end = BlockEnd();
    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                     "out of range");
        if (iProp == aProperty) {
            switch (nsCSSProps::kTypeTable[iProp]) {
                case eCSSType_Value: {
                    return ValueAtCursor(cursor);
                }
                case eCSSType_Rect: {
                    return RectAtCursor(cursor);
                }
                case eCSSType_ValuePair: {
                    return ValuePairAtCursor(cursor);
                }
                case eCSSType_ValueList:
                case eCSSType_ValuePairList: {
                    return &PointerAtCursor(const_cast<char*>(cursor));
                }
            }
        }
        switch (nsCSSProps::kTypeTable[iProp]) {
            case eCSSType_Value: {
                cursor += CDBValueStorage_advance;
            } break;

            case eCSSType_Rect: {
                cursor += CDBRectStorage_advance;
            } break;

            case eCSSType_ValuePair: {
                cursor += CDBValuePairStorage_advance;
            } break;

            case eCSSType_ValueList:
            case eCSSType_ValuePairList: {
                cursor += CDBPointerStorage_advance;
            } break;
        }
    }
    NS_ASSERTION(cursor == cursor_end, "inconsistent data");

    return nsnull;
}

already_AddRefed<nsCSSCompressedDataBlock>
nsCSSCompressedDataBlock::Clone() const
{
    const char *cursor = Block(), *cursor_end = BlockEnd();
    char *result_cursor;

    nsCSSCompressedDataBlock *result =
        new(cursor_end - cursor) nsCSSCompressedDataBlock();
    if (!result)
        return nsnull;
    result_cursor = result->Block();

    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                     "out of range");
        PropertyAtCursor(result_cursor) = iProp;

        switch (nsCSSProps::kTypeTable[iProp]) {
            case eCSSType_Value: {
                const nsCSSValue* val = ValueAtCursor(cursor);
                NS_ASSERTION(val->GetUnit() != eCSSUnit_Null, "oops");
                nsCSSValue *result_val = ValueAtCursor(result_cursor);
                new (result_val) nsCSSValue(*val);
                cursor += CDBValueStorage_advance;
                result_cursor +=  CDBValueStorage_advance;
            } break;

            case eCSSType_Rect: {
                const nsCSSRect* val = RectAtCursor(cursor);
                NS_ASSERTION(val->HasValue(), "oops");
                nsCSSRect* result_val = RectAtCursor(result_cursor);
                new (result_val) nsCSSRect(*val);
                cursor += CDBRectStorage_advance;
                result_cursor += CDBRectStorage_advance;
            } break;

            case eCSSType_ValuePair: {
                const nsCSSValuePair* val = ValuePairAtCursor(cursor);
                NS_ASSERTION(val->mXValue.GetUnit() != eCSSUnit_Null ||
                             val->mYValue.GetUnit() != eCSSUnit_Null, "oops");
                nsCSSValuePair* result_val = ValuePairAtCursor(result_cursor);
                new (result_val) nsCSSValuePair(*val);
                cursor += CDBValuePairStorage_advance;
                result_cursor += CDBValuePairStorage_advance;
            } break;

            case eCSSType_ValueList:
            case eCSSType_ValuePairList: {
                void *copy;
                NS_ASSERTION(PointerAtCursor(cursor), "oops");
                switch (nsCSSProps::kTypeTable[iProp]) {
                    default:
                        NS_NOTREACHED("unreachable");
                        
                        
                    case eCSSType_ValueList:
                        copy = ValueListAtCursor(cursor)->Clone();
                        break;
                    case eCSSType_ValuePairList:
                        copy = ValuePairListAtCursor(cursor)->Clone();
                        break;
                }
                if (!copy) {
                    result->mBlockEnd = result_cursor;
                    result->Destroy();
                    return nsnull;
                }
                PointerAtCursor(result_cursor) = copy;
                cursor += CDBPointerStorage_advance;
                result_cursor += CDBPointerStorage_advance;
            } break;
        }
    }
    NS_ASSERTION(cursor == cursor_end, "inconsistent data");

    result->mBlockEnd = result_cursor;
    result->mStyleBits = mStyleBits;
    NS_ASSERTION(result->DataSize() == DataSize(), "wrong size");

    result->AddRef();
    return result;
}

void
nsCSSCompressedDataBlock::Destroy()
{
    const char* cursor = Block();
    const char* cursor_end = BlockEnd();
    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                     "out of range");

        switch (nsCSSProps::kTypeTable[iProp]) {
            case eCSSType_Value: {
                const nsCSSValue* val = ValueAtCursor(cursor);
                NS_ASSERTION(val->GetUnit() != eCSSUnit_Null, "oops");
                val->~nsCSSValue();
                cursor += CDBValueStorage_advance;
            } break;

            case eCSSType_Rect: {
                const nsCSSRect* val = RectAtCursor(cursor);
                NS_ASSERTION(val->HasValue(), "oops");
                val->~nsCSSRect();
                cursor += CDBRectStorage_advance;
            } break;

            case eCSSType_ValuePair: {
                const nsCSSValuePair* val = ValuePairAtCursor(cursor);
                NS_ASSERTION(val->mXValue.GetUnit() != eCSSUnit_Null ||
                             val->mYValue.GetUnit() != eCSSUnit_Null, "oops");
                val->~nsCSSValuePair();
                cursor += CDBValuePairStorage_advance;
            } break;

            case eCSSType_ValueList: {
                nsCSSValueList* val = ValueListAtCursor(cursor);
                NS_ASSERTION(val, "oops");
                delete val;
                cursor += CDBPointerStorage_advance;
            } break;

            case eCSSType_ValuePairList: {
                nsCSSValuePairList* val = ValuePairListAtCursor(cursor);
                NS_ASSERTION(val, "oops");
                delete val;
                cursor += CDBPointerStorage_advance;
            } break;
        }
    }
    NS_ASSERTION(cursor == cursor_end, "inconsistent data");
    delete this;
}

 already_AddRefed<nsCSSCompressedDataBlock>
nsCSSCompressedDataBlock::CreateEmptyBlock()
{
    nsCSSCompressedDataBlock *result = new(0) nsCSSCompressedDataBlock();
    if (!result)
        return nsnull;
    result->mBlockEnd = result->Block();

    result->AddRef();
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

const nsCSSExpandedDataBlock::PropertyOffsetInfo
nsCSSExpandedDataBlock::kOffsetTable[eCSSProperty_COUNT_no_shorthands] = {
    #define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, datastruct_,     \
                                 member_, type_, kwtable_)                     \
        { offsetof(nsCSSExpandedDataBlock, m##datastruct_.member_),            \
          size_t(-1),                                                          \
          size_t(-1) },
    #define CSS_PROP(name_, id_, method_, flags_, datastruct_, member_, type_, \
                     kwtable_, stylestruct_, stylestructoffset_, animtype_)    \
        { offsetof(nsCSSExpandedDataBlock, m##datastruct_.member_),            \
          offsetof(nsRuleData, m##datastruct_##Data),                          \
          offsetof(nsRuleData##datastruct_, member_) },
    #include "nsCSSPropList.h"
    #undef CSS_PROP
    #undef CSS_PROP_BACKENDONLY
};

void
nsCSSExpandedDataBlock::DoExpand(nsRefPtr<nsCSSCompressedDataBlock> *aBlock,
                                 PRBool aImportant)
{
    NS_PRECONDITION(*aBlock, "unexpected null block");

    if (!(*aBlock)->IsMutable()) {
        
        
        *aBlock = (*aBlock)->Clone();
        if (!*aBlock) {
            
            NS_WARNING("out of memory");
            return;
        }
        NS_ABORT_IF_FALSE((*aBlock)->IsMutable(), "we just cloned it");
    }

    





    const char* cursor = (*aBlock)->Block();
    const char* cursor_end = (*aBlock)->BlockEnd();
    while (cursor < cursor_end) {
        nsCSSProperty iProp = PropertyAtCursor(cursor);
        NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                     "out of range");
        NS_ASSERTION(!HasPropertyBit(iProp),
                     "compressed block has property multiple times");
        SetPropertyBit(iProp);
        if (aImportant)
            SetImportantBit(iProp);
        void *prop = PropertyAt(iProp);

        switch (nsCSSProps::kTypeTable[iProp]) {
            case eCSSType_Value: {
                const nsCSSValue* val = ValueAtCursor(cursor);
                nsCSSValue* dest = static_cast<nsCSSValue*>(prop);
                NS_ASSERTION(val->GetUnit() != eCSSUnit_Null, "oops");
                NS_ASSERTION(dest->GetUnit() == eCSSUnit_Null,
                             "expanding into non-empty block");
#ifdef NS_BUILD_REFCNT_LOGGING
                dest->~nsCSSValue();
#endif
                memcpy(dest, val, sizeof(nsCSSValue));
                cursor += CDBValueStorage_advance;
            } break;

            case eCSSType_Rect: {
                const nsCSSRect* val = RectAtCursor(cursor);
                nsCSSRect* dest = static_cast<nsCSSRect*>(prop);
                NS_ASSERTION(val->HasValue(), "oops");
                NS_ASSERTION(!dest->HasValue(),
                             "expanding into non-empty block");
#ifdef NS_BUILD_REFCNT_LOGGING
                dest->~nsCSSRect();
#endif
                memcpy(dest, val, sizeof(nsCSSRect));
                cursor += CDBRectStorage_advance;
            } break;

            case eCSSType_ValuePair: {
                const nsCSSValuePair* val = ValuePairAtCursor(cursor);
                nsCSSValuePair* dest = static_cast<nsCSSValuePair*>(prop);
                NS_ASSERTION(val->mXValue.GetUnit() != eCSSUnit_Null ||
                             val->mYValue.GetUnit() != eCSSUnit_Null, "oops");
                NS_ASSERTION(dest->mXValue.GetUnit() == eCSSUnit_Null &&
                             dest->mYValue.GetUnit() == eCSSUnit_Null,
                             "expanding into non-empty block");
#ifdef NS_BUILD_REFCNT_LOGGING
                dest->~nsCSSValuePair();
#endif
                memcpy(dest, val, sizeof(nsCSSValuePair));
                cursor += CDBValuePairStorage_advance;
            } break;

            case eCSSType_ValueList:
            case eCSSType_ValuePairList: {
                void* val = PointerAtCursor(cursor);
                void** dest = static_cast<void**>(prop);
                NS_ASSERTION(val, "oops");
                NS_ASSERTION(!*dest, "expanding into non-empty block");
                *dest = val;
                cursor += CDBPointerStorage_advance;
            } break;
        }
    }
    NS_ASSERTION(cursor == cursor_end, "inconsistent data");

    NS_ASSERTION((*aBlock)->mRefCnt == 1, "unexpected reference count");
    delete aBlock->forget().get();
}

void
nsCSSExpandedDataBlock::Expand(
                          nsRefPtr<nsCSSCompressedDataBlock> *aNormalBlock,
                          nsRefPtr<nsCSSCompressedDataBlock> *aImportantBlock)
{
    NS_PRECONDITION(*aNormalBlock, "unexpected null block");
    AssertInitialState();

    DoExpand(aNormalBlock, PR_FALSE);
    if (*aImportantBlock) {
        DoExpand(aImportantBlock, PR_TRUE);
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
            nsCSSProperty iProp = nsCSSPropertySet::CSSPropertyAt(iHigh, iLow);
            NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                         "out of range");
#ifdef DEBUG
            void *prop = PropertyAt(iProp);
#endif
            PRUint32 increment = 0;
            switch (nsCSSProps::kTypeTable[iProp]) {
                case eCSSType_Value: {
#ifdef DEBUG
                    nsCSSValue* val = static_cast<nsCSSValue*>(prop);
                    NS_ASSERTION(val->GetUnit() != eCSSUnit_Null,
                                 "null value while computing size");
#endif
                    increment = CDBValueStorage_advance;
                } break;

                case eCSSType_Rect: {
#ifdef DEBUG
                    nsCSSRect* val = static_cast<nsCSSRect*>(prop);
                    NS_ASSERTION(val->HasValue(),
                                 "Valueless rect while computing size");
#endif
                    increment = CDBRectStorage_advance;
                } break;

                case eCSSType_ValuePair: {
#ifdef DEBUG
                    nsCSSValuePair* val = static_cast<nsCSSValuePair*>(prop);
                    NS_ASSERTION(val->mXValue.GetUnit() != eCSSUnit_Null ||
                                 val->mYValue.GetUnit() != eCSSUnit_Null,
                                 "Valueless pair while computing size");
#endif
                    increment = CDBValuePairStorage_advance;
                } break;

                case eCSSType_ValueList:
                case eCSSType_ValuePairList: {
#ifdef DEBUG
                    void* val = *static_cast<void**>(prop);
                    NS_ASSERTION(val, "Null pointer while computing size");
#endif
                    increment = CDBPointerStorage_advance;
                } break;
            }
            if (mPropertiesImportant.HasPropertyAt(iHigh, iLow))
                result.important += increment;
            else
                result.normal += increment;
        }
    }
    return result;
}

void
nsCSSExpandedDataBlock::Compress(nsCSSCompressedDataBlock **aNormalBlock,
                                 nsCSSCompressedDataBlock **aImportantBlock)
{
    nsRefPtr<nsCSSCompressedDataBlock> result_normal, result_important;
    char *cursor_normal, *cursor_important;

    ComputeSizeResult size = ComputeSize();
    
    result_normal = new(size.normal) nsCSSCompressedDataBlock();
    if (!result_normal) {
        *aNormalBlock = nsnull;
        *aImportantBlock = nsnull;
        return;
    }
    cursor_normal = result_normal->Block();

    if (size.important != 0) {
        result_important = new(size.important) nsCSSCompressedDataBlock();
        if (!result_important) {
            *aNormalBlock = nsnull;
            *aImportantBlock = nsnull;
            return;
        }
        cursor_important = result_important->Block();
    } else {
        result_important = nsnull;
    }

    




    for (size_t iHigh = 0; iHigh < nsCSSPropertySet::kChunkCount; ++iHigh) {
        if (!mPropertiesSet.HasPropertyInChunk(iHigh))
            continue;
        for (size_t iLow = 0; iLow < nsCSSPropertySet::kBitsInChunk; ++iLow) {
            if (!mPropertiesSet.HasPropertyAt(iHigh, iLow))
                continue;
            nsCSSProperty iProp = nsCSSPropertySet::CSSPropertyAt(iHigh, iLow);
            NS_ASSERTION(0 <= iProp && iProp < eCSSProperty_COUNT_no_shorthands,
                         "out of range");
            void *prop = PropertyAt(iProp);
            PRBool important =
                mPropertiesImportant.HasPropertyAt(iHigh, iLow);
            char *&cursor = important ? cursor_important : cursor_normal;
            nsCSSCompressedDataBlock *result =
                important ? result_important : result_normal;
            switch (nsCSSProps::kTypeTable[iProp]) {
                case eCSSType_Value: {
                    nsCSSValue* val = static_cast<nsCSSValue*>(prop);
                    NS_ASSERTION(val->GetUnit() != eCSSUnit_Null,
                                 "Null value while compressing");
                    CDBValueStorage *storage =
                        reinterpret_cast<CDBValueStorage*>(cursor);
                    storage->property = iProp;
                    memcpy(&storage->value, val, sizeof(nsCSSValue));
                    new (val) nsCSSValue();
                    cursor += CDBValueStorage_advance;
                } break;

                case eCSSType_Rect: {
                    nsCSSRect* val = static_cast<nsCSSRect*>(prop);
                    NS_ASSERTION(val->HasValue(),
                                 "Valueless rect while compressing");
                    CDBRectStorage *storage =
                        reinterpret_cast<CDBRectStorage*>(cursor);
                    storage->property = iProp;
                    memcpy(&storage->value, val, sizeof(nsCSSRect));
                    new (val) nsCSSRect();
                    cursor += CDBRectStorage_advance;
                } break;

                case eCSSType_ValuePair: {
                    nsCSSValuePair* val = static_cast<nsCSSValuePair*>(prop);
                    NS_ASSERTION(val->mXValue.GetUnit() != eCSSUnit_Null ||
                                 val->mYValue.GetUnit() != eCSSUnit_Null,
                                 "Valueless pair while compressing");
                    CDBValuePairStorage *storage =
                        reinterpret_cast<CDBValuePairStorage*>(cursor);
                    storage->property = iProp;
                    memcpy(&storage->value, val, sizeof(nsCSSValuePair));
                    new (val) nsCSSValuePair();
                    cursor += CDBValuePairStorage_advance;
                } break;

                case eCSSType_ValueList:
                case eCSSType_ValuePairList: {
                    void*& val = *static_cast<void**>(prop);
                    NS_ASSERTION(val, "Null pointer while compressing");
                    CDBPointerStorage *storage =
                        reinterpret_cast<CDBPointerStorage*>(cursor);
                    storage->property = iProp;
                    storage->value = val;
                    val = nsnull;
                    cursor += CDBPointerStorage_advance;
                } break;
            }
            result->mStyleBits |=
                nsCachedStyleData::GetBitForSID(nsCSSProps::kSIDTable[iProp]);
        }
    }

    result_normal->mBlockEnd = cursor_normal;
    NS_ASSERTION(result_normal->DataSize() == ptrdiff_t(size.normal),
                 "size miscalculation");
    if (result_important) {
        result_important->mBlockEnd = cursor_important;
        NS_ASSERTION(result_important->DataSize() == ptrdiff_t(size.important),
                     "size miscalculation");
    }

    ClearSets();
    AssertInitialState();
    result_normal.forget(aNormalBlock);
    result_important.forget(aImportantBlock);
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
            ClearProperty(iProp);
        }
    }

    AssertInitialState();
}

void
nsCSSExpandedDataBlock::ClearProperty(nsCSSProperty aPropID)
{
    NS_ASSERTION(0 <= aPropID && aPropID < eCSSProperty_COUNT_no_shorthands,
                 "out of range");

    ClearPropertyBit(aPropID);
    ClearImportantBit(aPropID);

    void *prop = PropertyAt(aPropID);
    switch (nsCSSProps::kTypeTable[aPropID]) {
        case eCSSType_Value: {
            nsCSSValue* val = static_cast<nsCSSValue*>(prop);
            val->Reset();
        } break;

        case eCSSType_Rect: {
            nsCSSRect* val = static_cast<nsCSSRect*>(prop);
            val->Reset();
        } break;

        case eCSSType_ValuePair: {
            nsCSSValuePair* val = static_cast<nsCSSValuePair*>(prop);
            val->mXValue.Reset();
            val->mYValue.Reset();
        } break;

        case eCSSType_ValueList: {
            nsCSSValueList*& val = *static_cast<nsCSSValueList**>(prop);
            if (val) {
                delete val;
                val = nsnull;
            }
        } break;

        case eCSSType_ValuePairList: {
            nsCSSValuePairList*& val =
              *static_cast<nsCSSValuePairList**>(prop);
            if (val) {
                delete val;
                val = nsnull;
            }
        } break;
    }
}

#ifdef DEBUG
void
nsCSSExpandedDataBlock::DoAssertInitialState()
{
    mPropertiesSet.AssertIsEmpty("not initial state");
    mPropertiesImportant.AssertIsEmpty("not initial state");

    for (PRUint32 i = 0; i < eCSSProperty_COUNT_no_shorthands; ++i) {
        void *prop = PropertyAt(nsCSSProperty(i));
        switch (nsCSSProps::kTypeTable[i]) {
            case eCSSType_Value: {
                nsCSSValue* val = static_cast<nsCSSValue*>(prop);
                NS_ASSERTION(val->GetUnit() == eCSSUnit_Null,
                             "not initial state");
            } break;

            case eCSSType_Rect: {
                nsCSSRect* val = static_cast<nsCSSRect*>(prop);
                NS_ASSERTION(val->mTop.GetUnit() == eCSSUnit_Null,
                             "not initial state");
                NS_ASSERTION(val->mRight.GetUnit() == eCSSUnit_Null,
                             "not initial state");
                NS_ASSERTION(val->mBottom.GetUnit() == eCSSUnit_Null,
                             "not initial state");
                NS_ASSERTION(val->mLeft.GetUnit() == eCSSUnit_Null,
                             "not initial state");
            } break;

            case eCSSType_ValuePair: {
                nsCSSValuePair* val = static_cast<nsCSSValuePair*>(prop);
                NS_ASSERTION(val->mXValue.GetUnit() == eCSSUnit_Null,
                             "not initial state");
                NS_ASSERTION(val->mYValue.GetUnit() == eCSSUnit_Null,
                             "not initial state");
            } break;

            case eCSSType_ValueList: {
                nsCSSValueList* val = *static_cast<nsCSSValueList**>(prop);
                NS_ASSERTION(val == nsnull, "not initial state");
            } break;

            case eCSSType_ValuePairList: {
                nsCSSValuePairList* val =
                  *static_cast<nsCSSValuePairList**>(prop);
                NS_ASSERTION(val == nsnull, "not initial state");
            } break;
        }
    }
}
#endif
