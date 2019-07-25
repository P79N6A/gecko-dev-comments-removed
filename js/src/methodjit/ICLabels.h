









































#if !defined jsjaeger_ic_labels_h__ && defined JS_METHODJIT
#define jsjaeger_ic_labels_h__

#include "jscntxt.h"
#include "jstl.h"
#include "jsvector.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/CodeGenIncludes.h"
#include "methodjit/MethodJIT.h"
#include "BaseAssembler.h"
#include "RematInfo.h"
#include "BaseCompiler.h"

class ICOffsetInitializer {
  public:
    ICOffsetInitializer();
};

namespace js {
namespace mjit {
namespace ic {











#if defined JS_CPU_X64 || defined JS_CPU_ARM
# define JS_HAS_IC_LABELS
#endif

# define JS_POLYIC_OFFSET_BITS 8


struct GetPropLabels : MacroAssemblerTypedefs {
    friend class ::ICOffsetInitializer;

    void setValueLoad(MacroAssembler &masm, Label fastPathRejoin, Label fastValueLoad) {
        int offset = masm.differenceBetween(fastPathRejoin, fastValueLoad);
#ifdef JS_HAS_IC_LABELS
        inlineValueLoadOffset = offset;
#endif
        



        JS_ASSERT(offset == inlineValueLoadOffset);
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
#if defined JS_CPU_X86 || defined JS_CPU_X64
        return fastPathStart.jumpAtOffset(getInlineTypeJumpOffset());
#elif defined JS_CPU_ARM
        
        return fastPathStart.jumpAtOffset(getInlineTypeJumpOffset() - sizeof(ARMWord));
#endif
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
    
    void setStubShapeJump(int offset) {
#ifdef JS_HAS_IC_LABELS
        stubShapeJumpOffset = offset;
#endif
        JS_ASSERT(offset == stubShapeJumpOffset);
    }

    int getInlineShapeJumpOffset() {
#if defined JS_CPU_X86
        return INLINE_SHAPE_JUMP;
#elif defined JS_CPU_X64
        return getInlineShapeOffset() + INLINE_SHAPE_JUMP;
#elif defined JS_CPU_ARM
        return INLINE_SHAPE_JUMP - sizeof(ARMWord);
#endif
    }

    void setInlineShapeJumpOffset(int offset) {
        JS_ASSERT(INLINE_SHAPE_JUMP == offset);
    }

    int getInlineTypeJumpOffset() {
#if defined JS_CPU_X86 || defined JS_CPU_X64
        return INLINE_TYPE_JUMP;
#elif defined JS_CPU_ARM
        return inlineTypeJumpOffset;
#endif
    }

    void setInlineTypeJumpOffset(int offset) {
#if defined JS_CPU_X86 || defined JS_CPU_X64
        JS_ASSERT(INLINE_TYPE_JUMP == offset);
#elif defined JS_CPU_ARM
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
#if defined JS_CPU_X86 || defined JS_CPU_X64
        return stubShapeJumpOffset;
#elif defined JS_CPU_ARM
        return stubShapeJumpOffset - sizeof(ARMWord);
#endif
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
#endif
};


struct SetPropLabels {
    friend class ::ICOffsetInitializer;

#ifdef JS_PUNBOX64
    void setDslotsLoadOffset(int offset) {
# ifdef JS_HAS_IC_LABELS
        dslotsLoadOffset = offset;
# endif
        JS_ASSERT(offset == dslotsLoadOffset);
    }
#endif

    int getDslotsLoadOffset() {
#ifdef JS_PUNBOX64
        return dslotsLoadOffset;
#else
        JS_NOT_REACHED("this is inlined");
        return 0;
#endif
    }

    void setInlineShapeOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        inlineShapeOffset = offset;
#endif
        JS_ASSERT(offset == inlineShapeOffset);
    }

    void setStubShapeJump(int offset) {
#ifdef JS_HAS_IC_LABELS
        stubShapeJump = offset;
#endif
        JS_ASSERT(offset == stubShapeJump);
    }

    int getInlineShapeOffset() {
        return inlineShapeOffset;
    }
    int getStubShapeJump() {
        return stubShapeJump;
    }

  private:
#ifdef JS_PUNBOX64
    
    int32 dslotsLoadOffset : 8;
#endif

    
    int32 inlineShapeOffset : 8;

    




    int32 stubShapeJump : 8;
};


struct BindNameLabels {
    friend class ::ICOffsetInitializer;

    void setInlineJumpOffset(int offset) {
#ifdef JS_HAS_IC_LABELS
        inlineJumpOffset = offset;
#endif
        JS_ASSERT(offset == inlineJumpOffset);
    }

    int getInlineJumpOffset() {
        return inlineJumpOffset;
    }

  private:
    
    int32 inlineJumpOffset : 8;
};

} 
} 
} 

#endif 

