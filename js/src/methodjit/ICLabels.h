









































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











#if defined JS_CPU_X64 || defined JS_CPU_ARM || defined JS_CPU_SPARC
# define JS_HAS_IC_LABELS
#endif


struct GetPropLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setValueLoad(MacroAssembler &masm, Label fastPathRejoin, Label fastValueLoad) {
        int offset = masm.differenceBetween(fastPathRejoin, fastValueLoad);
#ifdef JS_HAS_IC_LABELS
        inlineValueLoadOffset = offset;
#endif
        



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

    void setInlineShapeData(MacroAssembler &masm, Label shapeGuard, DataLabel32 inlineShape) {
        int offset = masm.differenceBetween(shapeGuard, inlineShape);
        setInlineShapeOffset(offset);
    }

    CodeLocationDataLabel32 getInlineShapeData(CodeLocationLabel fastShapeGuard) {
        return fastShapeGuard.dataLabel32AtOffset(getInlineShapeOffset());
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
#ifdef JS_HAS_IC_LABELS
        dslotsLoadOffset = offset;
#endif
        JS_ASSERT(offset == dslotsLoadOffset);
    }

    void setInlineShapeOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        inlineShapeOffset = offset;
#endif
        JS_ASSERT(offset == inlineShapeOffset);
    }
    
    void setStubShapeJumpOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        stubShapeJumpOffset = offset;
#endif
        JS_ASSERT(offset == stubShapeJumpOffset);
    }

    int getInlineShapeJumpOffset() {
#if defined JS_CPU_X64
        return getInlineShapeOffset() + INLINE_SHAPE_JUMP;
#else
        return POST_INST_OFFSET(INLINE_SHAPE_JUMP);
#endif
    }

    void setInlineShapeJumpOffset(int offset) {
        JS_ASSERT(INLINE_SHAPE_JUMP == offset);
    }

    int getInlineTypeJumpOffset() {
#if defined JS_CPU_X86 || defined JS_CPU_X64
        return INLINE_TYPE_JUMP;
#elif defined JS_CPU_ARM || defined JS_CPU_SPARC
        return POST_INST_OFFSET(inlineTypeJumpOffset);
#endif
    }

    void setInlineTypeJumpOffset(int offset) {
#if defined JS_CPU_X86 || defined JS_CPU_X64
        JS_ASSERT(INLINE_TYPE_JUMP == offset);
#elif defined JS_CPU_ARM || defined JS_CPU_SPARC
        inlineTypeJumpOffset = offset;
        JS_ASSERT(offset == inlineTypeJumpOffset);
#endif
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

#if defined JS_CPU_X86 
    static const int32 INLINE_SHAPE_JUMP = 12;
    static const int32 INLINE_TYPE_JUMP = 12;
#elif defined JS_CPU_X64
    static const int32 INLINE_SHAPE_JUMP = 6;
    static const int32 INLINE_TYPE_JUMP = 19;
#elif defined JS_CPU_ARM
    
    static const int32 INLINE_SHAPE_JUMP = 12;

    
    int32 inlineTypeJumpOffset : 8;
#elif defined JS_CPU_SPARC
    static const int32 INLINE_SHAPE_JUMP = 48;
    static const int32 INLINE_TYPE_JUMP = 48;
    
    int32 inlineTypeJumpOffset : 8;
#endif
};


struct SetPropLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setInlineValueStore(MacroAssembler &masm, Label fastPathRejoin, DataLabel32 inlineValueStore,
                             const ValueRemat &vr) {
        int offset = masm.differenceBetween(fastPathRejoin, inlineValueStore);
        setInlineValueStoreOffset(offset, vr.isConstant(), vr.isTypeKnown());
    }

    CodeLocationLabel getInlineValueStore(CodeLocationLabel fastPathRejoin, const ValueRemat &vr) {
        return fastPathRejoin.labelAtOffset(getInlineValueStoreOffset(vr.isConstant(),
                                                                      vr.isTypeKnown()));
    }

    void setInlineShapeData(MacroAssembler &masm, Label shapeGuard, DataLabel32 inlineShapeData) {
        int offset = masm.differenceBetween(shapeGuard, inlineShapeData);
        setInlineShapeDataOffset(offset);
    }

    CodeLocationDataLabel32 getInlineShapeData(CodeLocationLabel fastPathStart, int shapeGuardOffset) {
        return fastPathStart.dataLabel32AtOffset(shapeGuardOffset + getInlineShapeDataOffset());
    }

    void setDslotsLoad(MacroAssembler &masm, Label fastPathRejoin, Label beforeLoad,
                       const ValueRemat &rhs) {
        int offset = masm.differenceBetween(fastPathRejoin, beforeLoad);
        setDslotsLoadOffset(offset, rhs.isConstant(), rhs.isTypeKnown());
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

    

    void setDslotsLoadOffset(int offset, bool isConstant, bool isTypeKnown) {
#if defined JS_HAS_IC_LABELS
        dslotsLoadOffset = offset;
        JS_ASSERT(offset == dslotsLoadOffset);
#elif defined JS_CPU_X86
        JS_ASSERT_IF(isConstant, offset == INLINE_DSLOTS_BEFORE_CONSTANT);
        JS_ASSERT_IF(isTypeKnown && !isConstant, offset == INLINE_DSLOTS_BEFORE_KTYPE);
        JS_ASSERT_IF(!isTypeKnown, offset == INLINE_DSLOTS_BEFORE_DYNAMIC);
#else
# error
#endif
    }

    int getDslotsLoadOffset(const ValueRemat &vr) {
#if defined JS_CPU_X86
        if (vr.isConstant())
            return INLINE_DSLOTS_BEFORE_CONSTANT;
        if (vr.isTypeKnown())
            return INLINE_DSLOTS_BEFORE_KTYPE;
        return INLINE_DSLOTS_BEFORE_DYNAMIC;
#else
        (void) vr;
        return dslotsLoadOffset;
#endif
    }

    void setInlineShapeDataOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        inlineShapeDataOffset = offset;
#endif
        JS_ASSERT(offset == inlineShapeDataOffset);
    }

    void setStubShapeJumpOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        stubShapeJumpOffset = offset;
#endif
        JS_ASSERT(offset == stubShapeJumpOffset);
    }

    void setInlineValueStoreOffset(int offset, bool isConstant, bool isTypeKnown) {
#ifdef JS_HAS_IC_LABELS
        inlineValueStoreOffset = offset;
        JS_ASSERT(offset == inlineValueStoreOffset);
#elif defined JS_CPU_X86
        JS_ASSERT_IF(isConstant, offset == INLINE_VALUE_STORE_CONSTANT);
        JS_ASSERT_IF(isTypeKnown && !isConstant, offset == INLINE_VALUE_STORE_KTYPE);
        JS_ASSERT_IF(!isTypeKnown && !isConstant, offset == INLINE_VALUE_STORE_DYNAMIC);
#endif
    }

    void setInlineShapeJumpOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        inlineShapeJumpOffset = offset;
#endif
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

    int getInlineValueStoreOffset(bool isConstant, bool isTypeKnown) {
#ifdef JS_HAS_IC_LABELS
        return inlineValueStoreOffset;
#elif defined JS_CPU_X86
        if (isConstant)
            return INLINE_VALUE_STORE_CONSTANT;
        else if (isTypeKnown)
            return INLINE_VALUE_STORE_KTYPE;
        else
            return INLINE_VALUE_STORE_DYNAMIC;
#endif
    }

    
#if defined JS_CPU_X86
    static const int INLINE_DSLOTS_BEFORE_CONSTANT = -23;
    static const int INLINE_DSLOTS_BEFORE_KTYPE = -19;
    static const int INLINE_DSLOTS_BEFORE_DYNAMIC = -15;
#else
    int32 dslotsLoadOffset : 8;
#endif

    
    int32 inlineShapeDataOffset : 8;

    




    int32 stubShapeJumpOffset : 8;

#if defined JS_CPU_X86
    static const int INLINE_VALUE_STORE_CONSTANT = -20;
    static const int INLINE_VALUE_STORE_KTYPE = -16;
    static const int INLINE_VALUE_STORE_DYNAMIC = -12;
#else
    int32 inlineValueStoreOffset : 8;
#endif

    
    int32 inlineShapeJumpOffset : 8;
};


struct BindNameLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setInlineJumpOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        inlineJumpOffset = offset;
#endif
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
#ifdef JS_HAS_IC_LABELS
        stubJumpOffset = offset;
#endif
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
#ifdef JS_HAS_IC_LABELS
        inlineJumpOffset = offset;
#endif
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
#ifdef JS_HAS_IC_LABELS
        stubJumpOffset = offset;
#endif
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
