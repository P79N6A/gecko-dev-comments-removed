






#include "ion/BaselineJIT.h"
#include "ion/BaselineIC.h"
#include "ion/BaselineCompiler.h"
#include "ion/IonLinker.h"

using namespace js;
using namespace js::ion;

bool
BinaryOpCache::generateInt32(JSContext *cx, MacroAssembler &masm)
{
    Label notInt32, overflow;
    masm.branchTestInt32(Assembler::NotEqual, R0, &notInt32);
    masm.branchTestInt32(Assembler::NotEqual, R1, &notInt32);

    masm.addl(R1.payloadReg(), R0.payloadReg());
    masm.j(Assembler::Overflow, &overflow);

    masm.ret();

    
    masm.bind(&overflow);
    

    
    masm.bind(&notInt32);
    generateUpdate(cx, masm);
    return true;
}

bool
CompareCache::generateInt32(JSContext *cx, MacroAssembler &masm)
{
    Label notInt32;
    masm.branchTestInt32(Assembler::NotEqual, R0, &notInt32);
    masm.branchTestInt32(Assembler::NotEqual, R1, &notInt32);

    masm.cmpl(R0.payloadReg(), R1.payloadReg());

    switch (JSOp(*data.pc)) {
      case JSOP_LT:
        masm.setCC(Assembler::LessThan, R0.payloadReg());
        break;

      default:
        JS_NOT_REACHED("Unexpected compare op");
        break;
    }

    masm.movzxbl(R0.payloadReg(), R0.payloadReg());
    masm.movl(ImmType(JSVAL_TYPE_BOOLEAN), R0.typeReg());

    masm.ret();

    
    masm.bind(&notInt32);
    generateUpdate(cx, masm);
    return true;
}
