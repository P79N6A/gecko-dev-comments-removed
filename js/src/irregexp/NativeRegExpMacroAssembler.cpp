





























#include "irregexp/NativeRegExpMacroAssembler.h"

#include "irregexp/RegExpStack.h"
#include "jit/Linker.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "vm/MatchPairs.h"

using namespace js;
using namespace js::irregexp;
using namespace js::jit;






















NativeRegExpMacroAssembler::NativeRegExpMacroAssembler(LifoAlloc* alloc, RegExpShared* shared,
                                                       JSRuntime* rt, Mode mode, int registers_to_save)
  : RegExpMacroAssembler(*alloc, shared, registers_to_save),
    runtime(rt), mode_(mode)
{
    
    AllocatableGeneralRegisterSet regs(GeneralRegisterSet::All());

    input_end_pointer = regs.takeAny();
    current_character = regs.takeAny();
    current_position = regs.takeAny();
    backtrack_stack_pointer = regs.takeAny();
    temp0 = regs.takeAny();
    temp1 = regs.takeAny();
    temp2 = regs.takeAny();

    JitSpew(JitSpew_Codegen,
            "Starting RegExp (input_end_pointer %s) (current_character %s)"
            " (current_position %s) (backtrack_stack_pointer %s) (temp0 %s) temp1 (%s) temp2 (%s)",
            input_end_pointer.name(),
            current_character.name(),
            current_position.name(),
            backtrack_stack_pointer.name(),
            temp0.name(),
            temp1.name(),
            temp2.name());

    savedNonVolatileRegisters = SavedNonVolatileRegisters(regs);

    masm.jump(&entry_label_);
    masm.bind(&start_label_);
}

#define SPEW_PREFIX JitSpew_Codegen, "!!! "




RegExpCode
NativeRegExpMacroAssembler::GenerateCode(JSContext* cx, bool match_only)
{
    if (!cx->compartment()->ensureJitCompartmentExists(cx))
        return RegExpCode();

    JitSpew(SPEW_PREFIX "GenerateCode");

    
    if (num_registers_ % 2 == 1)
        num_registers_++;

    Label return_temp0;

    
    
    masm.bind(&entry_label_);

    
    size_t pushedNonVolatileRegisters = 0;
    for (GeneralRegisterForwardIterator iter(savedNonVolatileRegisters); iter.more(); ++iter) {
        masm.Push(*iter);
        pushedNonVolatileRegisters++;
    }

#ifndef JS_CODEGEN_X86
    
    
    masm.Push(IntArgReg0);
#endif

    size_t frameSize = sizeof(FrameData) + num_registers_ * sizeof(void*);
    frameSize = JS_ROUNDUP(frameSize + masm.framePushed(), ABIStackAlignment) - masm.framePushed();

    
    masm.reserveStack(frameSize);
    masm.checkStackAlignment();

    
    Label stack_ok;
    void* stack_limit = runtime->addressOfJitStackLimit();
    masm.branchStackPtrRhs(Assembler::Below, AbsoluteAddress(stack_limit), &stack_ok);

    
    
    masm.movePtr(ImmWord(RegExpRunStatus_Error), temp0);
    masm.jump(&return_temp0);

    masm.bind(&stack_ok);

#ifdef XP_WIN
    
    
    const int kPageSize = 4096;
    for (int i = frameSize - sizeof(void*); i >= 0; i -= kPageSize)
        masm.storePtr(temp0, Address(masm.getStackPointer(), i));
#endif 

#ifndef JS_CODEGEN_X86
    
    Address inputOutputAddress(masm.getStackPointer(), frameSize);
#else
    
    Address inputOutputAddress(masm.getStackPointer(),
                               frameSize + (pushedNonVolatileRegisters + 1) * sizeof(void*));
#endif

    masm.loadPtr(inputOutputAddress, temp0);

    
    if (!match_only) {
        Register matchPairsRegister = input_end_pointer;
        masm.loadPtr(Address(temp0, offsetof(InputOutputData, matches)), matchPairsRegister);
        masm.loadPtr(Address(matchPairsRegister, MatchPairs::offsetOfPairs()), temp1);
        masm.storePtr(temp1, Address(masm.getStackPointer(), offsetof(FrameData, outputRegisters)));
        masm.load32(Address(matchPairsRegister, MatchPairs::offsetOfPairCount()), temp1);
        masm.lshiftPtr(Imm32(1), temp1);
        masm.store32(temp1, Address(masm.getStackPointer(), offsetof(FrameData, numOutputRegisters)));

#ifdef DEBUG
        
        Label enoughRegisters;
        masm.branchPtr(Assembler::GreaterThanOrEqual,
                       temp1, ImmWord(num_saved_registers_), &enoughRegisters);
        masm.assumeUnreachable("Not enough output registers for RegExp");
        masm.bind(&enoughRegisters);
#endif
    }

    
    masm.loadPtr(Address(temp0, offsetof(InputOutputData, inputEnd)), input_end_pointer);

    
    masm.loadPtr(Address(temp0, offsetof(InputOutputData, inputStart)), current_position);
    masm.storePtr(current_position, Address(masm.getStackPointer(), offsetof(FrameData, inputStart)));

    
    masm.loadPtr(Address(temp0, offsetof(InputOutputData, startIndex)), temp1);
    masm.storePtr(temp1, Address(masm.getStackPointer(), offsetof(FrameData, startIndex)));

    
    masm.subPtr(input_end_pointer, current_position);

    
    
    masm.computeEffectiveAddress(Address(current_position, -char_size()), temp0);

    
    
    masm.storePtr(temp0, Address(masm.getStackPointer(), offsetof(FrameData, inputStartMinusOne)));

    
    masm.computeEffectiveAddress(BaseIndex(current_position, temp1, factor()), current_position);

    Label load_char_start_regexp, start_regexp;

    
    masm.branchPtr(Assembler::NotEqual, 
                   Address(masm.getStackPointer(), offsetof(FrameData, startIndex)), ImmWord(0),
                   &load_char_start_regexp);
    masm.movePtr(ImmWord('\n'), current_character);
    masm.jump(&start_regexp);

    
    masm.bind(&load_char_start_regexp);

    
    LoadCurrentCharacterUnchecked(-1, 1);
    masm.bind(&start_regexp);

    
    MOZ_ASSERT(num_saved_registers_ > 0);

    
    
    
    if (num_saved_registers_ > 8) {
        masm.movePtr(ImmWord(register_offset(0)), temp1);
        Label init_loop;
        masm.bind(&init_loop);
        masm.storePtr(temp0, BaseIndex(masm.getStackPointer(), temp1, TimesOne));
        masm.addPtr(ImmWord(sizeof(void*)), temp1);
        masm.branchPtr(Assembler::LessThan, temp1,
                       ImmWord(register_offset(num_saved_registers_)), &init_loop);
    } else {
        
        for (int i = 0; i < num_saved_registers_; i++)
            masm.storePtr(temp0, register_location(i));
    }

    
    masm.loadPtr(AbsoluteAddress(runtime->regexpStack.addressOfBase()), backtrack_stack_pointer);
    masm.storePtr(backtrack_stack_pointer,
                  Address(masm.getStackPointer(), offsetof(FrameData, backtrackStackBase)));

    masm.jump(&start_label_);

    
    if (success_label_.used()) {
        MOZ_ASSERT(num_saved_registers_ > 0);

        Address outputRegistersAddress(masm.getStackPointer(), offsetof(FrameData, outputRegisters));

        
        masm.bind(&success_label_);

        if (!match_only) {
            Register outputRegisters = temp1;
            Register inputByteLength = backtrack_stack_pointer;

            masm.loadPtr(outputRegistersAddress, outputRegisters);

            masm.loadPtr(inputOutputAddress, temp0);
            masm.loadPtr(Address(temp0, offsetof(InputOutputData, inputEnd)), inputByteLength);
            masm.subPtr(Address(temp0, offsetof(InputOutputData, inputStart)), inputByteLength);

            
            
            for (int i = 0; i < num_saved_registers_; i++) {
                masm.loadPtr(register_location(i), temp0);
                if (i == 0 && global_with_zero_length_check()) {
                    
                    masm.movePtr(temp0, current_character);
                }

                
                masm.addPtr(inputByteLength, temp0);

                
                if (mode_ == CHAR16)
                    masm.rshiftPtrArithmetic(Imm32(1), temp0);

                masm.store32(temp0, Address(outputRegisters, i * sizeof(int32_t)));
            }
        }

        
        if (global()) {
            
            masm.add32(Imm32(1), Address(masm.getStackPointer(), offsetof(FrameData, successfulCaptures)));

            Address numOutputRegistersAddress(masm.getStackPointer(), offsetof(FrameData, numOutputRegisters));

            
            
            masm.load32(numOutputRegistersAddress, temp0);

            masm.sub32(Imm32(num_saved_registers_), temp0);

            
            masm.branch32(Assembler::LessThan, temp0, Imm32(num_saved_registers_), &exit_label_);

            masm.store32(temp0, numOutputRegistersAddress);

            
            masm.add32(Imm32(num_saved_registers_ * sizeof(void*)), outputRegistersAddress);

            
            masm.loadPtr(Address(masm.getStackPointer(), offsetof(FrameData, inputStartMinusOne)), temp0);

            if (global_with_zero_length_check()) {
                

                
                masm.branchPtr(Assembler::NotEqual, current_position, current_character,
                               &load_char_start_regexp);

                
                masm.branchTestPtr(Assembler::Zero, current_position, current_position,
                                   &exit_label_);

                
                masm.addPtr(Imm32(char_size()), current_position);
            }

            masm.jump(&load_char_start_regexp);
        } else {
            masm.movePtr(ImmWord(RegExpRunStatus_Success), temp0);
        }
    }

    masm.bind(&exit_label_);

    if (global()) {
        
        masm.load32(Address(masm.getStackPointer(), offsetof(FrameData, successfulCaptures)), temp0);
    }

    masm.bind(&return_temp0);

    
    masm.loadPtr(inputOutputAddress, temp1);
    masm.storePtr(temp0, Address(temp1, offsetof(InputOutputData, result)));

#ifndef JS_CODEGEN_X86
    
    masm.freeStack(frameSize + sizeof(void*));
#else
    masm.freeStack(frameSize);
#endif

    
    for (GeneralRegisterBackwardIterator iter(savedNonVolatileRegisters); iter.more(); ++iter)
        masm.Pop(*iter);

    masm.abiret();

    
    if (backtrack_label_.used()) {
        masm.bind(&backtrack_label_);
        Backtrack();
    }

    
    if (stack_overflow_label_.used()) {
        
        
        masm.bind(&stack_overflow_label_);

        Label grow_failed;

        masm.movePtr(ImmPtr(runtime), temp1);

        
        LiveGeneralRegisterSet volatileRegs(GeneralRegisterSet::Volatile());
#if defined(JS_CODEGEN_ARM)
        volatileRegs.add(Register::FromCode(Registers::lr));
#elif defined(JS_CODEGEN_MIPS)
        volatileRegs.add(Register::FromCode(Registers::ra));
#endif
        volatileRegs.takeUnchecked(temp0);
        volatileRegs.takeUnchecked(temp1);
        masm.PushRegsInMask(volatileRegs);

        masm.setupUnalignedABICall(1, temp0);
        masm.passABIArg(temp1);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, GrowBacktrackStack));
        masm.storeCallResult(temp0);

        masm.PopRegsInMask(volatileRegs);

        
        
        
        Label return_from_overflow_handler;
        masm.branchTest32(Assembler::Zero, temp0, temp0, &return_from_overflow_handler);

        
        
        Address backtrackStackBaseAddress(temp2, offsetof(FrameData, backtrackStackBase));
        masm.subPtr(backtrackStackBaseAddress, backtrack_stack_pointer);

        masm.loadPtr(AbsoluteAddress(runtime->regexpStack.addressOfBase()), temp1);
        masm.storePtr(temp1, backtrackStackBaseAddress);
        masm.addPtr(temp1, backtrack_stack_pointer);

        
        masm.bind(&return_from_overflow_handler);
        masm.abiret();
    }

    if (exit_with_exception_label_.used()) {
        
        masm.bind(&exit_with_exception_label_);

        
        masm.movePtr(ImmWord(RegExpRunStatus_Error), temp0);
        masm.jump(&return_temp0);
    }

    Linker linker(masm);
    AutoFlushICache afc("RegExp");
    JitCode* code = linker.newCode<NoGC>(cx, REGEXP_CODE);
    if (!code)
        return RegExpCode();

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "RegExp");
#endif

    for (size_t i = 0; i < labelPatches.length(); i++) {
        LabelPatch& v = labelPatches[i];
        MOZ_ASSERT(!v.label);
        v.patchOffset.fixup(&masm);
        uintptr_t offset = masm.actualOffset(v.labelOffset);
        Assembler::PatchDataWithValueCheck(CodeLocationLabel(code, v.patchOffset),
                                           ImmPtr(code->raw() + offset),
                                           ImmPtr(0));
    }

    JitSpew(JitSpew_Codegen, "Created RegExp (raw %p length %d)",
            (void*) code->raw(), (int) masm.bytesNeeded());

    RegExpCode res;
    res.jitCode = code;
    return res;
}

int
NativeRegExpMacroAssembler::stack_limit_slack()
{
    return RegExpStack::kStackLimitSlack;
}

void
NativeRegExpMacroAssembler::AdvanceCurrentPosition(int by)
{
    JitSpew(SPEW_PREFIX "AdvanceCurrentPosition(%d)", by);

    if (by != 0)
        masm.addPtr(Imm32(by * char_size()), current_position);
}

void
NativeRegExpMacroAssembler::AdvanceRegister(int reg, int by)
{
    JitSpew(SPEW_PREFIX "AdvanceRegister(%d, %d)", reg, by);

    MOZ_ASSERT(reg >= 0);
    MOZ_ASSERT(reg < num_registers_);
    if (by != 0)
        masm.addPtr(Imm32(by), register_location(reg));
}

void
NativeRegExpMacroAssembler::Backtrack()
{
    JitSpew(SPEW_PREFIX "Backtrack");

    
    Label noInterrupt;
    masm.branch32(Assembler::Equal,
                  AbsoluteAddress(runtime->addressOfInterruptUint32()), Imm32(0),
                  &noInterrupt);
    masm.movePtr(ImmWord(RegExpRunStatus_Error), temp0);
    masm.jump(&exit_label_);
    masm.bind(&noInterrupt);

    
    PopBacktrack(temp0);
    masm.jump(temp0);
}

void
NativeRegExpMacroAssembler::Bind(Label* label)
{
    JitSpew(SPEW_PREFIX "Bind");

    masm.bind(label);
}

void
NativeRegExpMacroAssembler::CheckAtStart(Label* on_at_start)
{
    JitSpew(SPEW_PREFIX "CheckAtStart");

    Label not_at_start;

    
    Address startIndex(masm.getStackPointer(), offsetof(FrameData, startIndex));
    masm.branchPtr(Assembler::NotEqual, startIndex, ImmWord(0), &not_at_start);

    
    masm.computeEffectiveAddress(BaseIndex(input_end_pointer, current_position, TimesOne), temp0);

    Address inputStart(masm.getStackPointer(), offsetof(FrameData, inputStart));
    masm.branchPtr(Assembler::Equal, inputStart, temp0, BranchOrBacktrack(on_at_start));

    masm.bind(&not_at_start);
}

void
NativeRegExpMacroAssembler::CheckNotAtStart(Label* on_not_at_start)
{
    JitSpew(SPEW_PREFIX "CheckNotAtStart");

    
    Address startIndex(masm.getStackPointer(), offsetof(FrameData, startIndex));
    masm.branchPtr(Assembler::NotEqual, startIndex, ImmWord(0), BranchOrBacktrack(on_not_at_start));

    
    masm.computeEffectiveAddress(BaseIndex(input_end_pointer, current_position, TimesOne), temp0);

    Address inputStart(masm.getStackPointer(), offsetof(FrameData, inputStart));
    masm.branchPtr(Assembler::NotEqual, inputStart, temp0, BranchOrBacktrack(on_not_at_start));
}

void
NativeRegExpMacroAssembler::CheckCharacter(unsigned c, Label* on_equal)
{
    JitSpew(SPEW_PREFIX "CheckCharacter(%d)", (int) c);
    masm.branch32(Assembler::Equal, current_character, Imm32(c), BranchOrBacktrack(on_equal));
}

void
NativeRegExpMacroAssembler::CheckNotCharacter(unsigned c, Label* on_not_equal)
{
    JitSpew(SPEW_PREFIX "CheckNotCharacter(%d)", (int) c);
    masm.branch32(Assembler::NotEqual, current_character, Imm32(c), BranchOrBacktrack(on_not_equal));
}

void
NativeRegExpMacroAssembler::CheckCharacterAfterAnd(unsigned c, unsigned and_with,
                                                   Label* on_equal)
{
    JitSpew(SPEW_PREFIX "CheckCharacterAfterAnd(%d, %d)", (int) c, (int) and_with);

    if (c == 0) {
        masm.branchTest32(Assembler::Zero, current_character, Imm32(and_with),
                          BranchOrBacktrack(on_equal));
    } else {
        masm.move32(Imm32(and_with), temp0);
        masm.and32(current_character, temp0);
        masm.branch32(Assembler::Equal, temp0, Imm32(c), BranchOrBacktrack(on_equal));
    }
}

void
NativeRegExpMacroAssembler::CheckNotCharacterAfterAnd(unsigned c, unsigned and_with,
                                                      Label* on_not_equal)
{
    JitSpew(SPEW_PREFIX "CheckNotCharacterAfterAnd(%d, %d)", (int) c, (int) and_with);

    if (c == 0) {
        masm.branchTest32(Assembler::NonZero, current_character, Imm32(and_with),
                          BranchOrBacktrack(on_not_equal));
    } else {
        masm.move32(Imm32(and_with), temp0);
        masm.and32(current_character, temp0);
        masm.branch32(Assembler::NotEqual, temp0, Imm32(c), BranchOrBacktrack(on_not_equal));
    }
}

void
NativeRegExpMacroAssembler::CheckCharacterGT(char16_t c, Label* on_greater)
{
    JitSpew(SPEW_PREFIX "CheckCharacterGT(%d)", (int) c);
    masm.branch32(Assembler::GreaterThan, current_character, Imm32(c),
                  BranchOrBacktrack(on_greater));
}

void
NativeRegExpMacroAssembler::CheckCharacterLT(char16_t c, Label* on_less)
{
    JitSpew(SPEW_PREFIX "CheckCharacterLT(%d)", (int) c);
    masm.branch32(Assembler::LessThan, current_character, Imm32(c), BranchOrBacktrack(on_less));
}

void
NativeRegExpMacroAssembler::CheckGreedyLoop(Label* on_tos_equals_current_position)
{
    JitSpew(SPEW_PREFIX "CheckGreedyLoop");

    Label fallthrough;
    masm.branchPtr(Assembler::NotEqual,
                   Address(backtrack_stack_pointer, -int(sizeof(void*))), current_position,
                   &fallthrough);
    masm.subPtr(Imm32(sizeof(void*)), backtrack_stack_pointer);  
    JumpOrBacktrack(on_tos_equals_current_position);
    masm.bind(&fallthrough);
}

void
NativeRegExpMacroAssembler::CheckNotBackReference(int start_reg, Label* on_no_match)
{
    JitSpew(SPEW_PREFIX "CheckNotBackReference(%d)", start_reg);

    Label fallthrough;
    Label success;
    Label fail;

    
    masm.loadPtr(register_location(start_reg), current_character);
    masm.loadPtr(register_location(start_reg + 1), temp0);
    masm.subPtr(current_character, temp0);  

    
    masm.branchPtr(Assembler::LessThan, temp0, ImmWord(0), BranchOrBacktrack(on_no_match));

    
    masm.branchPtr(Assembler::Equal, temp0, ImmWord(0), &fallthrough);

    
    masm.movePtr(current_position, temp1);
    masm.addPtr(temp0, temp1);
    masm.branchPtr(Assembler::GreaterThan, temp1, ImmWord(0), BranchOrBacktrack(on_no_match));

    
    masm.push(backtrack_stack_pointer);

    
    masm.computeEffectiveAddress(BaseIndex(input_end_pointer, current_position, TimesOne), temp1); 
    masm.addPtr(input_end_pointer, current_character); 
    masm.computeEffectiveAddress(BaseIndex(temp0, temp1, TimesOne), backtrack_stack_pointer); 

    Label loop;
    masm.bind(&loop);
    if (mode_ == ASCII) {
        masm.load8ZeroExtend(Address(current_character, 0), temp0);
        masm.load8ZeroExtend(Address(temp1, 0), temp2);
    } else {
        MOZ_ASSERT(mode_ == CHAR16);
        masm.load16ZeroExtend(Address(current_character, 0), temp0);
        masm.load16ZeroExtend(Address(temp1, 0), temp2);
    }
    masm.branch32(Assembler::NotEqual, temp0, temp2, &fail);

    
    masm.addPtr(Imm32(char_size()), current_character);
    masm.addPtr(Imm32(char_size()), temp1);

    
    masm.branchPtr(Assembler::Below, temp1, backtrack_stack_pointer, &loop);
    masm.jump(&success);

    masm.bind(&fail);

    
    masm.pop(backtrack_stack_pointer);
    JumpOrBacktrack(on_no_match);

    masm.bind(&success);

    
    masm.movePtr(backtrack_stack_pointer, current_position);
    masm.subPtr(input_end_pointer, current_position);

    
    masm.pop(backtrack_stack_pointer);

    masm.bind(&fallthrough);
}

void
NativeRegExpMacroAssembler::CheckNotBackReferenceIgnoreCase(int start_reg, Label* on_no_match)
{
    JitSpew(SPEW_PREFIX "CheckNotBackReferenceIgnoreCase(%d)", start_reg);

    Label fallthrough;

    masm.loadPtr(register_location(start_reg), current_character);  
    masm.loadPtr(register_location(start_reg + 1), temp1);  
    masm.subPtr(current_character, temp1);  

    
    
    
    masm.branchPtr(Assembler::LessThan, temp1, ImmWord(0), BranchOrBacktrack(on_no_match));

    
    
    masm.branchPtr(Assembler::Equal, temp1, ImmWord(0), &fallthrough);

    
    masm.movePtr(current_position, temp0);
    masm.addPtr(temp1, temp0);
    masm.branchPtr(Assembler::GreaterThan, temp0, ImmWord(0), BranchOrBacktrack(on_no_match));

    if (mode_ == ASCII) {
        Label success, fail;

        
        
        masm.push(current_position);

        masm.addPtr(input_end_pointer, current_character); 
        masm.addPtr(input_end_pointer, current_position); 
        masm.addPtr(current_position, temp1); 

        Label loop, loop_increment;
        masm.bind(&loop);
        masm.load8ZeroExtend(Address(current_position, 0), temp0);
        masm.load8ZeroExtend(Address(current_character, 0), temp2);
        masm.branch32(Assembler::Equal, temp0, temp2, &loop_increment);

        
        masm.or32(Imm32(0x20), temp0); 

        
        Label convert_capture;
        masm.computeEffectiveAddress(Address(temp0, -'a'), temp2);
        masm.branch32(Assembler::BelowOrEqual, temp2, Imm32(static_cast<int32_t>('z' - 'a')),
                      &convert_capture);

        
        masm.sub32(Imm32(224 - 'a'), temp2);
        masm.branch32(Assembler::Above, temp2, Imm32(254 - 224), &fail);

        
        masm.branch32(Assembler::Equal, temp2, Imm32(247 - 224), &fail);

        masm.bind(&convert_capture);

        
        masm.load8ZeroExtend(Address(current_character, 0), temp2);
        masm.or32(Imm32(0x20), temp2);

        masm.branch32(Assembler::NotEqual, temp0, temp2, &fail);

        masm.bind(&loop_increment);

        
        masm.addPtr(Imm32(1), current_character);
        masm.addPtr(Imm32(1), current_position);

        
        masm.branchPtr(Assembler::Below, current_position, temp1, &loop);
        masm.jump(&success);

        masm.bind(&fail);

        
        masm.pop(current_position);
        JumpOrBacktrack(on_no_match);

        masm.bind(&success);

        
        masm.addToStackPtr(Imm32(sizeof(uintptr_t)));

        
        masm.subPtr(input_end_pointer, current_position);
    } else {
        MOZ_ASSERT(mode_ == CHAR16);

        
        LiveGeneralRegisterSet volatileRegs(GeneralRegisterSet::Volatile());
        volatileRegs.takeUnchecked(temp0);
        volatileRegs.takeUnchecked(temp2);
        masm.PushRegsInMask(volatileRegs);

        
        
        masm.addPtr(input_end_pointer, current_character);

        
        
        
        masm.addPtr(input_end_pointer, current_position);

        
        
        
        
        masm.setupUnalignedABICall(3, temp0);
        masm.passABIArg(current_character);
        masm.passABIArg(current_position);
        masm.passABIArg(temp1);
        int (*fun)(const char16_t*, const char16_t*, size_t) = CaseInsensitiveCompareStrings;
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, fun));
        masm.storeCallResult(temp0);

        masm.PopRegsInMask(volatileRegs);

        
        masm.branchTest32(Assembler::Zero, temp0, temp0, BranchOrBacktrack(on_no_match));

        
        masm.addPtr(temp1, current_position);
    }

    masm.bind(&fallthrough);
}

void
NativeRegExpMacroAssembler::CheckNotCharacterAfterMinusAnd(char16_t c, char16_t minus, char16_t and_with,
                                                           Label* on_not_equal)
{
    JitSpew(SPEW_PREFIX "CheckNotCharacterAfterMinusAnd(%d, %d, %d)", (int) c,
            (int) minus, (int) and_with);

    masm.computeEffectiveAddress(Address(current_character, -minus), temp0);
    if (c == 0) {
        masm.branchTest32(Assembler::NonZero, temp0, Imm32(and_with),
                          BranchOrBacktrack(on_not_equal));
    } else {
        masm.and32(Imm32(and_with), temp0);
        masm.branch32(Assembler::NotEqual, temp0, Imm32(c), BranchOrBacktrack(on_not_equal));
    }
}

void
NativeRegExpMacroAssembler::CheckCharacterInRange(char16_t from, char16_t to,
                                                  Label* on_in_range)
{
    JitSpew(SPEW_PREFIX "CheckCharacterInRange(%d, %d)", (int) from, (int) to);

    masm.computeEffectiveAddress(Address(current_character, -from), temp0);
    masm.branch32(Assembler::BelowOrEqual, temp0, Imm32(to - from), BranchOrBacktrack(on_in_range));
}

void
NativeRegExpMacroAssembler::CheckCharacterNotInRange(char16_t from, char16_t to,
                                                     Label* on_not_in_range)
{
    JitSpew(SPEW_PREFIX "CheckCharacterNotInRange(%d, %d)", (int) from, (int) to);

    masm.computeEffectiveAddress(Address(current_character, -from), temp0);
    masm.branch32(Assembler::Above, temp0, Imm32(to - from), BranchOrBacktrack(on_not_in_range));
}

void
NativeRegExpMacroAssembler::CheckBitInTable(uint8_t* table, Label* on_bit_set)
{
    JitSpew(SPEW_PREFIX "CheckBitInTable");

    masm.movePtr(ImmPtr(table), temp0);

    
    
    static_assert(JSString::MAX_LATIN1_CHAR > kTableMask,
                  "No need to mask if MAX_LATIN1_CHAR <= kTableMask");
    masm.move32(Imm32(kTableSize - 1), temp1);
    masm.and32(current_character, temp1);

    masm.load8ZeroExtend(BaseIndex(temp0, temp1, TimesOne), temp0);
    masm.branchTest32(Assembler::NonZero, temp0, temp0, BranchOrBacktrack(on_bit_set));
}

void
NativeRegExpMacroAssembler::Fail()
{
    JitSpew(SPEW_PREFIX "Fail");

    if (!global())
        masm.movePtr(ImmWord(RegExpRunStatus_Success_NotFound), temp0);
    masm.jump(&exit_label_);
}

void
NativeRegExpMacroAssembler::IfRegisterGE(int reg, int comparand, Label* if_ge)
{
    JitSpew(SPEW_PREFIX "IfRegisterGE(%d, %d)", reg, comparand);
    masm.branchPtr(Assembler::GreaterThanOrEqual, register_location(reg), ImmWord(comparand),
                   BranchOrBacktrack(if_ge));
}

void
NativeRegExpMacroAssembler::IfRegisterLT(int reg, int comparand, Label* if_lt)
{
    JitSpew(SPEW_PREFIX "IfRegisterLT(%d, %d)", reg, comparand);
    masm.branchPtr(Assembler::LessThan, register_location(reg), ImmWord(comparand),
                   BranchOrBacktrack(if_lt));
}

void
NativeRegExpMacroAssembler::IfRegisterEqPos(int reg, Label* if_eq)
{
    JitSpew(SPEW_PREFIX "IfRegisterEqPos(%d)", reg);
    masm.branchPtr(Assembler::Equal, register_location(reg), current_position,
                   BranchOrBacktrack(if_eq));
}

void
NativeRegExpMacroAssembler::LoadCurrentCharacter(int cp_offset, Label* on_end_of_input,
                                                 bool check_bounds, int characters)
{
    JitSpew(SPEW_PREFIX "LoadCurrentCharacter(%d, %d)", cp_offset, characters);

    MOZ_ASSERT(cp_offset >= -1);      
    MOZ_ASSERT(cp_offset < (1<<30));  
    if (check_bounds)
        CheckPosition(cp_offset + characters - 1, on_end_of_input);
    LoadCurrentCharacterUnchecked(cp_offset, characters);
}

void
NativeRegExpMacroAssembler::LoadCurrentCharacterUnchecked(int cp_offset, int characters)
{
    JitSpew(SPEW_PREFIX "LoadCurrentCharacterUnchecked(%d, %d)", cp_offset, characters);

    if (mode_ == ASCII) {
        BaseIndex address(input_end_pointer, current_position, TimesOne, cp_offset);
        if (characters == 4) {
            masm.load32(address, current_character);
        } else if (characters == 2) {
            masm.load16ZeroExtend(address, current_character);
        } else {
            MOZ_ASSERT(characters == 1);
            masm.load8ZeroExtend(address, current_character);
        }
    } else {
        MOZ_ASSERT(mode_ == CHAR16);
        MOZ_ASSERT(characters <= 2);
        BaseIndex address(input_end_pointer, current_position, TimesOne, cp_offset * sizeof(char16_t));
        if (characters == 2)
            masm.load32(address, current_character);
        else
            masm.load16ZeroExtend(address, current_character);
    }
}

void
NativeRegExpMacroAssembler::PopCurrentPosition()
{
    JitSpew(SPEW_PREFIX "PopCurrentPosition");

    PopBacktrack(current_position);
}

void
NativeRegExpMacroAssembler::PopRegister(int register_index)
{
    JitSpew(SPEW_PREFIX "PopRegister(%d)", register_index);

    PopBacktrack(temp0);
    masm.storePtr(temp0, register_location(register_index));
}

void
NativeRegExpMacroAssembler::PushBacktrack(Label* label)
{
    JitSpew(SPEW_PREFIX "PushBacktrack");

    CodeOffsetLabel patchOffset = masm.movWithPatch(ImmPtr(nullptr), temp0);

    MOZ_ASSERT(!label->bound());
    if (!labelPatches.append(LabelPatch(label, patchOffset)))
        CrashAtUnhandlableOOM("NativeRegExpMacroAssembler::PushBacktrack");

    PushBacktrack(temp0);
    CheckBacktrackStackLimit();
}

void
NativeRegExpMacroAssembler::BindBacktrack(Label* label)
{
    JitSpew(SPEW_PREFIX "BindBacktrack");

    Bind(label);

    for (size_t i = 0; i < labelPatches.length(); i++) {
        LabelPatch& v = labelPatches[i];
        if (v.label == label) {
            v.labelOffset = label->offset();
            v.label = nullptr;
            break;
        }
    }
}

void
NativeRegExpMacroAssembler::PushBacktrack(Register source)
{
    JitSpew(SPEW_PREFIX "PushBacktrack");

    MOZ_ASSERT(source != backtrack_stack_pointer);

    
    masm.storePtr(source, Address(backtrack_stack_pointer, 0));
    masm.addPtr(Imm32(sizeof(void*)), backtrack_stack_pointer);
}

void
NativeRegExpMacroAssembler::PushBacktrack(int32_t value)
{
    JitSpew(SPEW_PREFIX "PushBacktrack(%d)", (int) value);

    
    masm.storePtr(ImmWord(value), Address(backtrack_stack_pointer, 0));
    masm.addPtr(Imm32(sizeof(void*)), backtrack_stack_pointer);
}

void
NativeRegExpMacroAssembler::PopBacktrack(Register target)
{
    JitSpew(SPEW_PREFIX "PopBacktrack");

    MOZ_ASSERT(target != backtrack_stack_pointer);

    
    masm.subPtr(Imm32(sizeof(void*)), backtrack_stack_pointer);
    masm.loadPtr(Address(backtrack_stack_pointer, 0), target);
}

void
NativeRegExpMacroAssembler::CheckBacktrackStackLimit()
{
    JitSpew(SPEW_PREFIX "CheckBacktrackStackLimit");

    const void* limitAddr = runtime->regexpStack.addressOfLimit();

    Label no_stack_overflow;
    masm.branchPtr(Assembler::AboveOrEqual, AbsoluteAddress(limitAddr),
                   backtrack_stack_pointer, &no_stack_overflow);

    
    masm.moveStackPtrTo(temp2);

    masm.call(&stack_overflow_label_);
    masm.bind(&no_stack_overflow);

    
    masm.branchTest32(Assembler::Zero, temp0, temp0, &exit_with_exception_label_);
}

void
NativeRegExpMacroAssembler::PushCurrentPosition()
{
    JitSpew(SPEW_PREFIX "PushCurrentPosition");

    PushBacktrack(current_position);
}

void
NativeRegExpMacroAssembler::PushRegister(int register_index, StackCheckFlag check_stack_limit)
{
    JitSpew(SPEW_PREFIX "PushRegister(%d)", register_index);

    masm.loadPtr(register_location(register_index), temp0);
    PushBacktrack(temp0);
    if (check_stack_limit)
        CheckBacktrackStackLimit();
}

void
NativeRegExpMacroAssembler::ReadCurrentPositionFromRegister(int reg)
{
    JitSpew(SPEW_PREFIX "ReadCurrentPositionFromRegister(%d)", reg);

    masm.loadPtr(register_location(reg), current_position);
}

void
NativeRegExpMacroAssembler::WriteCurrentPositionToRegister(int reg, int cp_offset)
{
    JitSpew(SPEW_PREFIX "WriteCurrentPositionToRegister(%d, %d)", reg, cp_offset);

    if (cp_offset == 0) {
        masm.storePtr(current_position, register_location(reg));
    } else {
        masm.computeEffectiveAddress(Address(current_position, cp_offset * char_size()), temp0);
        masm.storePtr(temp0, register_location(reg));
    }
}

void
NativeRegExpMacroAssembler::ReadBacktrackStackPointerFromRegister(int reg)
{
    JitSpew(SPEW_PREFIX "ReadBacktrackStackPointerFromRegister(%d)", reg);

    masm.loadPtr(register_location(reg), backtrack_stack_pointer);
    masm.addPtr(Address(masm.getStackPointer(),
                offsetof(FrameData, backtrackStackBase)), backtrack_stack_pointer);
}

void
NativeRegExpMacroAssembler::WriteBacktrackStackPointerToRegister(int reg)
{
    JitSpew(SPEW_PREFIX "WriteBacktrackStackPointerToRegister(%d)", reg);

    masm.movePtr(backtrack_stack_pointer, temp0);
    masm.subPtr(Address(masm.getStackPointer(),
                offsetof(FrameData, backtrackStackBase)), temp0);
    masm.storePtr(temp0, register_location(reg));
}

void
NativeRegExpMacroAssembler::SetCurrentPositionFromEnd(int by)
{
    JitSpew(SPEW_PREFIX "SetCurrentPositionFromEnd(%d)", by);

    Label after_position;
    masm.branchPtr(Assembler::GreaterThanOrEqual, current_position,
                   ImmWord(-by * char_size()), &after_position);
    masm.movePtr(ImmWord(-by * char_size()), current_position);

    
    
    
    LoadCurrentCharacterUnchecked(-1, 1);
    masm.bind(&after_position);
}

void
NativeRegExpMacroAssembler::SetRegister(int register_index, int to)
{
    JitSpew(SPEW_PREFIX "SetRegister(%d, %d)", register_index, to);

    MOZ_ASSERT(register_index >= num_saved_registers_);  
    masm.storePtr(ImmWord(to), register_location(register_index));
}

bool
NativeRegExpMacroAssembler::Succeed()
{
    JitSpew(SPEW_PREFIX "Succeed");

    masm.jump(&success_label_);
    return global();
}

void
NativeRegExpMacroAssembler::ClearRegisters(int reg_from, int reg_to)
{
    JitSpew(SPEW_PREFIX "ClearRegisters(%d, %d)", reg_from, reg_to);

    MOZ_ASSERT(reg_from <= reg_to);
    masm.loadPtr(Address(masm.getStackPointer(), offsetof(FrameData, inputStartMinusOne)), temp0);
    for (int reg = reg_from; reg <= reg_to; reg++)
        masm.storePtr(temp0, register_location(reg));
}

void
NativeRegExpMacroAssembler::CheckPosition(int cp_offset, Label* on_outside_input)
{
    JitSpew(SPEW_PREFIX "CheckPosition(%d)", cp_offset);
    masm.branchPtr(Assembler::GreaterThanOrEqual, current_position,
                   ImmWord(-cp_offset * char_size()), BranchOrBacktrack(on_outside_input));
}

Label*
NativeRegExpMacroAssembler::BranchOrBacktrack(Label* branch)
{
    if (branch)
        return branch;
    return &backtrack_label_;
}

void
NativeRegExpMacroAssembler::JumpOrBacktrack(Label* to)
{
    JitSpew(SPEW_PREFIX "JumpOrBacktrack");

    if (to)
        masm.jump(to);
    else
        Backtrack();
}

bool
NativeRegExpMacroAssembler::CheckSpecialCharacterClass(char16_t type, Label* on_no_match)
{
    JitSpew(SPEW_PREFIX "CheckSpecialCharacterClass(%d)", (int) type);

    Label* branch = BranchOrBacktrack(on_no_match);

    
    
    switch (type) {
      case 's':
        
        if (mode_ == ASCII) {
            
            Label success;
            masm.branch32(Assembler::Equal, current_character, Imm32(' '), &success);

            
            masm.computeEffectiveAddress(Address(current_character, -'\t'), temp0);
            masm.branch32(Assembler::BelowOrEqual, temp0, Imm32('\r' - '\t'), &success);

            
            masm.branch32(Assembler::NotEqual, temp0, Imm32(0x00a0 - '\t'), branch);

            masm.bind(&success);
            return true;
        }
        return false;
      case 'S':
        
        return false;
      case 'd':
        
        masm.computeEffectiveAddress(Address(current_character, -'0'), temp0);
        masm.branch32(Assembler::Above, temp0, Imm32('9' - '0'), branch);
        return true;
      case 'D':
        
        masm.computeEffectiveAddress(Address(current_character, -'0'), temp0);
        masm.branch32(Assembler::BelowOrEqual, temp0, Imm32('9' - '0'), branch);
        return true;
      case '.': {
        
        masm.move32(current_character, temp0);
        masm.xor32(Imm32(0x01), temp0);

        
        masm.sub32(Imm32(0x0b), temp0);
        masm.branch32(Assembler::BelowOrEqual, temp0, Imm32(0x0c - 0x0b), branch);
        if (mode_ == CHAR16) {
            
            
            
            masm.sub32(Imm32(0x2028 - 0x0b), temp0);
            masm.branch32(Assembler::BelowOrEqual, temp0, Imm32(0x2029 - 0x2028), branch);
        }
        return true;
      }
      case 'w': {
        if (mode_ != ASCII) {
            
            masm.branch32(Assembler::Above, current_character, Imm32('z'), branch);
        }
        MOZ_ASSERT(0 == word_character_map[0]);  
        masm.movePtr(ImmPtr(word_character_map), temp0);
        masm.load8ZeroExtend(BaseIndex(temp0, current_character, TimesOne), temp0);
        masm.branchTest32(Assembler::Zero, temp0, temp0, branch);
        return true;
      }
      case 'W': {
        Label done;
        if (mode_ != ASCII) {
            
            masm.branch32(Assembler::Above, current_character, Imm32('z'), &done);
        }
        MOZ_ASSERT(0 == word_character_map[0]);  
        masm.movePtr(ImmPtr(word_character_map), temp0);
        masm.load8ZeroExtend(BaseIndex(temp0, current_character, TimesOne), temp0);
        masm.branchTest32(Assembler::NonZero, temp0, temp0, branch);
        if (mode_ != ASCII)
            masm.bind(&done);
        return true;
      }
        
      case '*':
        
        return true;
      case 'n': {
        
        
        masm.move32(current_character, temp0);
        masm.xor32(Imm32(0x01), temp0);

        
        masm.sub32(Imm32(0x0b), temp0);

        if (mode_ == ASCII) {
            masm.branch32(Assembler::Above, temp0, Imm32(0x0c - 0x0b), branch);
        } else {
            Label done;
            masm.branch32(Assembler::BelowOrEqual, temp0, Imm32(0x0c - 0x0b), &done);
            MOZ_ASSERT(CHAR16 == mode_);

            
            
            
            masm.sub32(Imm32(0x2028 - 0x0b), temp0);
            masm.branch32(Assembler::Above, temp0, Imm32(1), branch);

            masm.bind(&done);
        }
        return true;
      }
        
      default:
        return false;
    }
}

bool
NativeRegExpMacroAssembler::CanReadUnaligned()
{
#if defined(JS_CODEGEN_ARM)
    return !jit::HasAlignmentFault();
#elif defined(JS_CODEGEN_MIPS)
    return false;
#else
    return true;
#endif
}

const uint8_t
NativeRegExpMacroAssembler::word_character_map[] =
{
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,

    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,  

    0x00u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0xffu, 0x00u, 0x00u, 0x00u, 0x00u, 0xffu,  

    0x00u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,  
    0xffu, 0xffu, 0xffu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,  

    
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,

    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,

    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,

    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
};
