








































#ifndef nsCSSDataBlock_h__
#define nsCSSDataBlock_h__

#include "nsCSSStruct.h"

struct nsRuleData;

class nsCSSExpandedDataBlock;







class nsCSSCompressedDataBlock {
public:
    friend class nsCSSExpandedDataBlock;

    



    nsresult MapRuleInfoInto(nsRuleData *aRuleData) const;

    







    const void* StorageFor(nsCSSProperty aProperty) const;

    


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

    typedef PRUint8 property_set_type;
    enum { kPropertiesSetChunkSize = 8 }; 
                                          
    
    enum { kPropertiesSetChunkCount =
             (eCSSProperty_COUNT_no_shorthands + (kPropertiesSetChunkSize-1)) /
             kPropertiesSetChunkSize };
    





    property_set_type mPropertiesSet[kPropertiesSetChunkCount];
    


    property_set_type mPropertiesImportant[kPropertiesSetChunkCount];

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

    void AssertInSetRange(nsCSSProperty aProperty) {
        NS_ASSERTION(0 <= aProperty &&
                     aProperty < eCSSProperty_COUNT_no_shorthands,
                     "out of bounds");
    }

    void SetPropertyBit(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        mPropertiesSet[aProperty / kPropertiesSetChunkSize] |=
            property_set_type(1 << (aProperty % kPropertiesSetChunkSize));
    }

    void ClearPropertyBit(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        mPropertiesSet[aProperty / kPropertiesSetChunkSize] &=
            ~property_set_type(1 << (aProperty % kPropertiesSetChunkSize));
    }

    PRBool HasPropertyBit(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        return (mPropertiesSet[aProperty / kPropertiesSetChunkSize] &
                (1 << (aProperty % kPropertiesSetChunkSize))) != 0;
    }

    void SetImportantBit(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        mPropertiesImportant[aProperty / kPropertiesSetChunkSize] |=
            property_set_type(1 << (aProperty % kPropertiesSetChunkSize));
    }

    void ClearImportantBit(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        mPropertiesImportant[aProperty / kPropertiesSetChunkSize] &=
            ~property_set_type(1 << (aProperty % kPropertiesSetChunkSize));
    }

    PRBool HasImportantBit(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        return (mPropertiesImportant[aProperty / kPropertiesSetChunkSize] &
                (1 << (aProperty % kPropertiesSetChunkSize))) != 0;
    }

    void ClearSets() {
        memset(mPropertiesSet, 0, sizeof(mPropertiesSet));
        memset(mPropertiesImportant, 0, sizeof(mPropertiesImportant));
    }
};

#endif 
