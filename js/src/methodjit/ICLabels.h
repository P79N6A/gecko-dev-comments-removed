









































#if !defined jsjaeger_ic_labels_h__ && defined JS_METHODJIT
#define jsjaeger_ic_labels_h__

#include "methodjit/BaseCompiler.h"

class ICOffsetInitializer {
  public:
    ICOffsetInitializer();
};

namespace js {
namespace mjit {
namespace ic {


struct GetPropLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setValueLoad(MacroAssembler &masm, Label fastPathRejoin, Label fastValueLoad) {
        int offset = masm.differenceBetween(fastPathRejoin, fastValueLoad);
        inlineValueLoadOffset = offset;

        



        JS_ASSERT(offset == inlineValueLoadOffset);
        (void) offset;
    }

    CodeLocationLabel getValueLoad(CodeLocationLabel fastPathRejoin) {
        return fastPathRejoin.labelAtOffset(inlineValueLoadOffset);
    }

    void setDslotsLoad(MacroAssembler &masm, Label fastPathRejoin, Label dslotsLoad) {
        int offset = masm.differenceBetween(fastPathRejoin, dslotsLoad);
        setDslotsLoadOffset(offset);
    }

    CodeLocationInstruction getDslotsLoad(CodeLocationLabel fastPathRejoin) {
        return fastPathRejoin.instructionAtOffset(getDslotsLoadOffset());
    }

    void setInlineShapeData(MacroAssembler &masm, Label shapeGuard, DataLabelPtr inlineShape) {
        int offset = masm.differenceBetween(shapeGuard, inlineShape);
        setInlineShapeOffset(offset);
    }

    CodeLocationDataLabelPtr getInlineShapeData(CodeLocationLabel fastShapeGuard) {
        return fastShapeGuard.dataLabelPtrAtOffset(getInlineShapeOffset());
    }

    



    template <typename T>
    void setInlineShapeJump(MacroAssembler &masm, T base, Label afterJump) {
        setInlineShapeJumpOffset(masm.differenceBetween(base, afterJump));
    }

    CodeLocationJump getInlineShapeJump(CodeLocationLabel fastShapeGuard) {
        return fastShapeGuard.jumpAtOffset(getInlineShapeJumpOffset());
    }

    void setInlineTypeJump(MacroAssembler &masm, Label fastPathStart, Label afterTypeJump) {
        int offset = masm.differenceBetween(fastPathStart, afterTypeJump);
        setInlineTypeJumpOffset(offset);
    }

    CodeLocationJump getInlineTypeJump(CodeLocationLabel fastPathStart) {
        return fastPathStart.jumpAtOffset(getInlineTypeJumpOffset());
    }

    void setStubShapeJump(MacroAssembler &masm, Label stubStart, Label shapeJump) {
        int offset = masm.differenceBetween(stubStart, shapeJump);
        setStubShapeJumpOffset(offset);
    }

    

    void setDslotsLoadOffset(int offset) {
        dslotsLoadOffset = offset;
        JS_ASSERT(offset == dslotsLoadOffset);
    }

    void setInlineShapeOffset(int offset) {
        inlineShapeOffset = offset;
        JS_ASSERT(offset == inlineShapeOffset);
    }
    
    void setStubShapeJumpOffset(int offset) {
        stubShapeJumpOffset = offset;
        JS_ASSERT(offset == stubShapeJumpOffset);
    }

    int getInlineShapeJumpOffset() {
        return POST_INST_OFFSET(inlineShapeJumpOffset);
    }

    void setInlineShapeJumpOffset(int offset) {
        inlineShapeJumpOffset = offset;
        JS_ASSERT(offset == inlineShapeJumpOffset);
    }

    int getInlineTypeJumpOffset() {
        return POST_INST_OFFSET(inlineTypeJumpOffset);
    }

    void setInlineTypeJumpOffset(int offset) {
        inlineTypeJumpOffset = offset;
        JS_ASSERT(offset == inlineTypeJumpOffset);
     }

    int getInlineShapeOffset() {
        return inlineShapeOffset;
    }
    int getDslotsLoadOffset() {
        return dslotsLoadOffset;
    }
    int getStubShapeJumpOffset() {
        return POST_INST_OFFSET(stubShapeJumpOffset);
    }

  private:
    
    int32 dslotsLoadOffset : 8;

    
    int32 inlineShapeOffset : 8;

    
    int32 inlineValueLoadOffset : 8;

    




    int32 stubShapeJumpOffset : 8;

    
    int32 inlineShapeJumpOffset : 8;

    
    int32 inlineTypeJumpOffset : 8;
};


struct SetPropLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setInlineValueStore(MacroAssembler &masm, Label fastPathRejoin, DataLabel32 inlineValueStore) {
        int offset = masm.differenceBetween(fastPathRejoin, inlineValueStore);
        setInlineValueStoreOffset(offset);
    }

    CodeLocationLabel getInlineValueStore(CodeLocationLabel fastPathRejoin) {
        return fastPathRejoin.labelAtOffset(getInlineValueStoreOffset());
    }

    void setInlineShapeData(MacroAssembler &masm, Label shapeGuard, DataLabelPtr inlineShapeData) {
        int offset = masm.differenceBetween(shapeGuard, inlineShapeData);
        setInlineShapeDataOffset(offset);
    }

    CodeLocationDataLabelPtr getInlineShapeData(CodeLocationLabel fastPathStart, int shapeGuardOffset) {
        return fastPathStart.dataLabelPtrAtOffset(shapeGuardOffset + getInlineShapeDataOffset());
    }

    void setDslotsLoad(MacroAssembler &masm, Label fastPathRejoin, Label beforeLoad) {
        int offset = masm.differenceBetween(fastPathRejoin, beforeLoad);
        setDslotsLoadOffset(offset);
    }

    CodeLocationInstruction getDslotsLoad(CodeLocationLabel fastPathRejoin, const ValueRemat &vr) {
        return fastPathRejoin.instructionAtOffset(getDslotsLoadOffset(vr));
    }

    void setInlineShapeJump(MacroAssembler &masm, Label shapeGuard, Label afterJump) {
        setInlineShapeJumpOffset(masm.differenceBetween(shapeGuard, afterJump));
    }

    CodeLocationJump getInlineShapeJump(CodeLocationLabel shapeGuard) {
        return shapeGuard.jumpAtOffset(getInlineShapeJumpOffset());
    }

    void setStubShapeJump(MacroAssembler &masm, Label stubStart, Label afterShapeJump) {
        int offset = masm.differenceBetween(stubStart, afterShapeJump);
        setStubShapeJumpOffset(offset);
    }

    CodeLocationJump getStubShapeJump(CodeLocationLabel stubStart) {
        return stubStart.jumpAtOffset(getStubShapeJumpOffset());
    }

  private:

    

    void setDslotsLoadOffset(int offset) {
        dslotsLoadOffset = offset;
        JS_ASSERT(offset == dslotsLoadOffset);
    }

    int getDslotsLoadOffset(const ValueRemat &vr) {
        (void) vr;
        return dslotsLoadOffset;
    }

    void setInlineShapeDataOffset(int offset) {
        inlineShapeDataOffset = offset;
        JS_ASSERT(offset == inlineShapeDataOffset);
    }

    void setStubShapeJumpOffset(int offset) {
        stubShapeJumpOffset = offset;
        JS_ASSERT(offset == stubShapeJumpOffset);
    }

    void setInlineValueStoreOffset(int offset) {
        inlineValueStoreOffset = offset;
        JS_ASSERT(offset == inlineValueStoreOffset);
    }

    void setInlineShapeJumpOffset(int offset) {
        inlineShapeJumpOffset = offset;
        JS_ASSERT(offset == inlineShapeJumpOffset);
    }

    int getInlineShapeJumpOffset() {
        return POST_INST_OFFSET(inlineShapeJumpOffset);
    }

    int getInlineShapeDataOffset() {
        return inlineShapeDataOffset;
    }

    int getStubShapeJumpOffset() {
        return POST_INST_OFFSET(stubShapeJumpOffset);
    }

    int getInlineValueStoreOffset() {
        return inlineValueStoreOffset;
    }

    
    int32 dslotsLoadOffset : 8;

    
    int32 inlineShapeDataOffset : 8;

    




    int32 stubShapeJumpOffset : 8;

    int32 inlineValueStoreOffset : 8;

    
    int32 inlineShapeJumpOffset : 8;
};


struct BindNameLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setInlineJumpOffset(int offset) {
        inlineJumpOffset = offset;
        JS_ASSERT(offset == inlineJumpOffset);
    }

    void setInlineJump(MacroAssembler &masm, Label shapeGuard, Jump inlineJump) {
        int offset = masm.differenceBetween(shapeGuard, inlineJump);
        setInlineJumpOffset(offset);
    }

    CodeLocationJump getInlineJump(CodeLocationLabel fastPathStart) {
        return fastPathStart.jumpAtOffset(getInlineJumpOffset());
    }

    int getInlineJumpOffset() {
        return inlineJumpOffset;
    }

    void setStubJumpOffset(int offset) {
        stubJumpOffset = offset;
        JS_ASSERT(offset == stubJumpOffset);
    }

    void setStubJump(MacroAssembler &masm, Label stubStart, Jump stubJump) {
        int offset = masm.differenceBetween(stubStart, stubJump);
        setStubJumpOffset(offset);
    }

    CodeLocationJump getStubJump(CodeLocationLabel lastStubStart) {
        return lastStubStart.jumpAtOffset(getStubJumpOffset());
    }

    int getStubJumpOffset() {
        return stubJumpOffset;
    }

  private:
    
    int32 inlineJumpOffset : 8;

    
    int32 stubJumpOffset : 8;
};


struct ScopeNameLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setInlineJumpOffset(int offset) {
        inlineJumpOffset = offset;
        JS_ASSERT(offset == inlineJumpOffset);
    }

    void setInlineJump(MacroAssembler &masm, Label fastPathStart, Jump inlineJump) {
        int offset = masm.differenceBetween(fastPathStart, inlineJump);
        setInlineJumpOffset(offset);
    }

    CodeLocationJump getInlineJump(CodeLocationLabel fastPathStart) {
        return fastPathStart.jumpAtOffset(getInlineJumpOffset());
    }

    int getInlineJumpOffset() {
        return inlineJumpOffset;
    }

    void setStubJumpOffset(int offset) {
        stubJumpOffset = offset;
        JS_ASSERT(offset == stubJumpOffset);
    }

    void setStubJump(MacroAssembler &masm, Label stubStart, Jump stubJump) {
        int offset = masm.differenceBetween(stubStart, stubJump);
        setStubJumpOffset(offset);
    }

    CodeLocationJump getStubJump(CodeLocationLabel lastStubStart) {
        return lastStubStart.jumpAtOffset(getStubJumpOffset());
    }

    int getStubJumpOffset() {
        return stubJumpOffset;
    }

  private:
    
    int32 inlineJumpOffset : 8;

    
    int32 stubJumpOffset : 8;
};

} 
} 
} 

#endif 
