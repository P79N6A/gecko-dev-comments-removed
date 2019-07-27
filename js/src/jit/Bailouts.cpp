





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
jit::Bailout(BailoutStack* sp, BaselineBailoutInfo** bailoutInfo)
{
    JSContext* cx = GetJSContextFromJitCode();
    MOZ_ASSERT(bailoutInfo);

    
    MOZ_ASSERT(IsInRange(FAKE_JIT_TOP_FOR_BAILOUT, 0, 0x1000) &&
               IsInRange(FAKE_JIT_TOP_FOR_BAILOUT + sizeof(CommonFrameLayout), 0, 0x1000),
               "Fake jitTop pointer should be within the first page.");
    cx->runtime()->jitTop = FAKE_JIT_TOP_FOR_BAILOUT;

    JitActivationIterator jitActivations(cx->runtime());
    BailoutFrameInfo bailoutData(jitActivations, sp);
    JitFrameIterator iter(jitActivations);
    MOZ_ASSERT(!iter.ionScript()->invalidated());
    CommonFrameLayout* currentFramePtr = iter.current();

    TraceLoggerThread* logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogTimestamp(logger, TraceLogger_Bailout);

    JitSpew(JitSpew_IonBailouts, "Took bailout! Snapshot offset: %d", iter.snapshotOffset());

    MOZ_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = nullptr;
    uint32_t retval = BailoutIonToBaseline(cx, bailoutData.activation(), iter, false, bailoutInfo,
                                            nullptr);
    MOZ_ASSERT(retval == BAILOUT_RETURN_OK ||
               retval == BAILOUT_RETURN_FATAL_ERROR ||
               retval == BAILOUT_RETURN_OVERRECURSED);
    MOZ_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != nullptr);

    if (retval != BAILOUT_RETURN_OK) {
        JSScript* script = iter.script();
        probes::ExitScript(cx, script, script->functionNonDelazifying(),
                            false);

        EnsureExitFrame(iter.jsFrame());
    }

    
    
    
    
    
    
    
    
    if (iter.ionScript()->invalidated())
        iter.ionScript()->decrementInvalidationCount(cx->runtime()->defaultFreeOp());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (cx->runtime()->jitRuntime()->isProfilerInstrumentationEnabled(cx->runtime()))
        cx->runtime()->jitActivation->setLastProfilingFrame(currentFramePtr);

    return retval;
}

uint32_t
jit::InvalidationBailout(InvalidationBailoutStack* sp, size_t* frameSizeOut,
                         BaselineBailoutInfo** bailoutInfo)
{
    sp->checkInvariants();

    JSContext* cx = GetJSContextFromJitCode();

    
    cx->runtime()->jitTop = FAKE_JIT_TOP_FOR_BAILOUT;

    JitActivationIterator jitActivations(cx->runtime());
    BailoutFrameInfo bailoutData(jitActivations, sp);
    JitFrameIterator iter(jitActivations);
    CommonFrameLayout* currentFramePtr = iter.current();

    TraceLoggerThread* logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogTimestamp(logger, TraceLogger_Invalidation);

    JitSpew(JitSpew_IonBailouts, "Took invalidation bailout! Snapshot offset: %d", iter.snapshotOffset());

    
    *frameSizeOut = iter.frameSize();

    MOZ_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = nullptr;
    uint32_t retval = BailoutIonToBaseline(cx, bailoutData.activation(), iter, true, bailoutInfo,
                                            nullptr);
    MOZ_ASSERT(retval == BAILOUT_RETURN_OK ||
               retval == BAILOUT_RETURN_FATAL_ERROR ||
               retval == BAILOUT_RETURN_OVERRECURSED);
    MOZ_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != nullptr);

    if (retval != BAILOUT_RETURN_OK) {
        
        
        
        
        
        
        
        
        
        
        
        JSScript* script = iter.script();
        probes::ExitScript(cx, script, script->functionNonDelazifying(),
                            false);

        JitFrameLayout* frame = iter.jsFrame();
        JitSpew(JitSpew_IonInvalidate, "Bailout failed (%s): converting to exit frame",
                (retval == BAILOUT_RETURN_FATAL_ERROR) ? "Fatal Error" : "Over Recursion");
        JitSpew(JitSpew_IonInvalidate, "   orig calleeToken %p", (void*) frame->calleeToken());
        JitSpew(JitSpew_IonInvalidate, "   orig frameSize %u", unsigned(frame->prevFrameLocalSize()));
        JitSpew(JitSpew_IonInvalidate, "   orig ra %p", (void*) frame->returnAddress());

        frame->replaceCalleeToken(nullptr);
        EnsureExitFrame(frame);

        JitSpew(JitSpew_IonInvalidate, "   new  calleeToken %p", (void*) frame->calleeToken());
        JitSpew(JitSpew_IonInvalidate, "   new  frameSize %u", unsigned(frame->prevFrameLocalSize()));
        JitSpew(JitSpew_IonInvalidate, "   new  ra %p", (void*) frame->returnAddress());
    }

    iter.ionScript()->decrementInvalidationCount(cx->runtime()->defaultFreeOp());

    
    if (cx->runtime()->jitRuntime()->isProfilerInstrumentationEnabled(cx->runtime()))
        cx->runtime()->jitActivation->setLastProfilingFrame(currentFramePtr);

    return retval;
}

BailoutFrameInfo::BailoutFrameInfo(const JitActivationIterator& activations,
                                   const JitFrameIterator& frame)
  : machine_(frame.machineState())
{
    framePointer_ = (uint8_t*) frame.fp();
    topFrameSize_ = frame.frameSize();
    topIonScript_ = frame.ionScript();
    attachOnJitActivation(activations);

    const OsiIndex* osiIndex = frame.osiIndex();
    snapshotOffset_ = osiIndex->snapshotOffset();
}

uint32_t
jit::ExceptionHandlerBailout(JSContext* cx, const InlineFrameIterator& frame,
                             ResumeFromException* rfe,
                             const ExceptionBailoutInfo& excInfo,
                             bool* overrecursed)
{
    
    
    
    MOZ_ASSERT_IF(!excInfo.propagatingIonExceptionForDebugMode(), cx->isExceptionPending());

    cx->runtime()->jitTop = FAKE_JIT_TOP_FOR_BAILOUT;
    gc::AutoSuppressGC suppress(cx);

    JitActivationIterator jitActivations(cx->runtime());
    BailoutFrameInfo bailoutData(jitActivations, frame.frame());
    JitFrameIterator iter(jitActivations);
    CommonFrameLayout* currentFramePtr = iter.current();

    BaselineBailoutInfo* bailoutInfo = nullptr;
    uint32_t retval = BailoutIonToBaseline(cx, bailoutData.activation(), iter, true,
                                           &bailoutInfo, &excInfo);

    if (retval == BAILOUT_RETURN_OK) {
        MOZ_ASSERT(bailoutInfo);

        
        
        if (excInfo.propagatingIonExceptionForDebugMode())
            bailoutInfo->bailoutKind = Bailout_IonExceptionDebugMode;

        rfe->kind = ResumeFromException::RESUME_BAILOUT;
        rfe->target = cx->runtime()->jitRuntime()->getBailoutTail()->raw();
        rfe->bailoutInfo = bailoutInfo;
    } else {
        
        
        
        MOZ_ASSERT(!bailoutInfo);

        if (retval == BAILOUT_RETURN_OVERRECURSED) {
            *overrecursed = true;
            if (!excInfo.propagatingIonExceptionForDebugMode())
                cx->clearPendingException();
        } else {
            MOZ_ASSERT(retval == BAILOUT_RETURN_FATAL_ERROR);
        }
    }

    
    if (cx->runtime()->jitRuntime()->isProfilerInstrumentationEnabled(cx->runtime()))
        cx->runtime()->jitActivation->setLastProfilingFrame(currentFramePtr);

    return retval;
}


bool
jit::EnsureHasScopeObjects(JSContext* cx, AbstractFramePtr fp)
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
jit::CheckFrequentBailouts(JSContext* cx, JSScript* script)
{
    if (script->hasIonScript()) {
        
        
        IonScript* ionScript = script->ionScript();

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
BailoutFrameInfo::attachOnJitActivation(const JitActivationIterator& jitActivations)
{
    MOZ_ASSERT(jitActivations.jitTop() == FAKE_JIT_TOP_FOR_BAILOUT);
    activation_ = jitActivations->asJit();
    activation_->setBailoutData(this);
}

BailoutFrameInfo::~BailoutFrameInfo()
{
    activation_->cleanBailoutData();
}
