





































#include "PolyIC.h"
#include "StubCalls.h"
#include "CodeGenIncludes.h"
#include "StubCalls-inl.h"
#include "assembler/assembler/LinkBuffer.h"
#include "jsscope.h"
#include "jsnum.h"
#include "jsscopeinlines.h"

using namespace js;
using namespace js::mjit;

#if ENABLE_PIC


static const uint32 INLINE_PATH_LENGTH = 64;


static const uint32 MAX_STUBS = 16;

typedef JSC::FunctionPtr FunctionPtr;
typedef JSC::RepatchBuffer RepatchBuffer;
typedef JSC::CodeBlock CodeBlock;
typedef JSC::CodeLocationLabel CodeLocationLabel;
typedef JSC::JITCode JITCode;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::Label Label;
typedef JSC::MacroAssembler::Imm32 Imm32;
typedef JSC::MacroAssembler::Address Address;
typedef JSC::ReturnAddressPtr ReturnAddressPtr;
typedef JSC::MacroAssemblerCodePtr MacroAssemblerCodePtr;

struct AutoPropertyDropper
{
    JSContext *cx;
    JSObject *holder;
    JSProperty *prop;

  public:
    AutoPropertyDropper(JSContext *cx, JSObject *obj, JSProperty *prop)
      : cx(cx), holder(obj), prop(prop)
    {
        JS_ASSERT(prop);
    }

    ~AutoPropertyDropper()
    {
        holder->dropProperty(cx, prop);
    }
};

class PICStubCompiler
{
    const char *type;

  protected:
    VMFrame &f;
    JSScript *script;
    ic::PICInfo &pic;

  public:
    PICStubCompiler(const char *type, VMFrame &f, JSScript *script, ic::PICInfo &pic)
      : type(type), f(f), script(script), pic(pic)
    { }

    bool disable(const char *reason, VoidStub stub)
    {
        return disable(reason, JS_FUNC_TO_DATA_PTR(void *, stub));
    }

    bool disable(const char *reason, void *stub)
    {
        spew("disabled", reason);
        JITCode jitCode(pic.slowPathStart.executableAddress(), INLINE_PATH_LENGTH);
        CodeBlock codeBlock(jitCode);
        RepatchBuffer repatcher(&codeBlock);
        ReturnAddressPtr retPtr(pic.slowPathStart.callAtOffset(pic.callReturn).executableAddress());
        MacroAssemblerCodePtr target(stub);
        repatcher.relinkCallerToTrampoline(retPtr, target);
        return true;
    }

    JSC::ExecutablePool *getExecPool(size_t size)
    {
        mjit::ThreadData *jd = &JS_METHODJIT_DATA(f.cx);
        return jd->execPool->poolForSize(size);
    }

  protected:
    void spew(const char *event, const char *op)
    {
        JaegerSpew(JSpew_PICs, "%s %s: %s (%s: %d)\n",
                   type, event, op, script->filename,
                   js_FramePCToLineNumber(f.cx, f.fp));
    }
};

class PICRepatchBuffer : public JSC::RepatchBuffer
{
    ic::PICInfo &pic;

  public:
    PICRepatchBuffer(ic::PICInfo &ic)
      : JSC::RepatchBuffer(ic.lastPathStart().executableAddress(),
                           INLINE_PATH_LENGTH),
        pic(ic)
    { }

    void relink(int32 offset, JSC::CodeLocationLabel target) {
        JSC::RepatchBuffer::relink(pic.lastPathStart().jumpAtOffset(offset), target);
    }
};

class GetPropCompiler : public PICStubCompiler
{
    JSObject *obj;
    JSAtom *atom;
    VoidStub stub;

    
#ifdef JS_CPU_X86
    static const int32 DSLOTS_LOAD  = -15;
    static const int32 TYPE_LOAD    = -6;
    static const int32 DATA_LOAD    = 0;
    static const int32 INLINE_SHAPE_OFFSET = 6;
    static const int32 INLINE_SHAPE_JUMP   = 12;
    static const int32 STUB_SHAPE_JUMP = 12;
#endif

  public:
    GetPropCompiler(VMFrame &f, JSScript *script, JSObject *obj, ic::PICInfo &pic, JSAtom *atom,
                    VoidStub stub)
      : PICStubCompiler("getprop", f, script, pic), obj(obj), atom(atom), stub(stub)
    { }

    static void reset(ic::PICInfo &pic)
    {
        RepatchBuffer repatcher(pic.fastPathStart.executableAddress(), INLINE_PATH_LENGTH);
        repatcher.repatchLEAToLoadPtr(pic.storeBack.instructionAtOffset(DSLOTS_LOAD));
        repatcher.repatch(pic.fastPathStart.dataLabel32AtOffset(pic.shapeGuard + INLINE_SHAPE_OFFSET),
                          int32(JSScope::INVALID_SHAPE));
        repatcher.relink(pic.fastPathStart.jumpAtOffset(pic.shapeGuard + INLINE_SHAPE_JUMP),
                         pic.slowPathStart);
    }

    bool patchInline(JSObject *holder, JSScopeProperty *sprop)
    {
        spew("patch", "inline");
        PICRepatchBuffer repatcher(pic);

        mjit::ThreadData &jm = JS_METHODJIT_DATA(f.cx);
        if (!jm.addScript(script)) {
            js_ReportOutOfMemory(f.cx);
            return false;
        }

        int32 offset;
        if (sprop->slot < JS_INITIAL_NSLOTS) {
            JSC::CodeLocationInstruction istr;
            istr = pic.storeBack.instructionAtOffset(DSLOTS_LOAD);
            repatcher.repatchLoadPtrToLEA(istr);

            
            
            
            
            
            
            
            int32 diff = int32(offsetof(JSObject, fslots)) -
                         int32(offsetof(JSObject, dslots));
            JS_ASSERT(diff != 0);
            offset  = (int32(sprop->slot) * sizeof(Value)) + diff;
        } else {
            offset = (sprop->slot - JS_INITIAL_NSLOTS) * sizeof(Value);
        }

        uint32 shapeOffs = pic.shapeGuard + INLINE_SHAPE_OFFSET;
        repatcher.repatch(pic.fastPathStart.dataLabel32AtOffset(shapeOffs),
                          obj->shape());
        repatcher.repatch(pic.storeBack.dataLabel32AtOffset(TYPE_LOAD),
                          offset + 4);
        repatcher.repatch(pic.storeBack.dataLabel32AtOffset(DATA_LOAD),
                          offset);

        pic.inlinePathPatched = true;

        return true;
    }

    bool generateStub(JSObject *holder, JSScopeProperty *sprop)
    {
        Vector<Jump, 8> shapeMismatches(f.cx);

        int lastStubSecondShapeGuard = pic.secondShapeGuard;

        Assembler masm;

        if (pic.objNeedsRemat) {
            if (pic.objRemat >= sizeof(JSStackFrame)) {
                masm.loadData32(Address(JSFrameReg, pic.objRemat), pic.objReg);
            } else {
                masm.move(RegisterID(pic.objRemat), pic.objReg);
            }
            pic.objNeedsRemat = false;
        }
        if (!pic.shapeRegHasBaseShape) {
            masm.loadShape(pic.objReg, pic.shapeReg);
            pic.shapeRegHasBaseShape = true;
        }

        Label start = masm.label();
        Jump shapeGuard = masm.branch32_force32(Assembler::NotEqual, pic.shapeReg,
                                                Imm32(obj->shape()));
        if (!shapeMismatches.append(shapeGuard))
            return false;

        if (obj != holder) {
            
            JSObject *tempObj = obj;
            Address fslot(pic.objReg, offsetof(JSObject, fslots) + JSSLOT_PROTO * sizeof(Value));
            do {
                tempObj = tempObj->getProto();
                JS_ASSERT(tempObj);
                JS_ASSERT(tempObj->isNative());

                masm.loadData32(fslot, pic.objReg);
                pic.shapeRegHasBaseShape = false;
                pic.objNeedsRemat = true;

                Jump j = masm.branchTestPtr(Assembler::Zero, pic.objReg, pic.objReg);
                if (!shapeMismatches.append(j))
                    return false;
            } while (tempObj != holder);

            
            masm.loadShape(pic.objReg, pic.shapeReg);
            Jump j = masm.branch32_force32(Assembler::NotEqual, pic.shapeReg,
                                           Imm32(holder->shape()));
            if (!shapeMismatches.append(j))
                return false;
            pic.secondShapeGuard = masm.distanceOf(masm.label()) - masm.distanceOf(start);
        } else {
            pic.secondShapeGuard = 0;
        }
        masm.loadSlot(pic.objReg, pic.objReg, sprop->slot, pic.shapeReg, pic.objReg);
        Jump done = masm.jump();

        JSC::ExecutablePool *ep = getExecPool(masm.size());
        if (!ep) {
            js_ReportOutOfMemory(f.cx);
            return false;
        }

        
        JSC::LinkBuffer buffer(&masm, ep);

        if (!pic.execPools.append(ep)) {
            ep->release();
            js_ReportOutOfMemory(f.cx);
            return false;
        }

        
        for (Jump *pj = shapeMismatches.begin(); pj != shapeMismatches.end(); ++pj)
            buffer.link(*pj, pic.slowPathStart);

        
        buffer.link(done, pic.storeBack);
        CodeLocationLabel cs = buffer.finalizeCodeAddendum();
        JaegerSpew(JSpew_PICs, "generated getprop stub at %p\n", cs.executableAddress());

        
        
        
        PICRepatchBuffer repatcher(pic);
        int shapeGuardJumpOffset;
        if (pic.stubsGenerated)
            shapeGuardJumpOffset = STUB_SHAPE_JUMP;
        else
            shapeGuardJumpOffset = pic.shapeGuard + INLINE_SHAPE_JUMP;
        repatcher.relink(shapeGuardJumpOffset, cs);
        if (lastStubSecondShapeGuard)
            repatcher.relink(lastStubSecondShapeGuard, cs);

        pic.stubsGenerated++;
        pic.lastStubStart = buffer.locationOf(start);

        if (pic.stubsGenerated == MAX_STUBS)
            disable("max stubs reached");

        return true;
    }

    bool update()
    {
        if (!pic.hit) {
            spew("first hit", "nop");
            pic.hit = true;
            return true;
        }

        JSObject *aobj = js_GetProtoIfDenseArray(obj);
        if (!aobj->isNative())
            return disable("non-native");

        JSObject *holder;
        JSProperty *prop;
        if (!aobj->lookupProperty(f.cx, ATOM_TO_JSID(atom), &holder, &prop))
            return false;

        if (!prop)
            return disable("lookup failed");

        AutoPropertyDropper dropper(f.cx, holder, prop);

        JSScopeProperty *sprop = (JSScopeProperty *)prop;
        if (!sprop->hasDefaultGetterOrIsMethod())
            return disable("getter");
        if (!SPROP_HAS_VALID_SLOT(sprop, holder->scope()))
            return disable("invalid slot");

        if (obj == holder && !pic.inlinePathPatched) {
            
            
            
            
            
            
            if (pic.stubsGenerated == 0)
                return patchInline(holder, sprop);
        } else {
            JS_ASSERT(pic.stubsGenerated < ic::MAX_PIC_STUBS);
            return generateStub(holder, sprop);
        }

        return true;
    }

    bool disable(const char *reason)
    {
        return PICStubCompiler::disable(reason, stub);
    }
};

void JS_FASTCALL
ic::GetProp(VMFrame &f, uint32 index)
{
    JSScript *script = f.fp->script;
    PICInfo &pic = script->pics[index];

    JSAtom *atom;
    atom = script->getAtom(pic.atomIndex);

    JSObject *obj = ValueToObject(f.cx, &f.regs.sp[-1]);
    if (!obj)
        THROW();

    if (pic.shouldGenerate()) {
        GetPropCompiler cc(f, script, obj, pic, atom, stubs::GetProp);
        if (!cc.update()) {
            cc.disable("error");
            THROW();
        }
    }

    Value v;
    if (!obj->getProperty(f.cx, ATOM_TO_JSID(atom), &v))
        THROW();
    f.regs.sp[-1] = v;
}

#endif

void
ic::PurgePICs(JSContext *cx, JSScript *script)
{
    uint32 npics = script->numPICs();
    for (uint32 i = 0; i < npics; i++) {
        ic::PICInfo &pic = script->pics[i];
        if (pic.kind == ic::PICInfo::GET)
            GetPropCompiler::reset(pic);
        pic.reset();
    }
}

