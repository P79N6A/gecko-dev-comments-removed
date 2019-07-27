





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

uint32_t
jit::Bailout(BailoutStack *sp, BaselineBailoutInfo **bailoutInfo)
{
    JSContext *cx = GetJSContextFromJitCode();
    MOZ_ASSERT(bailoutInfo);

    
    MOZ_ASSERT(IsInRange(FAKE_JIT_TOP_FOR_BAILOUT, 0, 0x1000) &&
               IsInRange(FAKE_JIT_TOP_FOR_BAILOUT + sizeof(IonCommonFrameLayout), 0, 0x1000),
               "Fake jitTop pointer should be within the first page.");
    cx->mainThread().jitTop = FAKE_JIT_TOP_FOR_BAILOUT;

    JitActivationIterator jitActivations(cx->runtime());
    BailoutFrameInfo bailoutData(jitActivations, sp);
    JitFrameIterator iter(jitActivations);
    MOZ_ASSERT(!iter.ionScript()->invalidated());

    TraceLoggerThread *logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogTimestamp(logger, TraceLogger_Bailout);

    JitSpew(JitSpew_IonBailouts, "Took bailout! Snapshot offset: %d", iter.snapshotOffset());

    MOZ_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = nullptr;
    bool poppedLastSPSFrame = false;
    uint32_t retval = BailoutIonToBaseline(cx, bailoutData.activation(), iter, false, bailoutInfo,
                                            nullptr, &poppedLastSPSFrame);
    MOZ_ASSERT(retval == BAILOUT_RETURN_OK ||
               retval == BAILOUT_RETURN_FATAL_ERROR ||
               retval == BAILOUT_RETURN_OVERRECURSED);
    MOZ_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != nullptr);

    if (retval != BAILOUT_RETURN_OK) {
        
        
        
        
        
        
        
        
        
        
        
        bool popSPSFrame = iter.ionScript()->hasSPSInstrumentation() &&
                           (SnapshotIterator(iter).bailoutKind() != Bailout_ArgumentCheck) &&
                           !poppedLastSPSFrame;
        JSScript *script = iter.script();
        probes::ExitScript(cx, script, script->functionNonDelazifying(), popSPSFrame);

        EnsureExitFrame(iter.jsFrame());
    }

    
    
    
    
    
    
    
    
    if (iter.ionScript()->invalidated())
        iter.ionScript()->decrementInvalidationCount(cx->runtime()->defaultFreeOp());

    return retval;
}

uint32_t
jit::InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut,
                         BaselineBailoutInfo **bailoutInfo)
{
    sp->checkInvariants();

    JSContext *cx = GetJSContextFromJitCode();

    
    cx->mainThread().jitTop = FAKE_JIT_TOP_FOR_BAILOUT;

    JitActivationIterator jitActivations(cx->runtime());
    BailoutFrameInfo bailoutData(jitActivations, sp);
    JitFrameIterator iter(jitActivations);

    TraceLoggerThread *logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogTimestamp(logger, TraceLogger_Invalidation);

    JitSpew(JitSpew_IonBailouts, "Took invalidation bailout! Snapshot offset: %d", iter.snapshotOffset());

    
    *frameSizeOut = iter.frameSize();

    MOZ_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = nullptr;
    bool poppedLastSPSFrame = false;
    uint32_t retval = BailoutIonToBaseline(cx, bailoutData.activation(), iter, true, bailoutInfo,
                                            nullptr, &poppedLastSPSFrame);
    MOZ_ASSERT(retval == BAILOUT_RETURN_OK ||
               retval == BAILOUT_RETURN_FATAL_ERROR ||
               retval == BAILOUT_RETURN_OVERRECURSED);
    MOZ_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != nullptr);

    if (retval != BAILOUT_RETURN_OK) {
        
        
        
        
        
        
        
        
        
        
        
        bool popSPSFrame = iter.ionScript()->hasSPSInstrumentation() &&
                           (SnapshotIterator(iter).bailoutKind() != Bailout_ArgumentCheck) &&
                           !poppedLastSPSFrame;
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

    iter.ionScript()->decrementInvalidationCount(cx->runtime()->defaultFreeOp());

    return retval;
}

BailoutFrameInfo::BailoutFrameInfo(const JitActivationIterator &activations,
                                   const JitFrameIterator &frame)
  : machine_(frame.machineState())
{
    framePointer_ = (uint8_t *) frame.fp();
    topFrameSize_ = frame.frameSize();
    topIonScript_ = frame.ionScript();
    attachOnJitActivation(activations);

    const OsiIndex *osiIndex = frame.osiIndex();
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
    BailoutFrameInfo bailoutData(jitActivations, frame.frame());
    JitFrameIterator iter(jitActivations);

    BaselineBailoutInfo *bailoutInfo = nullptr;
    bool poppedLastSPSFrame = false;
    uint32_t retval = BailoutIonToBaseline(cx, bailoutData.activation(), iter, true,
                                           &bailoutInfo, &excInfo, &poppedLastSPSFrame);

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

void
BailoutFrameInfo::attachOnJitActivation(const JitActivationIterator &jitActivations)
{
    MOZ_ASSERT(jitActivations.jitTop() == FAKE_JIT_TOP_FOR_BAILOUT);
    activation_ = jitActivations->asJit();
    activation_->setBailoutData(this);
}

BailoutFrameInfo::~BailoutFrameInfo()
{
    activation_->cleanBailoutData();
}
