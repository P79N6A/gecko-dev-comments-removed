






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
