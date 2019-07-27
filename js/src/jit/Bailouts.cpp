





#include "jit/Bailouts.h"

#include "jscntxt.h"

#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/JitCompartment.h"
#include "jit/JitSpewer.h"
#include "jit/Snapshots.h"
#include "vm/TraceLogging.h"

#include "jit/JitFrameIterator-inl.h"
#include "vm/Probes-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::IsInRange;

















SnapshotIterator::SnapshotIterator(const IonBailoutIterator &iter)
  : snapshot_(iter.ionScript()->snapshots(),
              iter.snapshotOffset(),
              iter.ionScript()->snapshotsRVATableSize(),
              iter.ionScript()->snapshotsListSize()),
    recover_(snapshot_,
             iter.ionScript()->recovers(),
             iter.ionScript()->recoversSize()),
    fp_(iter.jsFrame()),
    machine_(iter.machineState()),
    ionScript_(iter.ionScript()),
    instructionResults_(nullptr)
{
}

void
IonBailoutIterator::dump() const
{
    if (type_ == JitFrame_IonJS) {
        InlineFrameIterator frames(GetJSContextFromJitCode(), this);
        for (;;) {
            frames.dump();
            if (!frames.more())
                break;
            ++frames;
        }
    } else {
        JitFrameIterator::dump();
    }
}

uint32_t
jit::Bailout(BailoutStack *sp, BaselineBailoutInfo **bailoutInfo)
{
    JSContext *cx = GetJSContextFromJitCode();
    JS_ASSERT(bailoutInfo);

    
    MOZ_ASSERT(IsInRange(FAKE_JIT_TOP_FOR_BAILOUT, 0, 0x1000) &&
               IsInRange(FAKE_JIT_TOP_FOR_BAILOUT + sizeof(IonCommonFrameLayout), 0, 0x1000),
               "Fake jitTop pointer should be within the first page.");
    cx->mainThread().jitTop = FAKE_JIT_TOP_FOR_BAILOUT;
    gc::AutoSuppressGC suppress(cx);

    JitActivationIterator jitActivations(cx->runtime());
    IonBailoutIterator iter(jitActivations, sp);
    JitActivation *activation = jitActivations->asJit();

    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogTimestamp(logger, TraceLogger::Bailout);

    JitSpew(JitSpew_IonBailouts, "Took bailout! Snapshot offset: %d", iter.snapshotOffset());

    JS_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = nullptr;
    uint32_t retval = BailoutIonToBaseline(cx, activation, iter, false, bailoutInfo);
    JS_ASSERT(retval == BAILOUT_RETURN_OK ||
              retval == BAILOUT_RETURN_FATAL_ERROR ||
              retval == BAILOUT_RETURN_OVERRECURSED);
    JS_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != nullptr);

    if (retval != BAILOUT_RETURN_OK) {
        
        
        
        
        
        
        
        
        
        
        
        bool popSPSFrame = iter.ionScript()->hasSPSInstrumentation() &&
                           (SnapshotIterator(iter).bailoutKind() != Bailout_ArgumentCheck);
        JSScript *script = iter.script();
        probes::ExitScript(cx, script, script->functionNonDelazifying(), popSPSFrame);

        EnsureExitFrame(iter.jsFrame());
    }

    return retval;
}

uint32_t
jit::InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut,
                         BaselineBailoutInfo **bailoutInfo)
{
    sp->checkInvariants();

    JSContext *cx = GetJSContextFromJitCode();

    
    cx->mainThread().jitTop = FAKE_JIT_TOP_FOR_BAILOUT;
    gc::AutoSuppressGC suppress(cx);

    JitActivationIterator jitActivations(cx->runtime());
    IonBailoutIterator iter(jitActivations, sp);
    JitActivation *activation = jitActivations->asJit();

    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogTimestamp(logger, TraceLogger::Invalidation);

    JitSpew(JitSpew_IonBailouts, "Took invalidation bailout! Snapshot offset: %d", iter.snapshotOffset());

    
    *frameSizeOut = iter.topFrameSize();

    JS_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = nullptr;
    uint32_t retval = BailoutIonToBaseline(cx, activation, iter, true, bailoutInfo);
    JS_ASSERT(retval == BAILOUT_RETURN_OK ||
              retval == BAILOUT_RETURN_FATAL_ERROR ||
              retval == BAILOUT_RETURN_OVERRECURSED);
    JS_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != nullptr);

    if (retval != BAILOUT_RETURN_OK) {
        
        
        
        
        
        
        
        
        
        
        
        bool popSPSFrame = iter.ionScript()->hasSPSInstrumentation() &&
                           (SnapshotIterator(iter).bailoutKind() != Bailout_ArgumentCheck);
        JSScript *script = iter.script();
        probes::ExitScript(cx, script, script->functionNonDelazifying(), popSPSFrame);

        IonJSFrameLayout *frame = iter.jsFrame();
        JitSpew(JitSpew_IonInvalidate, "Bailout failed (%s): converting to exit frame",
                (retval == BAILOUT_RETURN_FATAL_ERROR) ? "Fatal Error" : "Over Recursion");
        JitSpew(JitSpew_IonInvalidate, "   orig calleeToken %p", (void *) frame->calleeToken());
        JitSpew(JitSpew_IonInvalidate, "   orig frameSize %u", unsigned(frame->prevFrameLocalSize()));
        JitSpew(JitSpew_IonInvalidate, "   orig ra %p", (void *) frame->returnAddress());

        frame->replaceCalleeToken(nullptr);
        EnsureExitFrame(frame);

        JitSpew(JitSpew_IonInvalidate, "   new  calleeToken %p", (void *) frame->calleeToken());
        JitSpew(JitSpew_IonInvalidate, "   new  frameSize %u", unsigned(frame->prevFrameLocalSize()));
        JitSpew(JitSpew_IonInvalidate, "   new  ra %p", (void *) frame->returnAddress());
    }

    iter.ionScript()->decref(cx->runtime()->defaultFreeOp());

    return retval;
}

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &activations,
                                       const JitFrameIterator &frame)
  : JitFrameIterator(activations),
    machine_(frame.machineState())
{
    kind_ = Kind_BailoutIterator;
    returnAddressToFp_ = frame.returnAddressToFp();
    topIonScript_ = frame.ionScript();
    const OsiIndex *osiIndex = frame.osiIndex();

    current_ = (uint8_t *) frame.fp();
    type_ = JitFrame_IonJS;
    topFrameSize_ = frame.frameSize();
    snapshotOffset_ = osiIndex->snapshotOffset();
}

uint32_t
jit::ExceptionHandlerBailout(JSContext *cx, const InlineFrameIterator &frame,
                             ResumeFromException *rfe,
                             const ExceptionBailoutInfo &excInfo,
                             bool *overrecursed)
{
    
    
    
    MOZ_ASSERT_IF(!excInfo.propagatingIonExceptionForDebugMode(), cx->isExceptionPending());

    cx->mainThread().jitTop = FAKE_JIT_TOP_FOR_BAILOUT;
    gc::AutoSuppressGC suppress(cx);

    JitActivationIterator jitActivations(cx->runtime());
    IonBailoutIterator iter(jitActivations, frame.frame());
    JitActivation *activation = jitActivations->asJit();

    BaselineBailoutInfo *bailoutInfo = nullptr;
    uint32_t retval = BailoutIonToBaseline(cx, activation, iter, true, &bailoutInfo, &excInfo);

    if (retval == BAILOUT_RETURN_OK) {
        MOZ_ASSERT(bailoutInfo);

        
        
        if (excInfo.propagatingIonExceptionForDebugMode())
            bailoutInfo->bailoutKind = Bailout_IonExceptionDebugMode;

        rfe->kind = ResumeFromException::RESUME_BAILOUT;
        rfe->target = cx->runtime()->jitRuntime()->getBailoutTail()->raw();
        rfe->bailoutInfo = bailoutInfo;
    } else {
        
        
        
        
        MOZ_ASSERT(!bailoutInfo);

        if (!excInfo.propagatingIonExceptionForDebugMode())
            cx->clearPendingException();

        if (retval == BAILOUT_RETURN_OVERRECURSED)
            *overrecursed = true;
        else
            MOZ_ASSERT(retval == BAILOUT_RETURN_FATAL_ERROR);
    }

    return retval;
}


bool
jit::EnsureHasScopeObjects(JSContext *cx, AbstractFramePtr fp)
{
    if (fp.isFunctionFrame() &&
        fp.fun()->isHeavyweight() &&
        !fp.hasCallObj())
    {
        return fp.initFunctionScopeObjects(cx);
    }
    return true;
}

bool
jit::CheckFrequentBailouts(JSContext *cx, JSScript *script)
{
    if (script->hasIonScript()) {
        
        
        IonScript *ionScript = script->ionScript();

        if (ionScript->numBailouts() >= js_JitOptions.frequentBailoutThreshold &&
            !script->hadFrequentBailouts())
        {
            script->setHadFrequentBailouts();

            JitSpew(JitSpew_IonInvalidate, "Invalidating due to too many bailouts");

            if (!Invalidate(cx, script))
                return false;
        }
    }

    return true;
}
