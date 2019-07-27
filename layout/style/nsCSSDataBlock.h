









#ifndef nsCSSDataBlock_h__
#define nsCSSDataBlock_h__

#include "mozilla/MemoryReporting.h"
#include "nsCSSProps.h"
#include "nsCSSPropertySet.h"
#include "nsCSSValue.h"
#include "imgRequestProxy.h"

struct nsRuleData;
class nsCSSExpandedDataBlock;

namespace mozilla {
namespace css {
class Declaration;
}
}







class nsCSSCompressedDataBlock {
private:
    friend class nsCSSExpandedDataBlock;

    
    
    explicit nsCSSCompressedDataBlock(uint32_t aNumProps)
      : mStyleBits(0), mNumProps(aNumProps)
    {}

public:
    ~nsCSSCompressedDataBlock();

    



    void MapRuleInfoInto(nsRuleData *aRuleData) const;

    








    const nsCSSValue* ValueFor(nsCSSProperty aProperty) const;

    







    bool TryReplaceValue(nsCSSProperty aProperty,
                           nsCSSExpandedDataBlock& aFromBlock,
                           bool* aChanged);

    


    nsCSSCompressedDataBlock* Clone() const;

    


    static nsCSSCompressedDataBlock* CreateEmptyBlock();

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

    bool HasDefaultBorderImageSlice() const;
    bool HasDefaultBorderImageWidth() const;
    bool HasDefaultBorderImageOutset() const;
    bool HasDefaultBorderImageRepeat() const;

private:
    void* operator new(size_t aBaseSize, uint32_t aNumProps) {
        NS_ABORT_IF_FALSE(aBaseSize == sizeof(nsCSSCompressedDataBlock),
                          "unexpected size for nsCSSCompressedDataBlock");
        return ::operator new(aBaseSize + DataSize(aNumProps));
    }

public:
    
    
    
    
    typedef int16_t CompressedCSSProperty;
    static const size_t MaxCompressedCSSProperty = INT16_MAX;

private:
    static size_t DataSize(uint32_t aNumProps) {
        return size_t(aNumProps) *
               (sizeof(nsCSSValue) + sizeof(CompressedCSSProperty));
    }

    int32_t mStyleBits; 
                        
    uint32_t mNumProps;
    
    
    
    
    
    

    nsCSSValue* Values() const {
        return (nsCSSValue*)(this + 1);
    }

    CompressedCSSProperty* CompressedProperties() const {
        return (CompressedCSSProperty*)(Values() + mNumProps);
    }

    nsCSSValue* ValueAtIndex(uint32_t i) const {
        NS_ABORT_IF_FALSE(i < mNumProps, "value index out of range");
        return Values() + i;
    }

    nsCSSProperty PropertyAtIndex(uint32_t i) const {
        NS_ABORT_IF_FALSE(i < mNumProps, "property index out of range");
        nsCSSProperty prop = (nsCSSProperty)CompressedProperties()[i];
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(prop), "out of range");
        return prop;
    }

    void CopyValueToIndex(uint32_t i, nsCSSValue* aValue) {
        new (ValueAtIndex(i)) nsCSSValue(*aValue);
    }

    void RawCopyValueToIndex(uint32_t i, nsCSSValue* aValue) {
        memcpy(ValueAtIndex(i), aValue, sizeof(nsCSSValue));
    }

    void SetPropertyAtIndex(uint32_t i, nsCSSProperty aProperty) {
        NS_ABORT_IF_FALSE(i < mNumProps, "set property index out of range");
        CompressedProperties()[i] = (CompressedCSSProperty)aProperty;
    }

    void SetNumPropsToZero() {
        mNumProps = 0;
    }
};



static_assert(sizeof(nsCSSCompressedDataBlock) == 8,
              "nsCSSCompressedDataBlock's size has changed");
static_assert(NS_ALIGNMENT_OF(nsCSSValue) == 4 || NS_ALIGNMENT_OF(nsCSSValue) == 8,
              "nsCSSValue doesn't align with nsCSSCompressedDataBlock"); 
static_assert(NS_ALIGNMENT_OF(nsCSSCompressedDataBlock::CompressedCSSProperty) == 2,
              "CompressedCSSProperty doesn't align with nsCSSValue"); 


static_assert(eCSSProperty_COUNT_no_shorthands <=
              nsCSSCompressedDataBlock::MaxCompressedCSSProperty,
              "nsCSSProperty doesn't fit in StoredSizeOfCSSProperty");

class nsCSSExpandedDataBlock {
    friend class nsCSSCompressedDataBlock;

public:
    nsCSSExpandedDataBlock();
    ~nsCSSExpandedDataBlock();

private:
    


    nsCSSValue mValues[eCSSProperty_COUNT_no_shorthands];

public:
    








    void Expand(nsCSSCompressedDataBlock *aNormalBlock,
                nsCSSCompressedDataBlock *aImportantBlock);

    







    void Compress(nsCSSCompressedDataBlock **aNormalBlock,
                  nsCSSCompressedDataBlock **aImportantBlock);

    



    void AddLonghandProperty(nsCSSProperty aProperty, const nsCSSValue& aValue);

    


    void Clear();

    



    void ClearProperty(nsCSSProperty aPropID);

    


    void ClearLonghandProperty(nsCSSProperty aPropID);

    










    bool TransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                             nsCSSProperty aPropID,
                             bool aIsImportant,
                             bool aOverrideImportant,
                             bool aMustCallValueAppended,
                             mozilla::css::Declaration* aDeclaration);

    





    void MapRuleInfoInto(nsCSSProperty aPropID, nsRuleData* aRuleData) const;

    void AssertInitialState() {
#ifdef DEBUG
        DoAssertInitialState();
#endif
    }

private:
    



    void ComputeNumProps(uint32_t* aNumPropsNormal,
                         uint32_t* aNumPropsImportant);
    
    void DoExpand(nsCSSCompressedDataBlock *aBlock, bool aImportant);

    


    bool DoTransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                               nsCSSProperty aPropID,
                               bool aIsImportant,
                               bool aOverrideImportant,
                               bool aMustCallValueAppended,
                               mozilla::css::Declaration* aDeclaration);

#ifdef DEBUG
    void DoAssertInitialState();
#endif

    





    nsCSSPropertySet mPropertiesSet;
    


    nsCSSPropertySet mPropertiesImportant;

    



    nsCSSValue* PropertyAt(nsCSSProperty aProperty) {
        NS_ABORT_IF_FALSE(0 <= aProperty &&
                          aProperty < eCSSProperty_COUNT_no_shorthands,
                          "property out of range");
        return &mValues[aProperty];
    }
    const nsCSSValue* PropertyAt(nsCSSProperty aProperty) const {
        NS_ABORT_IF_FALSE(0 <= aProperty &&
                          aProperty < eCSSProperty_COUNT_no_shorthands,
                          "property out of range");
        return &mValues[aProperty];
    }

    void SetPropertyBit(nsCSSProperty aProperty) {
        mPropertiesSet.AddProperty(aProperty);
    }

    void ClearPropertyBit(nsCSSProperty aProperty) {
        mPropertiesSet.RemoveProperty(aProperty);
    }

    bool HasPropertyBit(nsCSSProperty aProperty) {
        return mPropertiesSet.HasProperty(aProperty);
    }

    void SetImportantBit(nsCSSProperty aProperty) {
        mPropertiesImportant.AddProperty(aProperty);
    }

    void ClearImportantBit(nsCSSProperty aProperty) {
        mPropertiesImportant.RemoveProperty(aProperty);
    }

    bool HasImportantBit(nsCSSProperty aProperty) {
        return mPropertiesImportant.HasProperty(aProperty);
    }

    void ClearSets() {
        mPropertiesSet.Empty();
        mPropertiesImportant.Empty();
    }
};

#endif 
