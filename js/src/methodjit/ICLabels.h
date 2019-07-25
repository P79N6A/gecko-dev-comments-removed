









































#if !defined jsjaeger_ic_labels_h__ && defined JS_METHODJIT
#define jsjaeger_ic_labels_h__

#include "jscntxt.h"
#include "jstl.h"
#include "jsvector.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
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


struct GetPropLabels {
    friend class ::ICOffsetInitializer;

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
        stubShapeJump = offset;
#endif
        JS_ASSERT(offset == stubShapeJump);
    }

#ifdef JS_NUNBOX32
    void setTypeLoad(int offset) {
# ifdef JS_HAS_IC_LABELS
        inlineTypeLoad = offset;
# endif
        JS_ASSERT(offset == inlineTypeLoad);
    }
    void setDataLoad(int offset) {
# ifdef JS_HAS_IC_LABELS
        inlineDataLoad = offset;
# endif
        JS_ASSERT(offset == inlineDataLoad);
    }
    JSC::CodeLocationDataLabel32 getTypeLoad(JSC::CodeLocationLabel start) {
        return start.dataLabel32AtOffset(inlineTypeLoad);
    }
    JSC::CodeLocationDataLabel32 getDataLoad(JSC::CodeLocationLabel start) {
        return start.dataLabel32AtOffset(inlineDataLoad);
    }
#elif JS_PUNBOX64
    void setValueLoad(int offset) {
# ifdef JS_HAS_IC_LABELS
        inlineValueLoad = offset;
# endif
        JS_ASSERT(offset == inlineValueLoad);
    }
    JSC::CodeLocationDataLabel32 getValueLoad(JSC::CodeLocationLabel start) {
        return start.dataLabel32AtOffset(inlineValueLoad);
    }
#endif

    int getInlineShapeOffset() {
        return inlineShapeOffset;
    }
    int getDslotsLoadOffset() {
        return dslotsLoadOffset;
    }
    int getStubShapeJump() {
        return stubShapeJump;
    }

  private:
    
    int32 dslotsLoadOffset : 8;

    
    int32 inlineShapeOffset : 8;

    
#ifdef JS_NUNBOX32
    int32 inlineTypeLoad    : 8;
    int32 inlineDataLoad    : 8;
#elif JS_PUNBOX64
    int32 inlineValueLoad   : 8;
#endif

    




    int32 stubShapeJump : 8;
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

