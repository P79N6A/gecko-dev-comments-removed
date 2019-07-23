








































#ifndef nsCSSDataBlock_h__
#define nsCSSDataBlock_h__

#include "nsCSSStruct.h"
#include "nsCSSProps.h"
#include "nsCSSPropertySet.h"

struct nsRuleData;

class nsCSSExpandedDataBlock;
class nsCSSDeclaration;







class nsCSSCompressedDataBlock {
public:
    friend class nsCSSExpandedDataBlock;
    friend class nsCSSDeclaration;

    



    nsresult MapRuleInfoInto(nsRuleData *aRuleData) const;

    









    const void* StorageFor(nsCSSProperty aProperty) const;

    



    const nsCSSValue* ValueStorageFor(nsCSSProperty aProperty) const {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      return static_cast<const nsCSSValue*>(StorageFor(aProperty));
    }
    const nsCSSRect* RectStorageFor(nsCSSProperty aProperty) const {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Rect,
                        "type mismatch");
      return static_cast<const nsCSSRect*>(StorageFor(aProperty));
    }
    const nsCSSValuePair* ValuePairStorageFor(nsCSSProperty aProperty) const {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] ==
                          eCSSType_ValuePair,
                        "type mismatch");
      return static_cast<const nsCSSValuePair*>(StorageFor(aProperty));
    }
    const nsCSSValueList*const*
    ValueListStorageFor(nsCSSProperty aProperty) const {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] ==
                          eCSSType_ValueList,
                        "type mismatch");
      return static_cast<const nsCSSValueList*const*>(StorageFor(aProperty));
    }
    const nsCSSValuePairList*const*
    ValuePairListStorageFor(nsCSSProperty aProperty) const {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] ==
                          eCSSType_ValuePairList,
                        "type mismatch");
      return static_cast<const nsCSSValuePairList*const*>(
               StorageFor(aProperty));
    }

    


    nsCSSCompressedDataBlock* Clone() const;

    


    void Destroy();

    


    static nsCSSCompressedDataBlock* CreateEmptyBlock();

private:
    PRInt32 mStyleBits; 
                        

    enum { block_chars = 4 }; 
                              

    void* operator new(size_t aBaseSize, size_t aDataSize) {
        
        return ::operator new(aBaseSize + aDataSize -
                              sizeof(char) * block_chars);
    }

    nsCSSCompressedDataBlock() : mStyleBits(0) {}

    
    
    ~nsCSSCompressedDataBlock() { }

    char* mBlockEnd; 
    char mBlock_[block_chars]; 

    char* Block() { return mBlock_; }
    char* BlockEnd() { return mBlockEnd; }
    const char* Block() const { return mBlock_; }
    const char* BlockEnd() const { return mBlockEnd; }
    ptrdiff_t DataSize() const { return BlockEnd() - Block(); }

    
    
    void* SlotForValue(nsCSSProperty aProperty) {
      return const_cast<void*>(StorageFor(aProperty));
    }
};

class nsCSSExpandedDataBlock {
public:
    nsCSSExpandedDataBlock();
    ~nsCSSExpandedDataBlock();
    




    nsCSSFont mFont;
    nsCSSDisplay mDisplay;
    nsCSSMargin mMargin;
    nsCSSList mList;
    nsCSSPosition mPosition;
    nsCSSTable mTable;
    nsCSSColor mColor;
    nsCSSContent mContent;
    nsCSSText mText;
    nsCSSUserInterface mUserInterface;
    nsCSSAural mAural;
    nsCSSPage mPage;
    nsCSSBreaks mBreaks;
    nsCSSXUL mXUL;
#ifdef MOZ_SVG
    nsCSSSVG mSVG;
#endif
    nsCSSColumn mColumn;

    









    void Expand(nsCSSCompressedDataBlock **aNormalBlock,
                nsCSSCompressedDataBlock **aImportantBlock);

    




    void Compress(nsCSSCompressedDataBlock **aNormalBlock,
                  nsCSSCompressedDataBlock **aImportantBlock);

    


    void Clear();

    



    void ClearProperty(nsCSSProperty aPropID);

    void AssertInitialState() {
#ifdef DEBUG
        DoAssertInitialState();
#endif
    }

private:
    



    struct ComputeSizeResult {
        PRUint32 normal, important;
    };
    ComputeSizeResult ComputeSize();

    void DoExpand(nsCSSCompressedDataBlock *aBlock, PRBool aImportant);

#ifdef DEBUG
    void DoAssertInitialState();
#endif

    struct PropertyOffsetInfo {
        
        
        size_t block_offset; 
        size_t ruledata_struct_offset; 
        size_t ruledata_member_offset; 
    };

    static const PropertyOffsetInfo kOffsetTable[];

    





    nsCSSPropertySet mPropertiesSet;
    


    nsCSSPropertySet mPropertiesImportant;

public:
    




    void* PropertyAt(nsCSSProperty aProperty) {
        const PropertyOffsetInfo& offsets =
            nsCSSExpandedDataBlock::kOffsetTable[aProperty];
        return reinterpret_cast<void*>(reinterpret_cast<char*>(this) +
                                          offsets.block_offset);
    }

    




    static void* RuleDataPropertyAt(nsRuleData *aRuleData,
                                    nsCSSProperty aProperty) {
        const PropertyOffsetInfo& offsets =
            nsCSSExpandedDataBlock::kOffsetTable[aProperty];
        NS_ASSERTION(offsets.ruledata_struct_offset != size_t(-1),
                     "property should not use CSS_PROP_BACKENDONLY");
        char* cssstruct = *reinterpret_cast<char**>
                                           (reinterpret_cast<char*>(aRuleData) +
                              offsets.ruledata_struct_offset);
        return reinterpret_cast<void*>
                               (cssstruct + offsets.ruledata_member_offset);
    }

    void SetPropertyBit(nsCSSProperty aProperty) {
        mPropertiesSet.AddProperty(aProperty);
    }

    void ClearPropertyBit(nsCSSProperty aProperty) {
        mPropertiesSet.RemoveProperty(aProperty);
    }

    PRBool HasPropertyBit(nsCSSProperty aProperty) {
        return mPropertiesSet.HasProperty(aProperty);
    }

    void SetImportantBit(nsCSSProperty aProperty) {
        mPropertiesImportant.AddProperty(aProperty);
    }

    void ClearImportantBit(nsCSSProperty aProperty) {
        mPropertiesImportant.RemoveProperty(aProperty);
    }

    PRBool HasImportantBit(nsCSSProperty aProperty) {
        return mPropertiesImportant.HasProperty(aProperty);
    }

    void ClearSets() {
        mPropertiesSet.Empty();
        mPropertiesImportant.Empty();
    }
};

#endif 
