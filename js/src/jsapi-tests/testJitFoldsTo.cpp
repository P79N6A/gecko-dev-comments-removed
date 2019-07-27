






#include "jit/IonAnalysis.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/ValueNumbering.h"

#include "jsapi-tests/testJitMinimalFunc.h"
#include "jsapi-tests/tests.h"

using namespace js;
using namespace js::jit;

BEGIN_TEST(testJitFoldsTo_DivReciprocal)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();

    
    MParameter *p = func.createParameter();
    block->add(p);
    MConstant *c = MConstant::New(func.alloc, DoubleValue(4.0));
    block->add(c);
    MDiv *div = MDiv::New(func.alloc, p, c, MIRType_Double);
    block->add(div);
    MReturn *ret = MReturn::New(func.alloc, div);
    block->end(ret);

    if (!func.runGVN())
        return false;

    
    MDefinition *op = ret->getOperand(0);
    CHECK(op->isMul());
    CHECK(op->getOperand(0) == p);
    CHECK(op->getOperand(1)->isConstant());
    CHECK(op->getOperand(1)->toConstant()->value().toNumber() == 0.25);
    return true;
}
END_TEST(testJitFoldsTo_DivReciprocal)

BEGIN_TEST(testJitFoldsTo_NoDivReciprocal)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();

    
    MParameter *p = func.createParameter();
    block->add(p);
    MConstant *c = MConstant::New(func.alloc, DoubleValue(5.0));
    block->add(c);
    MDiv *div = MDiv::New(func.alloc, p, c, MIRType_Double);
    block->add(div);
    MReturn *ret = MReturn::New(func.alloc, div);
    block->end(ret);

    if (!func.runGVN())
        return false;

    
    MDefinition *op = ret->getOperand(0);
    CHECK(op->isDiv());
    CHECK(op->getOperand(0) == p);
    CHECK(op->getOperand(1) == c);
    return true;
}
END_TEST(testJitFoldsTo_NoDivReciprocal)

BEGIN_TEST(testJitNotNot)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();

    
    MParameter *p = func.createParameter();
    block->add(p);
    MNot *not0 = MNot::New(func.alloc, p);
    block->add(not0);
    MNot *not1 = MNot::New(func.alloc, not0);
    block->add(not1);
    MReturn *ret = MReturn::New(func.alloc, not1);
    block->end(ret);

    if (!func.runGVN())
        return false;

    
    MDefinition *op = ret->getOperand(0);
    CHECK(op->isNot());
    CHECK(op->getOperand(0)->isNot());
    CHECK(op->getOperand(0)->getOperand(0) == p);
    return true;
}
END_TEST(testJitNotNot)

BEGIN_TEST(testJitNotNotNot)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();

    
    MParameter *p = func.createParameter();
    block->add(p);
    MNot *not0 = MNot::New(func.alloc, p);
    block->add(not0);
    MNot *not1 = MNot::New(func.alloc, not0);
    block->add(not1);
    MNot *not2 = MNot::New(func.alloc, not1);
    block->add(not2);
    MReturn *ret = MReturn::New(func.alloc, not2);
    block->end(ret);

    if (!func.runGVN())
        return false;

    
    MDefinition *op = ret->getOperand(0);
    CHECK(op->isNot());
    CHECK(op->getOperand(0) == p);
    return true;
}
END_TEST(testJitNotNotNot)

BEGIN_TEST(testJitNotTest)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();
    MBasicBlock *then = func.createBlock(block);
    MBasicBlock *else_ = func.createBlock(block);
    MBasicBlock *exit = func.createBlock(block);

    
    MParameter *p = func.createParameter();
    block->add(p);
    MNot *not0 = MNot::New(func.alloc, p);
    block->add(not0);
    MTest *test = MTest::New(func.alloc, not0, then, else_);
    block->end(test);

    MNop *anchor = MNop::New(func.alloc);
    anchor->setGuard();
    then->add(anchor);
    then->end(MGoto::New(func.alloc, exit));

    else_->end(MGoto::New(func.alloc, exit));

    MReturn *ret = MReturn::New(func.alloc, p);
    exit->end(ret);

    exit->addPredecessorWithoutPhis(then);

    if (!func.runGVN())
        return false;

    
    test = block->lastIns()->toTest();
    CHECK(test->getOperand(0) == p);
    CHECK(test->getSuccessor(0) == else_);
    CHECK(test->getSuccessor(1) == then);
    return true;
}
END_TEST(testJitNotTest)

BEGIN_TEST(testJitNotNotTest)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();
    MBasicBlock *then = func.createBlock(block);
    MBasicBlock *else_ = func.createBlock(block);
    MBasicBlock *exit = func.createBlock(block);

    
    MParameter *p = func.createParameter();
    block->add(p);
    MNot *not0 = MNot::New(func.alloc, p);
    block->add(not0);
    MNot *not1 = MNot::New(func.alloc, not0);
    block->add(not1);
    MTest *test = MTest::New(func.alloc, not1, then, else_);
    block->end(test);

    MNop *anchor = MNop::New(func.alloc);
    anchor->setGuard();
    then->add(anchor);
    then->end(MGoto::New(func.alloc, exit));

    else_->end(MGoto::New(func.alloc, exit));

    MReturn *ret = MReturn::New(func.alloc, p);
    exit->end(ret);

    exit->addPredecessorWithoutPhis(then);

    if (!func.runGVN())
        return false;

    
    test = block->lastIns()->toTest();
    CHECK(test->getOperand(0) == p);
    CHECK(test->getSuccessor(0) == then);
    CHECK(test->getSuccessor(1) == else_);
    return true;
}
END_TEST(testJitNotNotTest)
