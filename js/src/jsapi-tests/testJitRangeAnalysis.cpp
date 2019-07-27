






#include "jit/IonAnalysis.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/RangeAnalysis.h"

#include "jsapi-tests/testJitMinimalFunc.h"
#include "jsapi-tests/tests.h"

using namespace js;
using namespace js::jit;

static bool
EquivalentRanges(const Range *a, const Range *b) {
    if (a->hasInt32UpperBound() != b->hasInt32UpperBound())
        return false;
    if (a->hasInt32LowerBound() != b->hasInt32LowerBound())
        return false;
    if (a->hasInt32UpperBound() && (a->upper() != b->upper()))
        return false;
    if (a->hasInt32LowerBound() && (a->lower() != b->lower()))
        return false;
    if (a->canHaveFractionalPart() != b->canHaveFractionalPart())
        return false;
    if (a->canBeNegativeZero() != b->canBeNegativeZero())
        return false;
    if (a->canBeNaN() != b->canBeNaN())
        return false;
    if (a->canBeInfiniteOrNaN() != b->canBeInfiniteOrNaN())
        return false;
    if (!a->canBeInfiniteOrNaN() && (a->exponent() != b->exponent()))
        return false;
    return true;
}

BEGIN_TEST(testJitRangeAnalysis_MathSign)
{
    MinimalAlloc func;

    Range *xnan = new(func.alloc) Range();

    Range *ninf = Range::NewDoubleSingletonRange(func.alloc, mozilla::NegativeInfinity<double>());
    Range *n1_5 = Range::NewDoubleSingletonRange(func.alloc, -1.5);
    Range *n1_0 = Range::NewDoubleSingletonRange(func.alloc, -1);
    Range *n0_5 = Range::NewDoubleSingletonRange(func.alloc, -0.5);
    Range *n0_0 = Range::NewDoubleSingletonRange(func.alloc, -0.0);

    Range *p0_0 = Range::NewDoubleSingletonRange(func.alloc, 0.0);
    Range *p0_5 = Range::NewDoubleSingletonRange(func.alloc, 0.5);
    Range *p1_0 = Range::NewDoubleSingletonRange(func.alloc, 1.0);
    Range *p1_5 = Range::NewDoubleSingletonRange(func.alloc, 1.5);
    Range *pinf = Range::NewDoubleSingletonRange(func.alloc, mozilla::PositiveInfinity<double>());

    Range *xnanSign = Range::sign(func.alloc, xnan);

    Range *ninfSign = Range::sign(func.alloc, ninf);
    Range *n1_5Sign = Range::sign(func.alloc, n1_5);
    Range *n1_0Sign = Range::sign(func.alloc, n1_0);
    Range *n0_5Sign = Range::sign(func.alloc, n0_5);
    Range *n0_0Sign = Range::sign(func.alloc, n0_0);

    Range *p0_0Sign = Range::sign(func.alloc, p0_0);
    Range *p0_5Sign = Range::sign(func.alloc, p0_5);
    Range *p1_0Sign = Range::sign(func.alloc, p1_0);
    Range *p1_5Sign = Range::sign(func.alloc, p1_5);
    Range *pinfSign = Range::sign(func.alloc, pinf);

    CHECK(!xnanSign);
    CHECK(EquivalentRanges(ninfSign, Range::NewInt32SingletonRange(func.alloc, -1)));
    CHECK(EquivalentRanges(n1_5Sign, Range::NewInt32SingletonRange(func.alloc, -1)));
    CHECK(EquivalentRanges(n1_0Sign, Range::NewInt32SingletonRange(func.alloc, -1)));

    
    
    CHECK(EquivalentRanges(n0_5Sign, Range::NewInt32Range(func.alloc, -1, 0)));

    CHECK(EquivalentRanges(n0_0Sign, Range::NewDoubleSingletonRange(func.alloc, -0.0)));

    CHECK(!n0_0Sign->canHaveFractionalPart());
    CHECK(n0_0Sign->canBeNegativeZero());
    CHECK(n0_0Sign->lower() == 0);
    CHECK(n0_0Sign->upper() == 0);

    CHECK(EquivalentRanges(p0_0Sign, Range::NewInt32SingletonRange(func.alloc, 0)));

    CHECK(!p0_0Sign->canHaveFractionalPart());
    CHECK(!p0_0Sign->canBeNegativeZero());
    CHECK(p0_0Sign->lower() == 0);
    CHECK(p0_0Sign->upper() == 0);

    
    
    CHECK(EquivalentRanges(p0_5Sign, Range::NewInt32Range(func.alloc, 0, 1)));

    CHECK(EquivalentRanges(p1_0Sign, Range::NewInt32SingletonRange(func.alloc, 1)));
    CHECK(EquivalentRanges(p1_5Sign, Range::NewInt32SingletonRange(func.alloc, 1)));
    CHECK(EquivalentRanges(pinfSign, Range::NewInt32SingletonRange(func.alloc, 1)));

    return true;
}
END_TEST(testJitRangeAnalysis_MathSign)

BEGIN_TEST(testJitRangeAnalysis_MathSignBeta)
{
    MinimalFunc func;
    MathCache cache;

    MBasicBlock *entry = func.createEntryBlock();
    MBasicBlock *thenBlock = func.createBlock(entry);
    MBasicBlock *elseBlock = func.createBlock(entry);
    MBasicBlock *elseThenBlock = func.createBlock(elseBlock);
    MBasicBlock *elseElseBlock = func.createBlock(elseBlock);

    
    MParameter *p = func.createParameter();
    entry->add(p);
    MConstant *c0 = MConstant::New(func.alloc, DoubleValue(0.0));
    entry->add(c0);
    MConstant *cm0 = MConstant::New(func.alloc, DoubleValue(-0.0));
    entry->add(cm0);
    MCompare *cmp = MCompare::New(func.alloc, p, c0, JSOP_LT);
    entry->add(cmp);
    entry->end(MTest::New(func.alloc, cmp, thenBlock, elseBlock));

    
    
    
    MAdd *thenAdd = MAdd::NewAsmJS(func.alloc, p, cm0, MIRType_Double);
    thenBlock->add(thenAdd);
    MMathFunction *thenSign = MMathFunction::New(func.alloc, thenAdd, MMathFunction::Sign, &cache);
    thenBlock->add(thenSign);
    MReturn *thenRet = MReturn::New(func.alloc, thenSign);
    thenBlock->end(thenRet);

    
    
    
    MCompare *elseCmp = MCompare::New(func.alloc, p, c0, JSOP_GE);
    elseBlock->add(elseCmp);
    elseBlock->end(MTest::New(func.alloc, elseCmp, elseThenBlock, elseElseBlock));

    
    
    
    MAdd *elseThenAdd = MAdd::NewAsmJS(func.alloc, p, cm0, MIRType_Double);
    elseThenBlock->add(elseThenAdd);
    MMathFunction *elseThenSign = MMathFunction::New(func.alloc, elseThenAdd, MMathFunction::Sign, &cache);
    elseThenBlock->add(elseThenSign);
    MReturn *elseThenRet = MReturn::New(func.alloc, elseThenSign);
    elseThenBlock->end(elseThenRet);

    
    
    
    
    
    MAdd *elseElseAdd = MAdd::NewAsmJS(func.alloc, p, cm0, MIRType_Double);
    elseElseBlock->add(elseElseAdd);
    MMathFunction *elseElseSign = MMathFunction::New(func.alloc, elseElseAdd, MMathFunction::Sign, &cache);
    elseElseBlock->add(elseElseSign);
    MReturn *elseElseRet = MReturn::New(func.alloc, elseElseSign);
    elseElseBlock->end(elseElseRet);

    if (!func.runRangeAnalysis())
        return false;

    CHECK(!p->range());
    CHECK(EquivalentRanges(c0->range(), Range::NewDoubleSingletonRange(func.alloc, 0.0)));
    CHECK(EquivalentRanges(cm0->range(), Range::NewDoubleSingletonRange(func.alloc, -0.0)));

    
    CHECK(EquivalentRanges(thenAdd->range(),
                           new(func.alloc) Range(Range::NoInt32LowerBound, 0,
                                                 Range::IncludesFractionalParts,
                                                 Range::ExcludesNegativeZero,
                                                 Range::IncludesInfinity)));

    
    CHECK(EquivalentRanges(thenSign->range(),
                           new(func.alloc) Range(-1, 0,
                                                 Range::ExcludesFractionalParts,
                                                 Range::ExcludesNegativeZero,
                                                 0)));

    
    CHECK(EquivalentRanges(elseThenAdd->range(),
                           new(func.alloc) Range(0, Range::NoInt32UpperBound,
                                                 Range::IncludesFractionalParts,
                                                 Range::IncludesNegativeZero,
                                                 Range::IncludesInfinity)));

    
    CHECK(EquivalentRanges(elseThenSign->range(),
                           new(func.alloc) Range(0, 1,
                                                 Range::ExcludesFractionalParts,
                                                 Range::IncludesNegativeZero,
                                                 0)));

    
    CHECK(elseElseAdd->range()->isUnknown());
    CHECK(!elseElseSign->range());

    return true;
}
END_TEST(testJitRangeAnalysis_MathSignBeta)
