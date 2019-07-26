





#include "ion/Bailouts.h"

#include "jsanalyze.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsinfer.h"

#include "ion/BaselineJIT.h"
#include "ion/Ion.h"
#include "ion/IonCompartment.h"
#include "ion/IonSpewer.h"
#include "ion/SnapshotReader.h"
#include "vm/Interpreter.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::ion;

















SnapshotIterator::SnapshotIterator(const IonBailoutIterator &iter)
  : SnapshotReader(iter.ionScript()->snapshots() + iter.snapshotOffset(),
                   iter.ionScript()->snapshots() + iter.ionScript()->snapshotsSize()),
    fp_(iter.jsFrame()),
    machine_(iter.machineState()),
    ionScript_(iter.ionScript())
{
}

void
IonBailoutIterator::dump() const
{
    if (type_ == IonFrame_OptimizedJS) {
        InlineFrameIterator frames(GetIonContext()->cx, this);
        for (;;) {
            frames.dump();
            if (!frames.more())
                break;
            ++frames;
        }
    } else {
        IonFrameIterator::dump();
    }
}

uint32_t
ion::Bailout(BailoutStack *sp, BaselineBailoutInfo **bailoutInfo)
{
    JS_ASSERT(bailoutInfo);
    JSContext *cx = GetIonContext()->cx;
    
    cx->mainThread().ionTop = NULL;
    JitActivationIterator jitActivations(cx->runtime());
    IonBailoutIterator iter(jitActivations, sp);
    JitActivation *activation = jitActivations.activation()->asJit();

    IonSpew(IonSpew_Bailouts, "Took bailout! Snapshot offset: %d", iter.snapshotOffset());

    JS_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = NULL;
    uint32_t retval = BailoutIonToBaseline(cx, activation, iter, false, bailoutInfo);
    JS_ASSERT(retval == BAILOUT_RETURN_OK ||
              retval == BAILOUT_RETURN_FATAL_ERROR ||
              retval == BAILOUT_RETURN_OVERRECURSED);
    JS_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != NULL);

    if (retval != BAILOUT_RETURN_OK)
        EnsureExitFrame(iter.jsFrame());

    return retval;
}

uint32_t
ion::InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut,
                         BaselineBailoutInfo **bailoutInfo)
{
    sp->checkInvariants();

    JSContext *cx = GetIonContext()->cx;

    
    cx->mainThread().ionTop = NULL;
    JitActivationIterator jitActivations(cx->runtime());
    IonBailoutIterator iter(jitActivations, sp);
    JitActivation *activation = jitActivations.activation()->asJit();

    IonSpew(IonSpew_Bailouts, "Took invalidation bailout! Snapshot offset: %d", iter.snapshotOffset());

    
    *frameSizeOut = iter.topFrameSize();

    JS_ASSERT(IsBaselineEnabled(cx));

    *bailoutInfo = NULL;
    uint32_t retval = BailoutIonToBaseline(cx, activation, iter, true, bailoutInfo);
    JS_ASSERT(retval == BAILOUT_RETURN_OK ||
              retval == BAILOUT_RETURN_FATAL_ERROR ||
              retval == BAILOUT_RETURN_OVERRECURSED);
    JS_ASSERT_IF(retval == BAILOUT_RETURN_OK, *bailoutInfo != NULL);

    if (retval != BAILOUT_RETURN_OK) {
        IonJSFrameLayout *frame = iter.jsFrame();
        IonSpew(IonSpew_Invalidate, "converting to exit frame");
        IonSpew(IonSpew_Invalidate, "   orig calleeToken %p", (void *) frame->calleeToken());
        IonSpew(IonSpew_Invalidate, "   orig frameSize %u", unsigned(frame->prevFrameLocalSize()));
        IonSpew(IonSpew_Invalidate, "   orig ra %p", (void *) frame->returnAddress());

        frame->replaceCalleeToken(NULL);
        EnsureExitFrame(frame);

        IonSpew(IonSpew_Invalidate, "   new  calleeToken %p", (void *) frame->calleeToken());
        IonSpew(IonSpew_Invalidate, "   new  frameSize %u", unsigned(frame->prevFrameLocalSize()));
        IonSpew(IonSpew_Invalidate, "   new  ra %p", (void *) frame->returnAddress());
    }

    iter.ionScript()->decref(cx->runtime()->defaultFreeOp());

    return retval;
}

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &activations,
                                       const IonFrameIterator &frame)
  : IonFrameIterator(activations),
    machine_(frame.machineState())
{
    returnAddressToFp_ = frame.returnAddressToFp();
    topIonScript_ = frame.ionScript();
    const OsiIndex *osiIndex = frame.osiIndex();

    current_ = (uint8_t *) frame.fp();
    type_ = IonFrame_OptimizedJS;
    topFrameSize_ = frame.frameSize();
    snapshotOffset_ = osiIndex->snapshotOffset();
}

uint32_t
ion::ExceptionHandlerBailout(JSContext *cx, const InlineFrameIterator &frame,
                             const ExceptionBailoutInfo &excInfo,
                             BaselineBailoutInfo **bailoutInfo)
{
    JS_ASSERT(cx->isExceptionPending());

    cx->mainThread().ionTop = NULL;
    JitActivationIterator jitActivations(cx->runtime());
    IonBailoutIterator iter(jitActivations, frame.frame());
    JitActivation *activation = jitActivations.activation()->asJit();

    *bailoutInfo = NULL;
    uint32_t retval = BailoutIonToBaseline(cx, activation, iter, true, bailoutInfo, &excInfo);
    JS_ASSERT(retval == BAILOUT_RETURN_OK ||
              retval == BAILOUT_RETURN_FATAL_ERROR ||
              retval == BAILOUT_RETURN_OVERRECURSED);

    JS_ASSERT((retval == BAILOUT_RETURN_OK) == (*bailoutInfo != NULL));

    return retval;
}


bool
ion::EnsureHasScopeObjects(JSContext *cx, AbstractFramePtr fp)
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
ion::CheckFrequentBailouts(JSContext *cx, JSScript *script)
{
    if (script->hasIonScript()) {
        
        
        IonScript *ionScript = script->ionScript();

        if (ionScript->numBailouts() >= js_IonOptions.frequentBailoutThreshold &&
            !script->hadFrequentBailouts)
        {
            script->hadFrequentBailouts = true;

            IonSpew(IonSpew_Invalidate, "Invalidating due to too many bailouts");

            if (!Invalidate(cx, script))
                return false;
        } else {
            
            
            if (ionScript->numExceptionBailouts() >= js_IonOptions.exceptionBailoutThreshold)
                ForbidCompilation(cx, script);
        }
    }

    return true;
}
