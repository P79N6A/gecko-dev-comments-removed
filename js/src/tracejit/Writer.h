






































#ifndef tracejit_Writer_h___
#define tracejit_Writer_h___

#include "jsiter.h"
#include "jsstr.h"
#include "jstypedarray.h"
#include "nanojit.h"

namespace js {
namespace tjit {

namespace nj = nanojit;

#if defined(DEBUG) && !defined(JS_JIT_SPEW)
#define JS_JIT_SPEW
#endif

#if defined(JS_JIT_SPEW)

enum LC_TMBits {
    



    LC_TMMinimal  = 1<<16,
    LC_TMTracer   = 1<<17,
    LC_TMRecorder = 1<<18,
    LC_TMAbort    = 1<<19,
    LC_TMStats    = 1<<20,
    LC_TMTreeVis  = 1<<21,
    LC_TMProfiler = 1<<22
};

#endif

























































static const nanojit::AccSet ACCSET_STATE         = (1 <<  0);
static const nanojit::AccSet ACCSET_STACK         = (1 <<  1);
static const nanojit::AccSet ACCSET_RSTACK        = (1 <<  2);
static const nanojit::AccSet ACCSET_CX            = (1 <<  3);
static const nanojit::AccSet ACCSET_TM            = (1 <<  4);
static const nanojit::AccSet ACCSET_EOS           = (1 <<  5);
static const nanojit::AccSet ACCSET_ALLOC         = (1 <<  6);
static const nanojit::AccSet ACCSET_FRAMEREGS     = (1 <<  7);
static const nanojit::AccSet ACCSET_STACKFRAME    = (1 <<  8);
static const nanojit::AccSet ACCSET_RUNTIME       = (1 <<  9);


static const nanojit::AccSet ACCSET_OBJ_CLASP     = (1 << 10);
static const nanojit::AccSet ACCSET_OBJ_FLAGS     = (1 << 11);
static const nanojit::AccSet ACCSET_OBJ_SHAPE     = (1 << 12);
static const nanojit::AccSet ACCSET_OBJ_PROTO     = (1 << 13);
static const nanojit::AccSet ACCSET_OBJ_PARENT    = (1 << 14);
static const nanojit::AccSet ACCSET_OBJ_PRIVATE   = (1 << 15);
static const nanojit::AccSet ACCSET_OBJ_CAPACITY  = (1 << 16);
static const nanojit::AccSet ACCSET_OBJ_SLOTS     = (1 << 17);  

static const nanojit::AccSet ACCSET_SLOTS         = (1 << 18);  
static const nanojit::AccSet ACCSET_TARRAY        = (1 << 19);
static const nanojit::AccSet ACCSET_TARRAY_DATA   = (1 << 20);
static const nanojit::AccSet ACCSET_ITER          = (1 << 21);
static const nanojit::AccSet ACCSET_ITER_PROPS    = (1 << 22);
static const nanojit::AccSet ACCSET_STRING        = (1 << 23);
static const nanojit::AccSet ACCSET_STRING_MCHARS = (1 << 24);
static const nanojit::AccSet ACCSET_TYPEMAP       = (1 << 25);
static const nanojit::AccSet ACCSET_FCSLOTS       = (1 << 26);
static const nanojit::AccSet ACCSET_ARGS_DATA     = (1 << 27);
static const nanojit::AccSet ACCSET_SEG           = (1 << 28);

static const uint8_t TM_NUM_USED_ACCS = 29; 






struct Address
{
  friend class Writer;

  private:
    nj::LIns *base;
    int32 offset;
    nj::AccSet accSet;

  protected:
    Address(nj::LIns *base, int32 offset, nj::AccSet accSet)
      : base(base), offset(offset), accSet(accSet) {}

    Address(Address addr, int32 offset)
      : base(addr.base), offset(addr.offset + offset), accSet(addr.accSet) {}

  public:
    Address() {}
};




struct StackAddress : Address 
{
    StackAddress(nj::LIns *base, int32 offset)
      : Address(base, offset, ACCSET_STACK) {}
};

struct CxAddress : Address
{
    CxAddress(nj::LIns *base, int32 offset)
      : Address(base, offset, ACCSET_CX) {}
};
#define CxAddress(fieldname) \
    CxAddress(cx_ins, offsetof(JSContext, fieldname))

struct EosAddress : Address
{
    EosAddress(nj::LIns *base, int32 offset)
      : Address(base, offset, ACCSET_EOS) {}
};

struct AllocSlotsAddress : Address
{
    AllocSlotsAddress(nj::LIns *base, unsigned slot = 0)
      : Address(base, slot * sizeof(Value), ACCSET_ALLOC) {}
};

struct StackFrameAddress : Address
{
    StackFrameAddress(nj::LIns *base, int32 offset)
      : Address(base, offset, ACCSET_STACKFRAME) {}
};

struct FSlotsAddress : Address
{
    FSlotsAddress(nj::LIns *base, unsigned slot)
      : Address(base, JSObject::getFixedSlotOffset(slot), ACCSET_SLOTS) {}
};

struct DSlotsAddress : Address
{
    DSlotsAddress(nj::LIns *base, unsigned slot = 0)
      : Address(base, slot * sizeof(Value), ACCSET_SLOTS) {}
};

struct IterPropsAddress : Address
{
    IterPropsAddress(nj::LIns *base)
      : Address(base, 0, ACCSET_ITER_PROPS) {}
};

struct FCSlotsAddress : Address
{
    FCSlotsAddress(nj::LIns *base, unsigned slot = 0)
      : Address(base, slot * sizeof(Value), ACCSET_FCSLOTS) {}
};

struct ArgsSlotOffsetAddress : Address
{
    ArgsSlotOffsetAddress(nj::LIns *base, unsigned offset = 0)
      : Address(base, offset, ACCSET_ARGS_DATA) {}
};

struct AnyAddress : Address
{
    AnyAddress(nj::LIns *base, int32 offset = 0)
      : Address(base, offset, nj::ACCSET_ALL)
    {
        JS_ASSERT(nj::ACCSET_LOAD_ANY == nj::ACCSET_STORE_ANY &&
                  nj::ACCSET_LOAD_ANY == nj::ACCSET_ALL);
    }
};


struct OffsetAddress : Address
{
    OffsetAddress(Address addr, int32 offset)
      : Address(addr, offset) {}
};

bool IsPromotedInt32(nj::LIns *ins);
bool IsPromotedUint32(nj::LIns *ins);
bool IsPromotedInt32OrUint32(nj::LIns *ins);
nj::LIns *DemoteToInt32(nj::LirWriter *out, nj::LIns *ins);
nj::LIns *DemoteToUint32(nj::LirWriter *out, nj::LIns *ins);


static const size_t sPayloadOffset = offsetof(jsval_layout, s.payload);
#if JS_BITS_PER_WORD == 32
static const size_t sTagOffset = offsetof(jsval_layout, s.tag);
#endif

struct MaybeBranch {
    bool set;
    nj::LIns *br;
    MaybeBranch() : set(false), br(NULL) {}
    MaybeBranch(nj::LIns *ins) : set(true), br(ins) {}
    operator bool() { return set; }
    typedef nj::LIns* LInsp;
    operator LInsp() {
        JS_ASSERT(set);
        return br;
    }
};





































class Writer
{
  private:
    nj::Allocator *alloc;
    nj::LirBuffer *lirbuf;      
    nj::LirWriter *const lir;   
    nj::CseFilter *const cse;   

    nj::LogControl *logc;       
    nj::Config     *njConfig;   

  public:
    Writer(nj::Allocator *alloc, nj::LirBuffer *lirbuf)
      : alloc(alloc), lirbuf(lirbuf), lir(NULL), cse(NULL), logc(NULL), njConfig(NULL) {}

    void init(nj::LogControl *logc, nj::Config *njConfig); 

    nj::LIns *name(nj::LIns *ins, const char *name) const {
#ifdef JS_JIT_SPEW
        
        if (logc->lcbits > 0)
            lirbuf->printer->lirNameMap->addName(ins, name);
#endif
        return ins;
    }

    








    void pauseAddingCSEValues()     { if (cse) cse->suspend(); }
    void resumeAddingCSEValues()    { if (cse) cse->resume();  }

    

    nj::LIns *start() const {
        return lir->ins0(nj::LIR_start);
    }

    nj::LIns *paramp(int32 arg, int32 kind) const {
        return lir->insParam(arg, kind);
    }

    nj::LIns *allocp(int32 size) const {
        return lir->insAlloc(size);
    }

    nj::LIns *livep(nj::LIns *x) const {
        return lir->ins1(nj::LIR_livep, x);
    }

    void comment(const char *str) {
    #ifdef JS_JIT_SPEW
        lir->insComment(str);  
    #endif
    }

    

    nj::LIns *ldStateFieldHelper(nj::LOpcode op, nj::LIns *state, int32 offset) const {
        return lir->insLoad(op, state, offset, ACCSET_STATE);
    }
    #define ldiStateField(fieldname) \
        name(w.ldStateFieldHelper(LIR_ldi, lirbuf->state, offsetof(TracerState, fieldname)), \
             #fieldname)
    #define ldpStateField(fieldname) \
        name(w.ldStateFieldHelper(LIR_ldp, lirbuf->state, offsetof(TracerState, fieldname)), \
             #fieldname)

    nj::LIns *stStateFieldHelper(nj::LIns *value, nj::LIns *state, int32 offset) const {
        return lir->insStore(value, state, offset, ACCSET_STATE);
    }
    #define stStateField(value, fieldname) \
        stStateFieldHelper(value, lirbuf->state, offsetof(TracerState, fieldname))

    nj::LIns *ldpRstack(nj::LIns *rp, int32 offset) const {
        return lir->insLoad(nj::LIR_ldp, rp, offset, ACCSET_RSTACK);
    }

    nj::LIns *stRstack(nj::LIns *value, nj::LIns *rp, int32 offset) const {
        return lir->insStore(value, rp, offset, ACCSET_RSTACK);
    }

    nj::LIns *ldpContextFieldHelper(nj::LIns *cx, int32 offset, nj::LoadQual loadQual) const {
        return lir->insLoad(nj::LIR_ldp, cx, offset, ACCSET_CX, loadQual);
    }
    #define ldpContextField(fieldname) \
        name(w.ldpContextFieldHelper(cx_ins, offsetof(JSContext, fieldname), LOAD_NORMAL), \
             #fieldname)
    #define ldpConstContextField(fieldname) \
        name(w.ldpContextFieldHelper(cx_ins, offsetof(JSContext, fieldname), LOAD_CONST), \
             #fieldname)
    nj::LIns *ldpContextRegs(nj::LIns *cx) const {
        int32 segOff = offsetof(JSContext, stack) + ContextStack::offsetOfSeg();
        nj::LIns *seg = ldpContextFieldHelper(cx, segOff, nj::LOAD_CONST);
        int32 regsOff = StackSegment::offsetOfRegs();
        return name(lir->insLoad(nj::LIR_ldp, seg, regsOff, ACCSET_SEG, nj::LOAD_CONST), "cx->regs()");

    }

    nj::LIns *stContextField(nj::LIns *value, nj::LIns *cx, int32 offset) const {
        return lir->insStore(value, cx, offset, ACCSET_CX);
    }
    #define stContextField(value, fieldname) \
        stContextField((value), cx_ins, offsetof(JSContext, fieldname))

    nj::LIns *stTraceMonitorField(nj::LIns *value, void *dest, const char *destName) const {
        return lir->insStore(value, name(lir->insImmP(dest), destName), 0, ACCSET_TM);
    }
    #define stTraceMonitorField(value, fieldname) \
        stTraceMonitorField(value, &traceMonitor->fieldname, #fieldname)

    nj::LIns *ldiAlloc(nj::LIns *alloc) const {
        return lir->insLoad(nj::LIR_ldi, alloc, 0, ACCSET_ALLOC);
    }

    nj::LIns *ldpAlloc(nj::LIns *alloc) const {
        return lir->insLoad(nj::LIR_ldp, alloc, 0, ACCSET_ALLOC);
    }

    nj::LIns *lddAlloc(nj::LIns *alloc) const {
        return lir->insLoad(nj::LIR_ldd, alloc, 0, ACCSET_ALLOC);
    }

    nj::LIns *stAlloc(nj::LIns *value, nj::LIns *alloc) const {
        return lir->insStore(value, alloc, 0, ACCSET_ALLOC);
    }

    nj::LIns *ldpFrameFp(nj::LIns *regs) const {
        return lir->insLoad(nj::LIR_ldp, regs, FrameRegs::offsetOfFp, ACCSET_FRAMEREGS);
    }

    nj::LIns *ldpStackFrameScopeChain(nj::LIns *frame) const {
        return lir->insLoad(nj::LIR_ldp, frame, StackFrame::offsetOfScopeChain(),
                            ACCSET_STACKFRAME);
    }

    nj::LIns *ldiRuntimeProtoHazardShape(nj::LIns *runtime) const {
        return name(lir->insLoad(nj::LIR_ldi, runtime, offsetof(JSRuntime, protoHazardShape),
                                 ACCSET_RUNTIME),
                    "protoHazardShape");
    }

    nj::LIns *ldpObjClasp(nj::LIns *obj, nj::LoadQual loadQual) const {
        return name(lir->insLoad(nj::LIR_ldp, obj, offsetof(JSObject, clasp), ACCSET_OBJ_CLASP,
                                 loadQual),
                    "clasp");
    }

    nj::LIns *ldiObjFlags(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldi, obj, offsetof(JSObject, flags), ACCSET_OBJ_FLAGS),
                    "flags");
    }

    nj::LIns *ldiObjShape(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldi, obj, offsetof(JSObject, objShape), ACCSET_OBJ_SHAPE),
                    "objShape");
    }

    nj::LIns *ldpObjProto(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldp, obj, offsetof(JSObject, proto), ACCSET_OBJ_PROTO),
                    "proto");
    }

    nj::LIns *ldpObjParent(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldp, obj, offsetof(JSObject, parent), ACCSET_OBJ_PARENT),
                    "parent");
    }

    nj::LIns *ldpObjPrivate(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldp, obj, offsetof(JSObject, privateData),
                                 ACCSET_OBJ_PRIVATE),
                    "private");
    }

    nj::LIns *lduiObjPrivate(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldi, obj, offsetof(JSObject, privateData),
                                 ACCSET_OBJ_PRIVATE),
                    "private_uint32");
    }

    nj::LIns *stuiObjPrivate(nj::LIns *obj, nj::LIns *value) const {
        return name(lir->insStore(nj::LIR_sti, value, obj, offsetof(JSObject, privateData),
                                  ACCSET_OBJ_PRIVATE),
                    "private_uint32");
    }

    nj::LIns *ldiDenseArrayCapacity(nj::LIns *array) const {
        return name(lir->insLoad(nj::LIR_ldi, array, offsetof(JSObject, capacity),
                                 ACCSET_OBJ_CAPACITY),
                    "capacity");
    }

    nj::LIns *ldpObjSlots(nj::LIns *obj) const {
        return name(lir->insLoad(nj::LIR_ldp, obj, offsetof(JSObject, slots), ACCSET_OBJ_SLOTS),
                    "slots");
    }

    nj::LIns *ldiConstTypedArrayLength(nj::LIns *array) const {
        return name(lir->insLoad(nj::LIR_ldi, array, js::TypedArray::lengthOffset(), ACCSET_TARRAY,
                                 nj::LOAD_CONST),
                    "typedArrayLength");
    }

    nj::LIns *ldpConstTypedArrayData(nj::LIns *array) const {
        return name(lir->insLoad(nj::LIR_ldp, array, js::TypedArray::dataOffset(), ACCSET_TARRAY,
                                 nj::LOAD_CONST),
                    "typedElems");
    }

    nj::LIns *ldc2iTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_ldc2i, addp(elems, index), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *lduc2uiTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_lduc2ui, addp(elems, index), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *lds2iTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_lds2i, addp(elems, lshpN(index, 1)), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *ldus2uiTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_ldus2ui, addp(elems, lshpN(index, 1)), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *ldiTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_ldi, addp(elems, lshpN(index, 2)), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *ldf2dTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_ldf2d, addp(elems, lshpN(index, 2)), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *lddTypedArrayElement(nj::LIns *elems, nj::LIns *index) const {
        return lir->insLoad(nj::LIR_ldd, addp(elems, lshpN(index, 3)), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *sti2cTypedArrayElement(nj::LIns *value, nj::LIns *elems, nj::LIns *index) const {
        return lir->insStore(nj::LIR_sti2c, value, addp(elems, index), 0, ACCSET_TARRAY_DATA);
    }

    nj::LIns *sti2sTypedArrayElement(nj::LIns *value, nj::LIns *elems, nj::LIns *index) const {
        return lir->insStore(nj::LIR_sti2s, value, addp(elems, lshpN(index, 1)), 0,
                             ACCSET_TARRAY_DATA);
    }

    nj::LIns *stiTypedArrayElement(nj::LIns *value, nj::LIns *elems, nj::LIns *index) const {
        return lir->insStore(nj::LIR_sti, value, addp(elems, lshpN(index, 2)), 0,
                             ACCSET_TARRAY_DATA);
    }

    nj::LIns *std2fTypedArrayElement(nj::LIns *value, nj::LIns *elems, nj::LIns *index) const {
        return lir->insStore(nj::LIR_std2f, value, addp(elems, lshpN(index, 2)), 0,
                             ACCSET_TARRAY_DATA);
    }

    nj::LIns *stdTypedArrayElement(nj::LIns *value, nj::LIns *elems, nj::LIns *index) const {
        return lir->insStore(nj::LIR_std, value, addp(elems, lshpN(index, 3)), 0,
                             ACCSET_TARRAY_DATA);
    }

    nj::LIns *ldpIterCursor(nj::LIns *iter) const {
        return name(lir->insLoad(nj::LIR_ldp, iter, offsetof(NativeIterator, props_cursor),
                                 ACCSET_ITER),
                    "cursor");
    }

    nj::LIns *ldpIterEnd(nj::LIns *iter) const {
        return name(lir->insLoad(nj::LIR_ldp, iter, offsetof(NativeIterator, props_end),
                                 ACCSET_ITER),
                    "end");
    }

    nj::LIns *stpIterCursor(nj::LIns *cursor, nj::LIns *iter) const {
        return lir->insStore(nj::LIR_stp, cursor, iter, offsetof(NativeIterator, props_cursor),
                             ACCSET_ITER);
    }

    nj::LIns *ldpStringLengthAndFlags(nj::LIns *str) const {
        return name(lir->insLoad(nj::LIR_ldp, str, JSString::offsetOfLengthAndFlags(),
                                 ACCSET_STRING),
                    "lengthAndFlags");
    }

    nj::LIns *ldpStringChars(nj::LIns *str) const {
        return name(lir->insLoad(nj::LIR_ldp, str, JSString::offsetOfChars(), ACCSET_STRING),
                    "chars");
    }

    nj::LIns *lduc2uiConstTypeMapEntry(nj::LIns *typemap, nj::LIns *index) const {
        nj::LIns *entry = addp(typemap, ui2p(muli(index, name(immi(sizeof(JSValueType)),
                                                              "sizeof(JSValueType)"))));
        return lir->insLoad(nj::LIR_lduc2ui, entry, 0, ACCSET_TYPEMAP, nj::LOAD_CONST);
    }

    nj::LIns *ldiVolatile(nj::LIns *base) const {
        return lir->insLoad(nj::LIR_ldi, base, 0, nj::ACCSET_LOAD_ANY, nj::LOAD_VOLATILE);
    }

    nj::LIns *stiVolatile(nj::LIns *value, nj::LIns *base) const {
        return lir->insStore(nj::LIR_sti, value, base, 0, nj::ACCSET_STORE_ANY);
    }

    nj::LIns *ldiVMSideExitFieldHelper(nj::LIns *lr, int32 offset) const {
        return lir->insLoad(nj::LIR_ldi, lr, offset, nj::ACCSET_LOAD_ANY);
    }
    #define ldiVMSideExitField(lr, fieldname) \
        name(w.ldiVMSideExitFieldHelper((lr), offsetof(VMSideExit, fieldname)), #fieldname)

    nj::LIns *ldpGuardRecordExit(nj::LIns *gr) const {
        




        return name(lir->insLoad(nj::LIR_ldp, gr, offsetof(nj::GuardRecord, exit),
                                 nj::ACCSET_LOAD_ANY),
                    "exit");
    }

    nj::LIns *stTprintArg(nj::LIns *insa[], nj::LIns *args, int index) const {
        JS_ASSERT(insa[index]);
        
        return lir->insStore(insa[index], args, sizeof(double) * index, nj::ACCSET_STORE_ANY);
    }

    

#if JS_BITS_PER_WORD == 32
    nj::LIns *ldiValueTag(Address addr) const {
        return name(lir->insLoad(nj::LIR_ldi, addr.base, addr.offset + sTagOffset, addr.accSet),
                    "tag");
    }

    nj::LIns *stiValueTag(nj::LIns *tag, Address addr) const {
        JS_ASSERT(tag->isI());
        return lir->insStore(tag, addr.base, addr.offset + sTagOffset, addr.accSet);
    }

    nj::LIns *ldiValuePayload(Address addr) const {
        return name(lir->insLoad(nj::LIR_ldi, addr.base, addr.offset + sPayloadOffset,
                                 addr.accSet),
                    "payload");
    }

    nj::LIns *stiValuePayload(nj::LIns *payload, Address addr) const {
        JS_ASSERT(payload->isI());
        return lir->insStore(payload, addr.base, addr.offset + sPayloadOffset, addr.accSet);
    }
#endif  

    nj::LIns *ldi(Address addr) const {
        return lir->insLoad(nj::LIR_ldi, addr.base, addr.offset, addr.accSet);
    }

#ifdef NANOJIT_64BIT
    nj::LIns *ldq(Address addr) const {
        return lir->insLoad(nj::LIR_ldq, addr.base, addr.offset, addr.accSet);
    }

    nj::LIns *stq(nj::LIns *value, Address addr) const {
        return lir->insStore(nj::LIR_stq, value, addr.base, addr.offset, addr.accSet);
    }
#endif

    nj::LIns *ldp(Address addr) const {
        return lir->insLoad(nj::LIR_ldp, addr.base, addr.offset, addr.accSet);
    }

    nj::LIns *ldd(Address addr) const {
        return lir->insLoad(nj::LIR_ldd, addr.base, addr.offset, addr.accSet);
    }

    nj::LIns *std(nj::LIns *value, Address addr) const {
        return lir->insStore(nj::LIR_std, value, addr.base, addr.offset, addr.accSet);
    }

    nj::LIns *st(nj::LIns *value, Address addr) const {
        return lir->insStore(value, addr.base, addr.offset, addr.accSet);
    }

    

    nj::LIns *call(const nj::CallInfo *call, nj::LIns *args[]) const {
        return lir->insCall(call, args);
    }

    

    nj::LIns *j(nj::LIns *target) const {
        return lir->insBranch(nj::LIR_j, NULL, target);
    }

    





    MaybeBranch jt(nj::LIns *cond) {
        if (cond->isImmI(1))
            return MaybeBranch();                                   
        return MaybeBranch(lir->insBranch(nj::LIR_jt, cond, NULL)); 
    }

    
    MaybeBranch jf(nj::LIns *cond) {
        if (cond->isImmI(0))
            return MaybeBranch();                                   
        return MaybeBranch(lir->insBranch(nj::LIR_jf, cond, NULL)); 
    }

    




    nj::LIns *jfUnoptimizable(nj::LIns *cond) const {
        JS_ASSERT(!cond->isImmI());
        return lir->insBranch(nj::LIR_jf, cond, NULL);
    }

    
    nj::LIns *jtUnoptimizable(nj::LIns *cond) const {
        JS_ASSERT(!cond->isImmI());
        return lir->insBranch(nj::LIR_jt, cond, NULL);
    }

    nj::LIns *label() const {
        return lir->ins0(nj::LIR_label);
    }

    




    void label(nj::LIns *br) {
        if (br) {
            JS_ASSERT(br->isop(nj::LIR_j) || br->isop(nj::LIR_jt) || br->isop(nj::LIR_jf));
            br->setTarget(label());
        }
    }

    
    void label(nj::LIns *br1, nj::LIns *br2) {
        if (br1 || br2) {
            nj::LIns *label_ = label();
            if (br1) {
                JS_ASSERT(br1->isop(nj::LIR_j) || br1->isop(nj::LIR_jt) || br1->isop(nj::LIR_jf));
                br1->setTarget(label_);
            }
            if (br2) {
                JS_ASSERT(br2->isop(nj::LIR_j) || br2->isop(nj::LIR_jt) || br2->isop(nj::LIR_jf));
                br2->setTarget(label_);
            }
        }
    }

    

    nj::LIns *x(nj::GuardRecord *gr) const {
        return lir->insGuard(nj::LIR_x, NULL, gr);
    }

    nj::LIns *xf(nj::LIns *cond, nj::GuardRecord *gr) const {
        return lir->insGuard(nj::LIR_xf, cond, gr);
    }

    nj::LIns *xt(nj::LIns *cond, nj::GuardRecord *gr) const {
        return lir->insGuard(nj::LIR_xt, cond, gr);
    }

    nj::LIns *xbarrier(nj::GuardRecord *gr) const {
        return lir->insGuard(nj::LIR_xbarrier, NULL, gr);
    }

    

    nj::LIns *immi(int32 i) const {
        return lir->insImmI(i);
    }

    nj::LIns *immiUndefined() const {
        return name(immi(0), "undefined");
    }

    






    #define nameImmi(i)         name(w.immi(i), #i)
    #define nameImmui(ui)       name(w.immi((uint32_t)ui), #ui)

#ifdef NANOJIT_64BIT
    nj::LIns *immq(uint64 q) const {
        return lir->insImmQ(q);
    }

    #define nameImmq(q)         name(w.immq(q), #q)
#endif

    







    nj::LIns *immpNonGC(const void *p) const {
        return lir->insImmP(p);
    }

    nj::LIns *immw(intptr_t i) const {
        return lir->insImmP((void *)i);
    }

    #define nameImmpNonGC(p)    name(w.immpNonGC(p), #p)
    #define nameImmw(ww)        name(w.immpNonGC((void *) (ww)), #ww)

    nj::LIns *immpNull() const {
        return name(immpNonGC(NULL), "NULL");
    }

    #define immpMagicWhy(why)   name(w.immpNonGC((void *)(size_t)(why)), #why)

    nj::LIns *immpMagicNull() const {
        return name(immpNonGC(NULL), "MAGIC_NULL");
    }

    nj::LIns *immd(double d) const {
        return lir->insImmD(d);
    }

    

    nj::LIns *eqi(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_eqi, x, y);
    }

    nj::LIns *eqi0(nj::LIns *x) const {
        return lir->insEqI_0(x);
    }

    nj::LIns *eqiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_eqi, x, imm);
    }

    nj::LIns *lti(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_lti, x, y);
    }

    nj::LIns *ltiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_lti, x, imm);
    }

    nj::LIns *gti(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_gti, x, y);
    }

    nj::LIns *gtiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_gti, x, imm);
    }

    nj::LIns *geiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_gei, x, imm);
    }

    nj::LIns *ltui(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_ltui, x, y);
    }

    nj::LIns *ltuiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_ltui, x, imm);
    }

    nj::LIns *gtui(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_gtui, x, y);
    }

    nj::LIns *leui(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_leui, x, y);
    }

    nj::LIns *geui(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_geui, x, y);
    }

#ifdef NANOJIT_64BIT
    nj::LIns *eqq(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_eqq, x, y);
    }

    nj::LIns *ltuq(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_ltuq, x, y);
    }

    nj::LIns *leuq(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_leuq, x, y);
    }

    nj::LIns *geuq(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_geuq, x, y);
    }
#endif

    nj::LIns *eqp(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_eqp, x, y);
    }

    nj::LIns *eqp0(nj::LIns *x) const {
        return lir->insEqP_0(x);
    }

    nj::LIns *ltp(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_ltp, x, y);
    }

    nj::LIns *ltup(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_ltup, x, y);
    }

    nj::LIns *eqd(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_eqd, x, y);
    }

    nj::LIns *eqd0(nj::LIns *x) const {
        return lir->ins2(nj::LIR_eqd, x, immd(0));
    }

    nj::LIns *ltdN(nj::LIns *x, jsdouble imm) const {
        return lir->ins2(nj::LIR_ltd, x, immd(imm));
    }

    

    nj::LIns *negi(nj::LIns *x) const {
        return lir->ins1(nj::LIR_negi, x);
    }

    nj::LIns *addi(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_addi, x, y);
    }

    nj::LIns *addiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_addi, x, imm);
    }

    nj::LIns *subi(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_subi, x, y);
    }

    nj::LIns *muli(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_muli, x, y);
    }

    nj::LIns *muliN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_muli, x, imm);
    }

#if defined NANOJIT_IA32 || defined NANOJIT_X64
    nj::LIns *divi(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_divi, x, y);
    }

    nj::LIns *modi(nj::LIns *x) const {
        return lir->ins1(nj::LIR_modi, x);
    }
#endif

    nj::LIns *andi(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_andi, x, y);
    }

    nj::LIns *andiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_andi, x, imm);
    }

    nj::LIns *ori(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_ori, x, y);
    }

    nj::LIns *xoriN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_xori, x, imm);
    }

    nj::LIns *lshiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_lshi, x, imm);
    }

    nj::LIns *rshiN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_rshi, x, imm);
    }

#ifdef NANOJIT_64BIT
    nj::LIns *andq(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_andq, x, y);
    }

    nj::LIns *orq(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_orq, x, y);
    }

    nj::LIns *lshqN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_lshq, x, imm);
    }

    nj::LIns *rshuqN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_rshuq, x, imm);
    }
#endif

    nj::LIns *addp(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_addp, x, y);
    }

    nj::LIns *andp(nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(nj::LIR_andp, x, y);
    }

    nj::LIns *lshpN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_lshp, x, imm);
    }

    nj::LIns *rshupN(nj::LIns *x, int32 imm) const {
        return lir->ins2ImmI(nj::LIR_rshup, x, imm);
    }

    nj::LIns *negd(nj::LIns *x) const {
        return lir->ins1(nj::LIR_negd, x);
    }

    nj::LIns *cmovi(nj::LIns *cond, nj::LIns *t, nj::LIns *f) const {
        
        NanoAssert(t->isI() && f->isI());
        return lir->insChoose(cond, t, f, njConfig->use_cmov());
    }

    nj::LIns *cmovp(nj::LIns *cond, nj::LIns *t, nj::LIns *f) const {
        
        NanoAssert(t->isP() && f->isP());
        return lir->insChoose(cond, t, f, njConfig->use_cmov());
    }

    nj::LIns *cmovd(nj::LIns *cond, nj::LIns *t, nj::LIns *f) const {
        
        NanoAssert(t->isD() && f->isD());
        return lir->insChoose(cond, t, f, true);
    }

    

#ifdef NANOJIT_64BIT
    nj::LIns *ui2uq(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_ui2uq, ins);
    }

    nj::LIns *q2i(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_q2i, ins);
    }
#endif

    nj::LIns *i2p(nj::LIns *x) const {
        return lir->insI2P(x);
    }

    nj::LIns *ui2p(nj::LIns *x) const {
        return lir->insUI2P(x);
    }

    nj::LIns *p2i(nj::LIns *x) const {
    #ifdef NANOJIT_64BIT
        return lir->ins1(nj::LIR_q2i, x);
    #else
        return x;
    #endif
    }

    nj::LIns *i2d(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_i2d, ins);
    }

    nj::LIns *ui2d(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_ui2d, ins);
    }

    



    nj::LIns *rawD2i(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_d2i, ins);
    }

#ifdef NANOJIT_64BIT
    nj::LIns *dasq(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_dasq, ins);
    }

    nj::LIns *qasd(nj::LIns *ins) const {
        return lir->ins1(nj::LIR_qasd, ins);
    }
#endif

    nj::LIns *demoteToInt32(nj::LIns *ins) const {
        return DemoteToInt32(lir, ins);
    }

    nj::LIns *demoteToUint32(nj::LIns *ins) const {
        return DemoteToUint32(lir, ins);
    }

    

    nj::LIns *addxovi(nj::LIns *x, nj::LIns *y, nj::GuardRecord *gr) const {
        return lir->insGuardXov(nj::LIR_addxovi, x, y, gr);
    }

    nj::LIns *subxovi(nj::LIns *x, nj::LIns *y, nj::GuardRecord *gr) const {
        return lir->insGuardXov(nj::LIR_subxovi, x, y, gr);
    }

    nj::LIns *mulxovi(nj::LIns *x, nj::LIns *y, nj::GuardRecord *gr) const {
        return lir->insGuardXov(nj::LIR_mulxovi, x, y, gr);
    }

    




    nj::LIns *ins1(nj::LOpcode op, nj::LIns *x) const {
        return lir->ins1(op, x);
    }

    nj::LIns *ins2(nj::LOpcode op, nj::LIns *x, nj::LIns *y) const {
        return lir->ins2(op, x, y);
    }

    

    



    nj::LIns *getObjPrivatizedSlot(nj::LIns *obj, uint32 slot) const {
#if JS_BITS_PER_WORD == 32
        nj::LIns *vaddr_ins = ldpObjSlots(obj);
        return lir->insLoad(nj::LIR_ldi, vaddr_ins,
                            slot * sizeof(Value) + sPayloadOffset, ACCSET_SLOTS, nj::LOAD_CONST);

#elif JS_BITS_PER_WORD == 64
        
        nj::LIns *vaddr_ins = ldpObjSlots(obj);
        nj::LIns *v_ins = lir->insLoad(nj::LIR_ldq, vaddr_ins,
                                       slot * sizeof(Value) + sPayloadOffset,
                                       ACCSET_SLOTS, nj::LOAD_CONST);
        return lshqN(v_ins, 1);
#endif
    }

    nj::LIns *getDslotAddress(nj::LIns *obj, nj::LIns *idx) const {
        JS_ASSERT(sizeof(Value) == 8); 
        nj::LIns *offset = lshpN(ui2p(idx), 3);
        nj::LIns *slots = ldpObjSlots(obj);
        return addp(slots, offset);
    }

    nj::LIns *getStringLength(nj::LIns *str) const {
        return name(rshupN(ldpStringLengthAndFlags(str), JSString::LENGTH_SHIFT),
                    "strLength");
    }

    nj::LIns *getStringChar(nj::LIns *str, nj::LIns *idx) const {
        nj::LIns *chars = ldpStringChars(str);
        return name(lir->insLoad(nj::LIR_ldus2ui, addp(chars, lshpN(idx, 1)), 0,
                                 ACCSET_STRING_MCHARS, nj::LOAD_CONST),
                    "strChar");
    }

    inline nj::LIns *getArgsLength(nj::LIns *args) const;
};

}   
}   

#endif 

