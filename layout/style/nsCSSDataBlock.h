









































#ifndef nsCSSDataBlock_h__
#define nsCSSDataBlock_h__

#include "nsCSSStruct.h"
#include "nsCSSProps.h"
#include "nsCSSPropertySet.h"

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

    
    
    nsCSSCompressedDataBlock() : mStyleBits(0) {}

public:
    ~nsCSSCompressedDataBlock();

    



    void MapRuleInfoInto(nsRuleData *aRuleData) const;

    









    const void* StorageFor(nsCSSProperty aProperty) const;

    


    void* SlotForValue(nsCSSProperty aProperty) {
      return const_cast<void*>(StorageFor(aProperty));
    }

    



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

    


    static nsCSSCompressedDataBlock* CreateEmptyBlock();

    









    static void MoveValue(void *aSource, void *aDest, nsCSSProperty aPropID,
                          PRBool* aChanged);

private:
    PRInt32 mStyleBits; 
                        

    enum { block_chars = 4 }; 
                              

    void* operator new(size_t aBaseSize, size_t aDataSize) {
        
        return ::operator new(aBaseSize + aDataSize -
                              sizeof(char) * block_chars);
    }

    


    void Destroy();

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
    nsCSSSVG mSVG;
    nsCSSColumn mColumn;

    








    void Expand(nsCSSCompressedDataBlock *aNormalBlock,
                nsCSSCompressedDataBlock *aImportantBlock);

    




    void Compress(nsCSSCompressedDataBlock **aNormalBlock,
                  nsCSSCompressedDataBlock **aImportantBlock);

    


    void Clear();

    



    void ClearProperty(nsCSSProperty aPropID);

    


    void ClearLonghandProperty(nsCSSProperty aPropID);

    











    void TransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                           nsCSSProperty aPropID,
                           PRBool aIsImportant,
                           PRBool aOverrideImportant,
                           PRBool aMustCallValueAppended,
                           mozilla::css::Declaration* aDeclaration,
                           PRBool* aChanged);

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

    


    void DoTransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                             nsCSSProperty aPropID,
                             PRBool aIsImportant,
                             PRBool aOverrideImportant,
                             PRBool aMustCallValueAppended,
                             mozilla::css::Declaration* aDeclaration,
                             PRBool* aChanged);

#ifdef DEBUG
    void DoAssertInitialState();
#endif

    
    
    static const size_t kOffsetTable[];

    





    nsCSSPropertySet mPropertiesSet;
    


    nsCSSPropertySet mPropertiesImportant;

public:
    




    void* PropertyAt(nsCSSProperty aProperty) {
        size_t offset = nsCSSExpandedDataBlock::kOffsetTable[aProperty];
        return reinterpret_cast<void*>(reinterpret_cast<char*>(this) + offset);
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
