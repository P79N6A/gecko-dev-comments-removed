






































#include "jsprf.h"
#include "jstl.h"

#include "jscompartment.h"
#include "Writer.h"
#include "nanojit.h"

#include "vm/ArgumentsObject.h"

namespace js {
namespace tjit {

using namespace nanojit;

class FuncFilter : public LirWriter
{
public:
    FuncFilter(LirWriter *out):
        LirWriter(out)
    {
    }

    LIns *ins2(LOpcode v, LIns *s0, LIns *s1)
    {
        if (s0 == s1 && v == LIR_eqd) {
            
            if (IsPromotedInt32OrUint32(s0)) {
                
                
                return insImmI(1);
            }
            if (s0->isop(LIR_addd) || s0->isop(LIR_subd) || s0->isop(LIR_muld)) {
                LIns *lhs = s0->oprnd1();
                LIns *rhs = s0->oprnd2();
                if (IsPromotedInt32OrUint32(lhs) && IsPromotedInt32OrUint32(rhs)) {
                    
                    
                    
                    
                    return insImmI(1);
                }
            }
        } else if (isCmpDOpcode(v)) {
            if (IsPromotedInt32(s0) && IsPromotedInt32(s1)) {
                v = cmpOpcodeD2I(v);
                return out->ins2(v, DemoteToInt32(out, s0), DemoteToInt32(out, s1));
            } else if (IsPromotedUint32(s0) && IsPromotedUint32(s1)) {
                
                v = cmpOpcodeD2UI(v);
                return out->ins2(v, DemoteToUint32(out, s0), DemoteToUint32(out, s1));
            }
        }
        return out->ins2(v, s0, s1);
    }
};

void
Writer::init(LogControl *logc_, Config *njConfig_)
{
    JS_ASSERT(logc_ && njConfig_);
    logc = logc_;
    njConfig = njConfig_;

    LirWriter *&lir = InitConst(this->lir);
    CseFilter *&cse = InitConst(this->cse);
    lir = new (alloc) LirBufWriter(lirbuf, *njConfig);
#ifdef DEBUG
    ValidateWriter *validate2;
    lir = validate2 =
        new (alloc) ValidateWriter(lir, lirbuf->printer, "end of writer pipeline");
#endif
#ifdef JS_JIT_SPEW
    if (logc->lcbits & LC_TMRecorder)
       lir = new (alloc) VerboseWriter(*alloc, lir, lirbuf->printer, logc);
#endif
    
    if (njConfig->cseopt)
        cse = new (alloc) CseFilter(lir, TM_NUM_USED_ACCS, *alloc);
        if (!cse->initOOM)
            lir = cse;      
    lir = new (alloc) ExprFilter(lir);
    lir = new (alloc) FuncFilter(lir);
#ifdef DEBUG
    ValidateWriter *validate1 =
        new (alloc) ValidateWriter(lir, lirbuf->printer, "start of writer pipeline");
    lir = validate1;
#endif
}

bool
IsPromotedInt32(LIns* ins)
{
    if (ins->isop(LIR_i2d))
        return true;
    if (ins->isImmD()) {
        jsdouble d = ins->immD();
        return d == jsdouble(jsint(d)) && !JSDOUBLE_IS_NEGZERO(d);
    }
    return false;
}

bool
IsPromotedUint32(LIns* ins)
{
    if (ins->isop(LIR_ui2d))
        return true;
    if (ins->isImmD()) {
        jsdouble d = ins->immD();
        return d == jsdouble(jsuint(d)) && !JSDOUBLE_IS_NEGZERO(d);
    }
    return false;
}

bool
IsPromotedInt32OrUint32(LIns* ins)
{
    return IsPromotedInt32(ins) || IsPromotedUint32(ins);
}

LIns *
DemoteToInt32(LirWriter *out, LIns *ins)
{
    JS_ASSERT(IsPromotedInt32(ins));
    if (ins->isop(LIR_i2d))
        return ins->oprnd1();
    JS_ASSERT(ins->isImmD());
    return out->insImmI(int32_t(ins->immD()));
}

LIns *
DemoteToUint32(LirWriter *out, LIns *ins)
{
    JS_ASSERT(IsPromotedUint32(ins));
    if (ins->isop(LIR_ui2d))
        return ins->oprnd1();
    JS_ASSERT(ins->isImmD());
    return out->insImmI(uint32_t(ins->immD()));
}

}   
}   

#ifdef DEBUG
namespace nanojit {

using namespace js;
using namespace js::tjit;

static bool
match(LIns *base, LOpcode opcode, AccSet accSet, int32_t disp)
{
    return base->isop(opcode) &&
           base->accSet() == accSet &&
           base->disp() == disp;
}

static bool
match(LIns *base, LOpcode opcode, AccSet accSet, LoadQual loadQual, int32_t disp)
{
    return base->isop(opcode) &&
           base->accSet() == accSet &&
           base->loadQual() == loadQual &&
           base->disp() == disp;
}

static bool
couldBeObjectOrString(LIns *ins)
{
    bool ret = false;

    if (ins->isop(LIR_callp)) {
        
        ret = true;

    } else if (ins->isop(LIR_ldp)) {
        
        ret = true;

    } else if (ins->isImmP()) {
        
        uintptr_t val = uintptr_t(ins->immP());
        if (val == 0 || val > 4096)
            ret = true;         

    } else if (ins->isop(LIR_cmovp)) {
        
        ret = couldBeObjectOrString(ins->oprnd2()) &&
              couldBeObjectOrString(ins->oprnd3());

    } else if (ins->isop(LIR_ori) &&
               ins->oprnd1()->isop(LIR_andi) &&
               ins->oprnd2()->isop(LIR_andi))
    {
        
        
        
        
        
        
        ret = true;

#if JS_BITS_PER_WORD == 64
    } else if (ins->isop(LIR_andq) &&
               ins->oprnd1()->isop(LIR_ldq) &&
               ins->oprnd2()->isImmQ() &&
               uintptr_t(ins->oprnd2()->immQ()) == JSVAL_PAYLOAD_MASK)
    {
        
        
        
        ret = true;
#endif
    } else if (ins->isop(LIR_addp) &&
               ((ins->oprnd1()->isImmP() &&
                 (void *)ins->oprnd1()->immP() == JSAtom::unitStaticTable) ||
                (ins->oprnd2()->isImmP() &&
                 (void *)ins->oprnd2()->immP() == JSAtom::unitStaticTable)))
    {
        
        
        
        
        ret = true;
    }

    return ret;
}

static bool
isConstPrivatePtr(LIns *ins, unsigned slot)
{
#if JS_BITS_PER_WORD == 32
    
    return match(ins, LIR_ldp, ACCSET_SLOTS, LOAD_CONST, slot * sizeof(Value) + sPayloadOffset);
#elif JS_BITS_PER_WORD == 64
    
    
    
    return ins->isop(LIR_lshq) &&
           match(ins->oprnd1(), LIR_ldp, ACCSET_SLOTS, LOAD_CONST, slot * sizeof(Value)) &&
           ins->oprnd2()->isImmI(1);
#endif
}



















void ValidateWriter::checkAccSet(LOpcode op, LIns *base, int32_t disp, AccSet accSet)
{
    bool ok;

    NanoAssert(accSet != ACCSET_NONE);

    #define dispWithin(Struct) \
        (0 <= disp && disp < int32_t(sizeof(Struct)))

    switch (accSet) {
      case ACCSET_STATE:
        
        
        ok = dispWithin(TracerState) && 
             base->isop(LIR_paramp) &&
             base->paramKind() == 0 &&
             base->paramArg() == 0;
        break;

      case ACCSET_STACK:
        
        
        
        
        
        
        ok = match(base, LIR_ldp, ACCSET_STATE, offsetof(TracerState, sp)) ||
             (base->isop(LIR_addp) &&
              match(base->oprnd1(), LIR_ldp, ACCSET_STATE, offsetof(TracerState, sp)));
        break;

      case ACCSET_RSTACK:
        
        
        
        
        
        ok = (op == LIR_ldp || op == LIR_stp) &&
             (match(base, LIR_ldp, ACCSET_STATE, offsetof(TracerState, rp)) ||
              match(base, LIR_ldp, ACCSET_STATE, offsetof(TracerState, callstackBase)));
        break;

      case ACCSET_CX:
        
        
        ok = dispWithin(JSContext) &&
             match(base, LIR_ldp, ACCSET_STATE, offsetof(TracerState, cx));
        break;

      case ACCSET_TM:
          
          ok = base->isImmP() && disp == 0;
          break;

      case ACCSET_EOS:
        
        
        ok = match(base, LIR_ldp, ACCSET_STATE, offsetof(TracerState, eos));
        break;

      case ACCSET_ALLOC:
        
        
        
        
        
        
        ok = base->isop(LIR_allocp) ||
             (base->isop(LIR_addp) &&
              base->oprnd1()->isop(LIR_allocp));
        break;

      case ACCSET_FRAMEREGS:
        
        
        ok = op == LIR_ldp &&
             dispWithin(FrameRegs) && 
             match(base, LIR_ldp, ACCSET_SEG, StackSegment::offsetOfRegs());
        break;

      case ACCSET_STACKFRAME:
        
        
        ok = dispWithin(StackFrame) && 
             match(base, LIR_ldp, ACCSET_FRAMEREGS, FrameRegs::offsetOfFp);
        break;

      case ACCSET_RUNTIME:
        
        
        ok = dispWithin(JSRuntime) &&
             match(base, LIR_ldp, ACCSET_CX, offsetof(JSContext, runtime));
        break;

      
      
      
      
      #define OK_OBJ_FIELD(ldop, field) \
            op == ldop && \
            disp == offsetof(JSObject, field) && \
            couldBeObjectOrString(base)

      case ACCSET_OBJ_CLASP:
        ok = OK_OBJ_FIELD(LIR_ldp, clasp);
        break;

      case ACCSET_OBJ_FLAGS:
        ok = OK_OBJ_FIELD(LIR_ldi, flags);
        break;

      case ACCSET_OBJ_SHAPE:
        ok = OK_OBJ_FIELD(LIR_ldi, objShape);
        break;

      case ACCSET_OBJ_PROTO:
        ok = OK_OBJ_FIELD(LIR_ldp, proto);
        break;

      case ACCSET_OBJ_PARENT:
        ok = OK_OBJ_FIELD(LIR_ldp, parent);
        break;

      case ACCSET_OBJ_PRIVATE:
        
        
        ok = (op == LIR_ldi || op == LIR_ldp ||
              op == LIR_sti || op == LIR_stp) &&
             disp == offsetof(JSObject, privateData) &&
             couldBeObjectOrString(base);
        break;

      case ACCSET_OBJ_CAPACITY:
        ok = OK_OBJ_FIELD(LIR_ldi, capacity);
        break;

      case ACCSET_OBJ_SLOTS:
        ok = OK_OBJ_FIELD(LIR_ldp, slots);
        break;

      case ACCSET_SLOTS:
        
        
        
        
        
        
        
        
        
        
        
        ok = couldBeObjectOrString(base) ||
             match(base, LIR_ldp, ACCSET_OBJ_SLOTS, offsetof(JSObject, slots)) ||
             (base->isop(LIR_addp) &&
              match(base->oprnd1(), LIR_ldp, ACCSET_OBJ_SLOTS, offsetof(JSObject, slots)));
        break;

      case ACCSET_TARRAY:
        
        
        
        
        ok = (op == LIR_ldi || op == LIR_ldp) &&
             dispWithin(TypedArray) &&
             match(base, LIR_ldp, ACCSET_OBJ_PRIVATE, offsetof(JSObject, privateData));
        break;

      case ACCSET_TARRAY_DATA:
        
        
        
        
        
        
        ok = match(base, LIR_ldp, ACCSET_TARRAY, LOAD_CONST, TypedArray::dataOffset()) ||
             (base->isop(LIR_addp) &&
              match(base->oprnd1(), LIR_ldp, ACCSET_TARRAY, LOAD_CONST, TypedArray::dataOffset()));
        break;

      case ACCSET_ITER:
        
        
        ok = (op == LIR_ldp || op == LIR_stp) &&
             dispWithin(NativeIterator) &&
             match(base, LIR_ldp, ACCSET_OBJ_PRIVATE, offsetof(JSObject, privateData));
        break;

      case ACCSET_ITER_PROPS:
        
        
        ok = (op == LIR_ldi || op == LIR_ldp || op == LIR_ldd) &&
             (disp == 0 || disp == 4) &&
             match(base, LIR_ldp, ACCSET_ITER, offsetof(NativeIterator, props_cursor));
        break;

      case ACCSET_STRING:
        
        
        
        
        ok = dispWithin(JSString) &&
             couldBeObjectOrString(base);
        break;

      case ACCSET_STRING_MCHARS:
        
        
        
        
        
        
        ok = op == LIR_ldus2ui &&
             disp == 0 &&
             (match(base, LIR_ldp, ACCSET_STRING, JSString::offsetOfChars()) ||
              (base->isop(LIR_addp) &&
               match(base->oprnd1(), LIR_ldp, ACCSET_STRING, JSString::offsetOfChars())));
        break;

      case ACCSET_TYPEMAP:
        
        
        
        
        
        
        ok = op == LIR_lduc2ui &&
             disp == 0 &&
             base->isop(LIR_addp);
        break;

      case ACCSET_FCSLOTS:
        
        
        
        
        ok = isConstPrivatePtr(base, JSObject::JSSLOT_FLAT_CLOSURE_UPVARS);
        break;

      case ACCSET_ARGS_DATA:
        
        
        
        
        
        
        
        
        ok = (isConstPrivatePtr(base, ArgumentsObject::DATA_SLOT) ||
              (base->isop(LIR_addp) &&
               isConstPrivatePtr(base->oprnd1(), ArgumentsObject::DATA_SLOT)));
        break;

      case ACCSET_SEG:
        
        ok = dispWithin(StackSegment) &&
             match(base, LIR_ldp, ACCSET_CX, offsetof(JSContext, stack) + ContextStack::offsetOfSeg());
        break;

      default:
        
        
        JS_ASSERT(!isSingletonAccSet(accSet));
        ok = true;
        break;
    }

    if (!ok) {
        InsBuf b1, b2;
        printer->formatIns(&b1, base);
        JS_snprintf(b2.buf, b2.len, "base = (%s); disp = %d", b1.buf, disp);
        errorAccSet(lirNames[op], accSet, b2.buf);
    }
}

} 

#endif

