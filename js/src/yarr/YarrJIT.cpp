




























#include "YarrJIT.h"

#include "assembler/assembler/LinkBuffer.h"
#include "Yarr.h"

#if ENABLE_YARR_JIT

using namespace WTF;

namespace JSC { namespace Yarr {

class YarrGenerator : private MacroAssembler {
    friend void jitCompile(JSGlobalData*, YarrCodeBlock& jitObject, const UString& pattern, unsigned& numSubpatterns, const char*& error, bool ignoreCase, bool multiline);

#if WTF_CPU_ARM
    static const RegisterID input = ARMRegisters::r0;
    static const RegisterID index = ARMRegisters::r1;
    static const RegisterID length = ARMRegisters::r2;
    static const RegisterID output = ARMRegisters::r4;

    static const RegisterID regT0 = ARMRegisters::r5;
    static const RegisterID regT1 = ARMRegisters::r6;

    static const RegisterID returnRegister = ARMRegisters::r0;
#elif WTF_CPU_MIPS
    static const RegisterID input = MIPSRegisters::a0;
    static const RegisterID index = MIPSRegisters::a1;
    static const RegisterID length = MIPSRegisters::a2;
    static const RegisterID output = MIPSRegisters::a3;

    static const RegisterID regT0 = MIPSRegisters::t4;
    static const RegisterID regT1 = MIPSRegisters::t5;

    static const RegisterID returnRegister = MIPSRegisters::v0;
#elif WTF_CPU_SH4
    static const RegisterID input = SH4Registers::r4;
    static const RegisterID index = SH4Registers::r5;
    static const RegisterID length = SH4Registers::r6;
    static const RegisterID output = SH4Registers::r7;

    static const RegisterID regT0 = SH4Registers::r0;
    static const RegisterID regT1 = SH4Registers::r1;

    static const RegisterID returnRegister = SH4Registers::r0;
#elif WTF_CPU_X86
    static const RegisterID input = X86Registers::eax;
    static const RegisterID index = X86Registers::edx;
    static const RegisterID length = X86Registers::ecx;
    static const RegisterID output = X86Registers::edi;

    static const RegisterID regT0 = X86Registers::ebx;
    static const RegisterID regT1 = X86Registers::esi;

    static const RegisterID returnRegister = X86Registers::eax;
#elif WTF_CPU_X86_64
    static const RegisterID input = X86Registers::edi;
    static const RegisterID index = X86Registers::esi;
    static const RegisterID length = X86Registers::edx;
    static const RegisterID output = X86Registers::ecx;

    static const RegisterID regT0 = X86Registers::eax;
    static const RegisterID regT1 = X86Registers::ebx;

    static const RegisterID returnRegister = X86Registers::eax;
#endif

    void optimizeAlternative(PatternAlternative* alternative)
    {
        if (!alternative->m_terms.size())
            return;

        for (unsigned i = 0; i < alternative->m_terms.size() - 1; ++i) {
            PatternTerm& term = alternative->m_terms[i];
            PatternTerm& nextTerm = alternative->m_terms[i + 1];

            if ((term.type == PatternTerm::TypeCharacterClass)
                && (term.quantityType == QuantifierFixedCount)
                && (nextTerm.type == PatternTerm::TypePatternCharacter)
                && (nextTerm.quantityType == QuantifierFixedCount)) {
                PatternTerm termCopy = term;
                alternative->m_terms[i] = nextTerm;
                alternative->m_terms[i + 1] = termCopy;
            }
        }
    }

    void matchCharacterClassRange(RegisterID character, JumpList& failures, JumpList& matchDest, const CharacterRange* ranges, unsigned count, unsigned* matchIndex, const UChar* matches, unsigned matchCount)
    {
        do {
            
            int which = count >> 1;
            char lo = ranges[which].begin;
            char hi = ranges[which].end;

            
            
            if ((*matchIndex < matchCount) && (matches[*matchIndex] < lo)) {
                Jump loOrAbove = branch32(GreaterThanOrEqual, character, Imm32((unsigned short)lo));

                
                if (which)
                    matchCharacterClassRange(character, failures, matchDest, ranges, which, matchIndex, matches, matchCount);

                while ((*matchIndex < matchCount) && (matches[*matchIndex] < lo)) {
                    matchDest.append(branch32(Equal, character, Imm32((unsigned short)matches[*matchIndex])));
                    ++*matchIndex;
                }
                failures.append(jump());

                loOrAbove.link(this);
            } else if (which) {
                Jump loOrAbove = branch32(GreaterThanOrEqual, character, Imm32((unsigned short)lo));

                matchCharacterClassRange(character, failures, matchDest, ranges, which, matchIndex, matches, matchCount);
                failures.append(jump());

                loOrAbove.link(this);
            } else
                failures.append(branch32(LessThan, character, Imm32((unsigned short)lo)));

            while ((*matchIndex < matchCount) && (matches[*matchIndex] <= hi))
                ++*matchIndex;

            matchDest.append(branch32(LessThanOrEqual, character, Imm32((unsigned short)hi)));
            

            
            unsigned next = which + 1;
            ranges += next;
            count -= next;
        } while (count);
    }

    void matchCharacterClass(RegisterID character, JumpList& matchDest, const CharacterClass* charClass)
    {
        if (charClass->m_table) {
            ExtendedAddress tableEntry(character, reinterpret_cast<intptr_t>(charClass->m_table->m_table));
            matchDest.append(branchTest8(charClass->m_table->m_inverted ? Zero : NonZero, tableEntry));
            return;
        }
        Jump unicodeFail;
        if (charClass->m_matchesUnicode.size() || charClass->m_rangesUnicode.size()) {
            Jump isAscii = branch32(LessThanOrEqual, character, TrustedImm32(0x7f));

            if (charClass->m_matchesUnicode.size()) {
                for (unsigned i = 0; i < charClass->m_matchesUnicode.size(); ++i) {
                    UChar ch = charClass->m_matchesUnicode[i];
                    matchDest.append(branch32(Equal, character, Imm32(ch)));
                }
            }

            if (charClass->m_rangesUnicode.size()) {
                for (unsigned i = 0; i < charClass->m_rangesUnicode.size(); ++i) {
                    UChar lo = charClass->m_rangesUnicode[i].begin;
                    UChar hi = charClass->m_rangesUnicode[i].end;

                    Jump below = branch32(LessThan, character, Imm32(lo));
                    matchDest.append(branch32(LessThanOrEqual, character, Imm32(hi)));
                    below.link(this);
                }
            }

            unicodeFail = jump();
            isAscii.link(this);
        }

        if (charClass->m_ranges.size()) {
            unsigned matchIndex = 0;
            JumpList failures;
            matchCharacterClassRange(character, failures, matchDest, charClass->m_ranges.begin(), charClass->m_ranges.size(), &matchIndex, charClass->m_matches.begin(), charClass->m_matches.size());
            while (matchIndex < charClass->m_matches.size())
                matchDest.append(branch32(Equal, character, Imm32((unsigned short)charClass->m_matches[matchIndex++])));

            failures.link(this);
        } else if (charClass->m_matches.size()) {
            
            Vector<char> matchesAZaz;

            for (unsigned i = 0; i < charClass->m_matches.size(); ++i) {
                char ch = charClass->m_matches[i];
                if (m_pattern.m_ignoreCase) {
                    if (isASCIILower(ch)) {
                        matchesAZaz.append(ch);
                        continue;
                    }
                    if (isASCIIUpper(ch))
                        continue;
                }
                matchDest.append(branch32(Equal, character, Imm32((unsigned short)ch)));
            }

            if (unsigned countAZaz = matchesAZaz.size()) {
                or32(TrustedImm32(32), character);
                for (unsigned i = 0; i < countAZaz; ++i)
                    matchDest.append(branch32(Equal, character, TrustedImm32(matchesAZaz[i])));
            }
        }

        if (charClass->m_matchesUnicode.size() || charClass->m_rangesUnicode.size())
            unicodeFail.link(this);
    }

    
    Jump jumpIfNoAvailableInput(unsigned countToCheck = 0)
    {
        if (countToCheck)
            add32(Imm32(countToCheck), index);
        return branch32(Above, index, length);
    }

    Jump jumpIfAvailableInput(unsigned countToCheck)
    {
        add32(Imm32(countToCheck), index);
        return branch32(BelowOrEqual, index, length);
    }

    Jump checkInput()
    {
        return branch32(BelowOrEqual, index, length);
    }

    Jump atEndOfInput()
    {
        return branch32(Equal, index, length);
    }

    Jump notAtEndOfInput()
    {
        return branch32(NotEqual, index, length);
    }

    Jump jumpIfCharEquals(UChar ch, int inputPosition)
    {
        return branch16(Equal, BaseIndex(input, index, TimesTwo, inputPosition * sizeof(UChar)), Imm32(ch));
    }

    Jump jumpIfCharNotEquals(UChar ch, int inputPosition)
    {
        return branch16(NotEqual, BaseIndex(input, index, TimesTwo, inputPosition * sizeof(UChar)), Imm32(ch));
    }

    void readCharacter(int inputPosition, RegisterID reg)
    {
        load16(BaseIndex(input, index, TimesTwo, inputPosition * sizeof(UChar)), reg);
    }

    void storeToFrame(RegisterID reg, unsigned frameLocation)
    {
        poke(reg, frameLocation);
    }

    void storeToFrame(TrustedImm32 imm, unsigned frameLocation)
    {
        poke(imm, frameLocation);
    }

    DataLabelPtr storeToFrameWithPatch(unsigned frameLocation)
    {
        return storePtrWithPatch(TrustedImmPtr(0), Address(stackPointerRegister, frameLocation * sizeof(void*)));
    }

    void loadFromFrame(unsigned frameLocation, RegisterID reg)
    {
        peek(reg, frameLocation);
    }

    void loadFromFrameAndJump(unsigned frameLocation)
    {
        jump(Address(stackPointerRegister, frameLocation * sizeof(void*)));
    }

    enum YarrOpCode {
        
        
        
        
        
        
        OpBodyAlternativeBegin,
        OpBodyAlternativeNext,
        OpBodyAlternativeEnd,
        
        
        OpNestedAlternativeBegin,
        OpNestedAlternativeNext,
        OpNestedAlternativeEnd,
        
        
        
        
        OpSimpleNestedAlternativeBegin,
        OpSimpleNestedAlternativeNext,
        OpSimpleNestedAlternativeEnd,
        
        OpParenthesesSubpatternOnceBegin,
        OpParenthesesSubpatternOnceEnd,
        
        OpParenthesesSubpatternTerminalBegin,
        OpParenthesesSubpatternTerminalEnd,
        
        OpParentheticalAssertionBegin,
        OpParentheticalAssertionEnd,
        
        OpTerm,
        
        
        OpMatchFailed
    };

    
    
    
    struct YarrOp {
        explicit YarrOp(PatternTerm* term)
            : m_op(OpTerm)
            , m_term(term)
            , m_isDeadCode(false)
        {
        }

        explicit YarrOp(YarrOpCode op)
            : m_op(op)
            , m_isDeadCode(false)
        {
        }

        
        YarrOpCode m_op;
        PatternTerm* m_term;

        
        
        
        
        
        PatternAlternative* m_alternative;
        size_t m_previousOp;
        size_t m_nextOp;

        
        
        
        
        Label m_reentry;
        JumpList m_jumps;

        
        
        bool m_isDeadCode;

        
        
        
        int m_checkAdjust;

        
        
        
        DataLabelPtr m_returnAddress;
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    class BacktrackingState {
    public:
        BacktrackingState()
            : m_pendingFallthrough(false)
        {
        }

        
        
        void append(const Jump& jump)
        {
            m_laterFailures.append(jump);
        }
        void append(JumpList& jumpList)
        {
            m_laterFailures.append(jumpList);
        }
        void append(const DataLabelPtr& returnAddress)
        {
            m_pendingReturns.append(returnAddress);
        }
        void fallthrough()
        {
            ASSERT(!m_pendingFallthrough);
            m_pendingFallthrough = true;
        }

        
        
        
        
        void link(MacroAssembler* assembler)
        {
            if (m_pendingReturns.size()) {
                Label here(assembler);
                for (unsigned i = 0; i < m_pendingReturns.size(); ++i)
                    m_backtrackRecords.append(ReturnAddressRecord(m_pendingReturns[i], here));
                m_pendingReturns.clear();
            }
            m_laterFailures.link(assembler);
            m_laterFailures.clear();
            m_pendingFallthrough = false;
        }
        void linkTo(Label label, MacroAssembler* assembler)
        {
            if (m_pendingReturns.size()) {
                for (unsigned i = 0; i < m_pendingReturns.size(); ++i)
                    m_backtrackRecords.append(ReturnAddressRecord(m_pendingReturns[i], label));
                m_pendingReturns.clear();
            }
            if (m_pendingFallthrough)
                assembler->jump(label);
            m_laterFailures.linkTo(label, assembler);
            m_laterFailures.clear();
            m_pendingFallthrough = false;
        }
        void takeBacktracksToJumpList(JumpList& jumpList, MacroAssembler* assembler)
        {
            if (m_pendingReturns.size()) {
                Label here(assembler);
                for (unsigned i = 0; i < m_pendingReturns.size(); ++i)
                    m_backtrackRecords.append(ReturnAddressRecord(m_pendingReturns[i], here));
                m_pendingReturns.clear();
                m_pendingFallthrough = true;
            }
            if (m_pendingFallthrough)
                jumpList.append(assembler->jump());
            jumpList.append(m_laterFailures);
            m_laterFailures.clear();
            m_pendingFallthrough = false;
        }

        bool isEmpty()
        {
            return m_laterFailures.empty() && m_pendingReturns.isEmpty() && !m_pendingFallthrough;
        }

        
        void linkDataLabels(LinkBuffer& linkBuffer)
        {
            ASSERT(isEmpty());
            for (unsigned i = 0; i < m_backtrackRecords.size(); ++i)
                linkBuffer.patch(m_backtrackRecords[i].m_dataLabel, linkBuffer.locationOf(m_backtrackRecords[i].m_backtrackLocation));
        }

    private:
        struct ReturnAddressRecord {
            ReturnAddressRecord(DataLabelPtr dataLabel, Label backtrackLocation)
                : m_dataLabel(dataLabel)
                , m_backtrackLocation(backtrackLocation)
            {
            }

            DataLabelPtr m_dataLabel;
            Label m_backtrackLocation;
        };

        JumpList m_laterFailures;
        bool m_pendingFallthrough;
        Vector<DataLabelPtr, 4> m_pendingReturns;
        Vector<ReturnAddressRecord, 4> m_backtrackRecords;
    };

    
    

    
    
    
    
    void backtrackTermDefault(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        m_backtrackingState.append(op.m_jumps);
    }

    void generateAssertionBOL(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (!term->inputPosition)
                matchDest.append(branch32(Equal, index, Imm32(m_checked)));

            readCharacter((term->inputPosition - m_checked) - 1, character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            op.m_jumps.append(jump());

            matchDest.link(this);
        } else {
            
            if (term->inputPosition)
                op.m_jumps.append(jump());
            else
                op.m_jumps.append(branch32(NotEqual, index, Imm32(m_checked)));
        }
    }
    void backtrackAssertionBOL(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generateAssertionEOL(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (term->inputPosition == m_checked)
                matchDest.append(atEndOfInput());

            readCharacter((term->inputPosition - m_checked), character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            op.m_jumps.append(jump());

            matchDest.link(this);
        } else {
            if (term->inputPosition == m_checked)
                op.m_jumps.append(notAtEndOfInput());
            
            else
                op.m_jumps.append(jump());
        }
    }
    void backtrackAssertionEOL(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    
    void matchAssertionWordchar(size_t opIndex, JumpList& nextIsWordChar, JumpList& nextIsNotWordChar)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        if (term->inputPosition == m_checked)
            nextIsNotWordChar.append(atEndOfInput());

        readCharacter((term->inputPosition - m_checked), character);
        matchCharacterClass(character, nextIsWordChar, m_pattern.wordcharCharacterClass());
    }

    void generateAssertionWordBoundary(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        Jump atBegin;
        JumpList matchDest;
        if (!term->inputPosition)
            atBegin = branch32(Equal, index, Imm32(m_checked));
        readCharacter((term->inputPosition - m_checked) - 1, character);
        matchCharacterClass(character, matchDest, m_pattern.wordcharCharacterClass());
        if (!term->inputPosition)
            atBegin.link(this);

        
        JumpList nonWordCharThenWordChar;
        JumpList nonWordCharThenNonWordChar;
        if (term->invert()) {
            matchAssertionWordchar(opIndex, nonWordCharThenNonWordChar, nonWordCharThenWordChar);
            nonWordCharThenWordChar.append(jump());
        } else {
            matchAssertionWordchar(opIndex, nonWordCharThenWordChar, nonWordCharThenNonWordChar);
            nonWordCharThenNonWordChar.append(jump());
        }
        op.m_jumps.append(nonWordCharThenNonWordChar);

        
        matchDest.link(this);
        JumpList wordCharThenWordChar;
        JumpList wordCharThenNonWordChar;
        if (term->invert()) {
            matchAssertionWordchar(opIndex, wordCharThenNonWordChar, wordCharThenWordChar);
            wordCharThenWordChar.append(jump());
        } else {
            matchAssertionWordchar(opIndex, wordCharThenWordChar, wordCharThenNonWordChar);
            
        }

        op.m_jumps.append(wordCharThenWordChar);

        nonWordCharThenWordChar.link(this);
        wordCharThenNonWordChar.link(this);
    }
    void backtrackAssertionWordBoundary(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generatePatternCharacterOnce(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];

        
        
        ASSERT(opIndex + 1 < m_ops.size());
        YarrOp& nextOp = m_ops[opIndex + 1];

        if (op.m_isDeadCode)
            return;

        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;

        if (nextOp.m_op == OpTerm) {
            PatternTerm* nextTerm = nextOp.m_term;
            if (nextTerm->type == PatternTerm::TypePatternCharacter
                && nextTerm->quantityType == QuantifierFixedCount
                && nextTerm->quantityCount == 1
                && nextTerm->inputPosition == (term->inputPosition + 1)) {

                UChar ch2 = nextTerm->patternCharacter;

                int mask = 0;
                int chPair = ch | (ch2 << 16);

                if (m_pattern.m_ignoreCase) {
                    if (isASCIIAlpha(ch))
                        mask |= 32;
                    if (isASCIIAlpha(ch2))
                        mask |= 32 << 16;
                }

                BaseIndex address(input, index, TimesTwo, (term->inputPosition - m_checked) * sizeof(UChar));
                if (mask) {
                    load32WithUnalignedHalfWords(address, character);
                    or32(Imm32(mask), character);
                    op.m_jumps.append(branch32(NotEqual, character, Imm32(chPair | mask)));
                } else
                    op.m_jumps.append(branch32WithUnalignedHalfWords(NotEqual, address, Imm32(chPair)));

                nextOp.m_isDeadCode = true;
                return;
            }
        }

        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(term->inputPosition - m_checked, character);
            or32(TrustedImm32(32), character);
            op.m_jumps.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            op.m_jumps.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked));
        }
    }
    void backtrackPatternCharacterOnce(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generatePatternCharacterFixed(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(index, countRegister);
        sub32(Imm32(term->quantityCount), countRegister);

        Label loop(this);
        BaseIndex address(input, countRegister, TimesTwo, (term->inputPosition - m_checked + term->quantityCount) * sizeof(UChar));

        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            load16(address, character);
            or32(TrustedImm32(32), character);
            op.m_jumps.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            op.m_jumps.append(branch16(NotEqual, address, Imm32(ch)));
        }
        add32(TrustedImm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
    }
    void backtrackPatternCharacterFixed(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generatePatternCharacterGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);

        JumpList failures;
        Label loop(this);
        failures.append(atEndOfInput());
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(term->inputPosition - m_checked, character);
            or32(TrustedImm32(32), character);
            failures.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            failures.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked));
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);
        if (term->quantityCount == quantifyInfinite)
            jump(loop);
        else
            branch32(NotEqual, countRegister, Imm32(term->quantityCount)).linkTo(loop, this);

        failures.link(this);
        op.m_reentry = label();

        storeToFrame(countRegister, term->frameLocation);

    }
    void backtrackPatternCharacterGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);
        m_backtrackingState.append(branchTest32(Zero, countRegister));
        sub32(TrustedImm32(1), countRegister);
        sub32(TrustedImm32(1), index);
        jump(op.m_reentry);
    }

    void generatePatternCharacterNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);
        op.m_reentry = label();
        storeToFrame(countRegister, term->frameLocation);
    }
    void backtrackPatternCharacterNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        JumpList nonGreedyFailures;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);

        nonGreedyFailures.append(atEndOfInput());
        if (term->quantityCount != quantifyInfinite)
            nonGreedyFailures.append(branch32(Equal, countRegister, Imm32(term->quantityCount)));
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(term->inputPosition - m_checked, character);
            or32(TrustedImm32(32), character);
            nonGreedyFailures.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            nonGreedyFailures.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked));
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);

        jump(op.m_reentry);

        nonGreedyFailures.link(this);
        sub32(countRegister, index);
        m_backtrackingState.fallthrough();
    }

    void generateCharacterClassOnce(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        JumpList matchDest;
        readCharacter((term->inputPosition - m_checked), character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            op.m_jumps.append(matchDest);
        else {
            op.m_jumps.append(jump());
            matchDest.link(this);
        }
    }
    void backtrackCharacterClassOnce(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generateCharacterClassFixed(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(index, countRegister);
        sub32(Imm32(term->quantityCount), countRegister);

        Label loop(this);
        JumpList matchDest;
        load16(BaseIndex(input, countRegister, TimesTwo, (term->inputPosition - m_checked + term->quantityCount) * sizeof(UChar)), character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            op.m_jumps.append(matchDest);
        else {
            op.m_jumps.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
    }
    void backtrackCharacterClassFixed(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generateCharacterClassGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);

        JumpList failures;
        Label loop(this);
        failures.append(atEndOfInput());

        if (term->invert()) {
            readCharacter(term->inputPosition - m_checked, character);
            matchCharacterClass(character, failures, term->characterClass);
        } else {
            JumpList matchDest;
            readCharacter(term->inputPosition - m_checked, character);
            matchCharacterClass(character, matchDest, term->characterClass);
            failures.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);
        if (term->quantityCount != quantifyInfinite) {
            branch32(NotEqual, countRegister, Imm32(term->quantityCount)).linkTo(loop, this);
            failures.append(jump());
        } else
            jump(loop);

        failures.link(this);
        op.m_reentry = label();

        storeToFrame(countRegister, term->frameLocation);
    }
    void backtrackCharacterClassGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);
        m_backtrackingState.append(branchTest32(Zero, countRegister));
        sub32(TrustedImm32(1), countRegister);
        sub32(TrustedImm32(1), index);
        jump(op.m_reentry);
    }

    void generateCharacterClassNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);
        op.m_reentry = label();
        storeToFrame(countRegister, term->frameLocation);
    }
    void backtrackCharacterClassNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        JumpList nonGreedyFailures;

        m_backtrackingState.link(this);

        Label backtrackBegin(this);
        loadFromFrame(term->frameLocation, countRegister);

        nonGreedyFailures.append(atEndOfInput());
        nonGreedyFailures.append(branch32(Equal, countRegister, Imm32(term->quantityCount)));

        JumpList matchDest;
        readCharacter(term->inputPosition - m_checked, character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            nonGreedyFailures.append(matchDest);
        else {
            nonGreedyFailures.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);

        jump(op.m_reentry);

        nonGreedyFailures.link(this);
        sub32(countRegister, index);
        m_backtrackingState.fallthrough();
    }

    
    
    
    void generateTerm(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        switch (term->type) {
        case PatternTerm::TypePatternCharacter:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    generatePatternCharacterOnce(opIndex);
                else
                    generatePatternCharacterFixed(opIndex);
                break;
            case QuantifierGreedy:
                generatePatternCharacterGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                generatePatternCharacterNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    generateCharacterClassOnce(opIndex);
                else
                    generateCharacterClassFixed(opIndex);
                break;
            case QuantifierGreedy:
                generateCharacterClassGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                generateCharacterClassNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeAssertionBOL:
            generateAssertionBOL(opIndex);
            break;

        case PatternTerm::TypeAssertionEOL:
            generateAssertionEOL(opIndex);
            break;

        case PatternTerm::TypeAssertionWordBoundary:
            generateAssertionWordBoundary(opIndex);
            break;

        case PatternTerm::TypeForwardReference:
            break;

        case PatternTerm::TypeParenthesesSubpattern:
        case PatternTerm::TypeParentheticalAssertion:
            ASSERT_NOT_REACHED();
        case PatternTerm::TypeBackReference:
            m_shouldFallBack = true;
            break;
        }
    }
    void backtrackTerm(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        switch (term->type) {
        case PatternTerm::TypePatternCharacter:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    backtrackPatternCharacterOnce(opIndex);
                else
                    backtrackPatternCharacterFixed(opIndex);
                break;
            case QuantifierGreedy:
                backtrackPatternCharacterGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                backtrackPatternCharacterNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    backtrackCharacterClassOnce(opIndex);
                else
                    backtrackCharacterClassFixed(opIndex);
                break;
            case QuantifierGreedy:
                backtrackCharacterClassGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                backtrackCharacterClassNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeAssertionBOL:
            backtrackAssertionBOL(opIndex);
            break;

        case PatternTerm::TypeAssertionEOL:
            backtrackAssertionEOL(opIndex);
            break;

        case PatternTerm::TypeAssertionWordBoundary:
            backtrackAssertionWordBoundary(opIndex);
            break;

        case PatternTerm::TypeForwardReference:
            break;

        case PatternTerm::TypeParenthesesSubpattern:
        case PatternTerm::TypeParentheticalAssertion:
            ASSERT_NOT_REACHED();
        case PatternTerm::TypeBackReference:
            m_shouldFallBack = true;
            break;
        }
    }

    void generate()
    {
        
        ASSERT(m_ops.size());
        size_t opIndex = 0;

        do {
            YarrOp& op = m_ops[opIndex];
            switch (op.m_op) {

            case OpTerm:
                generateTerm(opIndex);
                break;

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            case OpBodyAlternativeBegin: {
                PatternAlternative* alternative = op.m_alternative;

                
                
                op.m_jumps.append(jumpIfNoAvailableInput(alternative->m_minimumSize));
                
                
                op.m_reentry = label();

                m_checked += alternative->m_minimumSize;
                break;
            }
            case OpBodyAlternativeNext:
            case OpBodyAlternativeEnd: {
                PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                PatternAlternative* alternative = op.m_alternative;

                
                
                
                if (m_pattern.m_body->m_callFrameSize)
                    addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

                
                
                
                ASSERT(index != returnRegister);
                if (m_pattern.m_body->m_hasFixedSize) {
                    move(index, returnRegister);
                    if (priorAlternative->m_minimumSize)
                        sub32(Imm32(priorAlternative->m_minimumSize), returnRegister);
                    store32(returnRegister, output);
                } else
                    load32(Address(output), returnRegister);
                store32(index, Address(output, 4));
                generateReturn();

                
                

                if (op.m_op == OpBodyAlternativeNext) {
                    
                    
                    
                    
                    op.m_reentry = label();
                    if (int delta = alternative->m_minimumSize - priorAlternative->m_minimumSize) {
                        add32(Imm32(delta), index);
                        if (delta > 0)
                            op.m_jumps.append(jumpIfNoAvailableInput());
                    }
                } else if (op.m_nextOp == notFound) {
                    
                    
                    op.m_reentry = label();
                    sub32(Imm32(priorAlternative->m_minimumSize), index);
                }

                if (op.m_op == OpBodyAlternativeNext)
                    m_checked += alternative->m_minimumSize;
                m_checked -= priorAlternative->m_minimumSize;
                break;
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            case OpSimpleNestedAlternativeBegin:
            case OpNestedAlternativeBegin: {
                PatternTerm* term = op.m_term;
                PatternAlternative* alternative = op.m_alternative;
                PatternDisjunction* disjunction = term->parentheses.disjunction;

                
                op.m_checkAdjust = alternative->m_minimumSize;
                if ((term->quantityType == QuantifierFixedCount) && (term->type != PatternTerm::TypeParentheticalAssertion))
                    op.m_checkAdjust -= disjunction->m_minimumSize;
                if (op.m_checkAdjust)
                    op.m_jumps.append(jumpIfNoAvailableInput(op.m_checkAdjust));
 
                m_checked += op.m_checkAdjust;
                break;
            }
            case OpSimpleNestedAlternativeNext:
            case OpNestedAlternativeNext: {
                PatternTerm* term = op.m_term;
                PatternAlternative* alternative = op.m_alternative;
                PatternDisjunction* disjunction = term->parentheses.disjunction;

                
                if (op.m_op == OpNestedAlternativeNext) {
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    unsigned alternativeFrameLocation = parenthesesFrameLocation;
                    if (term->quantityType != QuantifierFixedCount)
                        alternativeFrameLocation += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    op.m_returnAddress = storeToFrameWithPatch(alternativeFrameLocation);
                }

                
                
                
                
                
                
                
                
                YarrOp* endOp = &m_ops[op.m_nextOp];
                while (endOp->m_nextOp != notFound) {
                    ASSERT(endOp->m_op == OpSimpleNestedAlternativeNext || endOp->m_op == OpNestedAlternativeNext);
                    endOp = &m_ops[endOp->m_nextOp];
                }
                ASSERT(endOp->m_op == OpSimpleNestedAlternativeEnd || endOp->m_op == OpNestedAlternativeEnd);
                endOp->m_jumps.append(jump());

                
                op.m_reentry = label();

                
                op.m_checkAdjust = alternative->m_minimumSize;
                if ((term->quantityType == QuantifierFixedCount) && (term->type != PatternTerm::TypeParentheticalAssertion))
                    op.m_checkAdjust -= disjunction->m_minimumSize;
                if (op.m_checkAdjust)
                    op.m_jumps.append(jumpIfNoAvailableInput(op.m_checkAdjust));

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked -= lastOp.m_checkAdjust;
                m_checked += op.m_checkAdjust;
                break;
            }
            case OpSimpleNestedAlternativeEnd:
            case OpNestedAlternativeEnd: {
                PatternTerm* term = op.m_term;

                
                if (op.m_op == OpNestedAlternativeEnd) {
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    unsigned alternativeFrameLocation = parenthesesFrameLocation;
                    if (term->quantityType != QuantifierFixedCount)
                        alternativeFrameLocation += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    op.m_returnAddress = storeToFrameWithPatch(alternativeFrameLocation);
                }

                
                
                
                op.m_jumps.link(this);
                op.m_jumps.clear();

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked -= lastOp.m_checkAdjust;
                break;
            }

            
            
            
            
            case OpParenthesesSubpatternOnceBegin: {
                PatternTerm* term = op.m_term;
                unsigned parenthesesFrameLocation = term->frameLocation;
                const RegisterID indexTemporary = regT0;
                ASSERT(term->quantityCount == 1);

                
                
                
                
                
                
                
                
                
                
                
                
                
                
                if (term->quantityType == QuantifierGreedy)
                    storeToFrame(index, parenthesesFrameLocation);
                else if (term->quantityType == QuantifierNonGreedy) {
                    storeToFrame(TrustedImm32(-1), parenthesesFrameLocation);
                    op.m_jumps.append(jump());
                    op.m_reentry = label();
                    storeToFrame(index, parenthesesFrameLocation);
                }

                
                
                
                
                
                
                if (term->capture()) {
                    int offsetId = term->parentheses.subpatternId << 1;
                    int inputOffset = term->inputPosition - m_checked;
                    if (term->quantityType == QuantifierFixedCount)
                        inputOffset -= term->parentheses.disjunction->m_minimumSize;
                    if (inputOffset) {
                        move(index, indexTemporary);
                        add32(Imm32(inputOffset), indexTemporary);
                        store32(indexTemporary, Address(output, offsetId * sizeof(int)));
                    } else
                        store32(index, Address(output, offsetId * sizeof(int)));
                }
                break;
            }
            case OpParenthesesSubpatternOnceEnd: {
                PatternTerm* term = op.m_term;
                unsigned parenthesesFrameLocation = term->frameLocation;
                const RegisterID indexTemporary = regT0;
                ASSERT(term->quantityCount == 1);

                
                
                if (term->quantityType != QuantifierFixedCount && !term->parentheses.disjunction->m_minimumSize)
                    op.m_jumps.append(branch32(Equal, index, Address(stackPointerRegister, parenthesesFrameLocation * sizeof(void*))));

                
                
                
                
                
                
                if (term->capture()) {
                    int offsetId = (term->parentheses.subpatternId << 1) + 1;
                    int inputOffset = term->inputPosition - m_checked;
                    if (inputOffset) {
                        move(index, indexTemporary);
                        add32(Imm32(inputOffset), indexTemporary);
                        store32(indexTemporary, Address(output, offsetId * sizeof(int)));
                    } else
                        store32(index, Address(output, offsetId * sizeof(int)));
                }

                
                
                
                if (term->quantityType == QuantifierGreedy)
                    op.m_reentry = label();
                else if (term->quantityType == QuantifierNonGreedy) {
                    YarrOp& beginOp = m_ops[op.m_previousOp];
                    beginOp.m_jumps.link(this);
                }
                break;
            }

            
            case OpParenthesesSubpatternTerminalBegin: {
                PatternTerm* term = op.m_term;
                ASSERT(term->quantityType == QuantifierGreedy);
                ASSERT(term->quantityCount == quantifyInfinite);
                ASSERT(!term->capture());

                
                op.m_reentry = label();

                
                
                storeToFrame(index, term->frameLocation);
                break;
            }
            case OpParenthesesSubpatternTerminalEnd: {
                PatternTerm* term = op.m_term;

                
                
                YarrOp& beginOp = m_ops[op.m_previousOp];
                branch32(NotEqual, index, Address(stackPointerRegister, term->frameLocation * sizeof(void*)), beginOp.m_reentry);

                
                op.m_jumps.append(jump());

                
                
                op.m_reentry = label();
                break;
            }

            
            case OpParentheticalAssertionBegin: {
                PatternTerm* term = op.m_term;

                
                
                unsigned parenthesesFrameLocation = term->frameLocation;
                storeToFrame(index, parenthesesFrameLocation);

                
                op.m_checkAdjust = m_checked - term->inputPosition;
                if (op.m_checkAdjust)
                    sub32(Imm32(op.m_checkAdjust), index);

                m_checked -= op.m_checkAdjust;
                break;
            }
            case OpParentheticalAssertionEnd: {
                PatternTerm* term = op.m_term;

                
                unsigned parenthesesFrameLocation = term->frameLocation;
                loadFromFrame(parenthesesFrameLocation, index);

                
                
                if (term->invert()) {
                    op.m_jumps.append(jump());
                    op.m_reentry = label();
                }

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked += lastOp.m_checkAdjust;
                break;
            }

            case OpMatchFailed:
                if (m_pattern.m_body->m_callFrameSize)
                    addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);
                move(TrustedImm32(-1), returnRegister);
                generateReturn();
                break;
            }

            ++opIndex;
        } while (opIndex < m_ops.size());
    }

    void backtrack()
    {
        
        size_t opIndex = m_ops.size();
        ASSERT(opIndex);

        do {
            --opIndex;
            YarrOp& op = m_ops[opIndex];
            switch (op.m_op) {

            case OpTerm:
                backtrackTerm(opIndex);
                break;

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            case OpBodyAlternativeBegin:
            case OpBodyAlternativeNext: {
                PatternAlternative* alternative = op.m_alternative;

                if (op.m_op == OpBodyAlternativeNext) {
                    PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                    m_checked += priorAlternative->m_minimumSize;
                }
                m_checked -= alternative->m_minimumSize;

                
                
                if (m_ops[op.m_nextOp].m_op != OpBodyAlternativeEnd) {
                    m_backtrackingState.linkTo(m_ops[op.m_nextOp].m_reentry, this);
                    break;
                }
                YarrOp& endOp = m_ops[op.m_nextOp];

                YarrOp* beginOp = &op;
                while (beginOp->m_op != OpBodyAlternativeBegin) {
                    ASSERT(beginOp->m_op == OpBodyAlternativeNext);
                    beginOp = &m_ops[beginOp->m_previousOp];
                }

                bool onceThrough = endOp.m_nextOp == notFound;

                
                
                
                if (onceThrough)
                    m_backtrackingState.linkTo(endOp.m_reentry, this);
                else {
                    
                    
                    
                    
                    
                    
                    int deltaLastAlternativeToFirstAlternativePlusOne = (beginOp->m_alternative->m_minimumSize - alternative->m_minimumSize) + 1;

                    
                    
                    
                    
                    if (!deltaLastAlternativeToFirstAlternativePlusOne && m_pattern.m_body->m_hasFixedSize)
                        m_backtrackingState.linkTo(beginOp->m_reentry, this);
                    else {
                        
                        
                        m_backtrackingState.link(this);

                        
                        if (!m_pattern.m_body->m_hasFixedSize) {
                            if (alternative->m_minimumSize == 1)
                                store32(index, Address(output));
                            else {
                                move(index, regT0);
                                if (alternative->m_minimumSize)
                                    sub32(Imm32(alternative->m_minimumSize - 1), regT0);
                                else
                                    add32(Imm32(1), regT0);
                                store32(regT0, Address(output));
                            }
                        }

                        if (deltaLastAlternativeToFirstAlternativePlusOne)
                            add32(Imm32(deltaLastAlternativeToFirstAlternativePlusOne), index);

                        
                        
                        
                        
                        
                        
                        if (deltaLastAlternativeToFirstAlternativePlusOne > 0)
                            checkInput().linkTo(beginOp->m_reentry, this);
                        else
                            jump(beginOp->m_reentry);
                    }
                }

                
                
                
                
                
                
                
                Label firstInputCheckFailed(this);

                
                
                
                
                
                
                YarrOp* prevOp = beginOp;
                YarrOp* nextOp = &m_ops[beginOp->m_nextOp];
                while (nextOp->m_op != OpBodyAlternativeEnd) {
                    prevOp->m_jumps.link(this);

                    int delta = nextOp->m_alternative->m_minimumSize - prevOp->m_alternative->m_minimumSize;
                    if (delta)
                        add32(Imm32(delta), index);

                    
                    
                    if (delta < 0) {
                        
                        
                        
                        Jump fail = jumpIfNoAvailableInput();
                        sub32(Imm32(delta), index);
                        jump(nextOp->m_reentry);
                        fail.link(this);
                    }
                    prevOp = nextOp;
                    nextOp = &m_ops[nextOp->m_nextOp];
                }

                

                
                
                if (onceThrough) {
                    op.m_jumps.linkTo(endOp.m_reentry, this);
                    jump(endOp.m_reentry);
                    break;
                }

                
                
                op.m_jumps.link(this);

                bool needsToUpdateMatchStart = !m_pattern.m_body->m_hasFixedSize;

                
                
                
                if (needsToUpdateMatchStart && alternative->m_minimumSize == 1) {
                    
                    store32(index, Address(output));
                    needsToUpdateMatchStart = false;
                }

                
                
                
                
                int deltaLastAlternativeToBodyMinimumPlusOne = (m_pattern.m_body->m_minimumSize + 1) - alternative->m_minimumSize;
                if (deltaLastAlternativeToBodyMinimumPlusOne)
                    add32(Imm32(deltaLastAlternativeToBodyMinimumPlusOne), index);
                Jump matchFailed = jumpIfNoAvailableInput();

                if (needsToUpdateMatchStart) {
                    if (!m_pattern.m_body->m_minimumSize)
                        store32(index, Address(output));
                    else {
                        move(index, regT0);
                        sub32(Imm32(m_pattern.m_body->m_minimumSize), regT0);
                        store32(regT0, Address(output));
                    }
                }

                
                
                
                int deltaBodyMinimumToFirstAlternative = beginOp->m_alternative->m_minimumSize - m_pattern.m_body->m_minimumSize;
                if (!deltaBodyMinimumToFirstAlternative)
                    jump(beginOp->m_reentry);
                else {
                    add32(Imm32(deltaBodyMinimumToFirstAlternative), index);
                    checkInput().linkTo(beginOp->m_reentry, this);
                    jump(firstInputCheckFailed);
                }

                
                
                matchFailed.link(this);

                if (m_pattern.m_body->m_callFrameSize)
                    addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);
                move(TrustedImm32(-1), returnRegister);
                generateReturn();
                break;
            }
            case OpBodyAlternativeEnd: {
                
                ASSERT(m_backtrackingState.isEmpty());

                PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                m_checked += priorAlternative->m_minimumSize;
                break;
            }

            
            
            
            
            
            
            
            
            
            
            
            case OpSimpleNestedAlternativeBegin:
            case OpSimpleNestedAlternativeNext:
            case OpNestedAlternativeBegin:
            case OpNestedAlternativeNext: {
                YarrOp& nextOp = m_ops[op.m_nextOp];
                bool isBegin = op.m_previousOp == notFound;
                bool isLastAlternative = nextOp.m_nextOp == notFound;
                ASSERT(isBegin == (op.m_op == OpSimpleNestedAlternativeBegin || op.m_op == OpNestedAlternativeBegin));
                ASSERT(isLastAlternative == (nextOp.m_op == OpSimpleNestedAlternativeEnd || nextOp.m_op == OpNestedAlternativeEnd));

                
                m_backtrackingState.append(op.m_jumps);

                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                if (op.m_checkAdjust) {
                    
                    m_backtrackingState.link(this);
                    sub32(Imm32(op.m_checkAdjust), index);
                    if (!isLastAlternative) {
                        
                        jump(nextOp.m_reentry);
                    } else if (!isBegin) {
                        
                        nextOp.m_jumps.append(jump());
                    } else {
                        
                        m_backtrackingState.fallthrough();
                    }
                } else {
                    
                    if (!isLastAlternative) {
                        
                        m_backtrackingState.linkTo(nextOp.m_reentry, this);
                    } else if (!isBegin) {
                        
                        m_backtrackingState.takeBacktracksToJumpList(nextOp.m_jumps, this);
                    }
                    
                }

                
                

                
                
                if (op.m_op == OpNestedAlternativeNext)
                    m_backtrackingState.append(op.m_returnAddress);

                
                
                
                if (isBegin) {
                    YarrOp* endOp = &m_ops[op.m_nextOp];
                    while (endOp->m_nextOp != notFound) {
                        ASSERT(endOp->m_op == OpSimpleNestedAlternativeNext || endOp->m_op == OpNestedAlternativeNext);
                        endOp = &m_ops[endOp->m_nextOp];
                    }
                    ASSERT(endOp->m_op == OpSimpleNestedAlternativeEnd || endOp->m_op == OpNestedAlternativeEnd);
                    m_backtrackingState.append(endOp->m_jumps);
                }

                if (!isBegin) {
                    YarrOp& lastOp = m_ops[op.m_previousOp];
                    m_checked += lastOp.m_checkAdjust;
                }
                m_checked -= op.m_checkAdjust;
                break;
            }
            case OpSimpleNestedAlternativeEnd:
            case OpNestedAlternativeEnd: {
                PatternTerm* term = op.m_term;

                
                
                
                
                if (op.m_op == OpNestedAlternativeEnd) {
                    m_backtrackingState.link(this);

                    
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    unsigned alternativeFrameLocation = parenthesesFrameLocation;
                    if (term->quantityType != QuantifierFixedCount)
                        alternativeFrameLocation += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    loadFromFrameAndJump(alternativeFrameLocation);

                    
                    
                    m_backtrackingState.append(op.m_returnAddress);
                }

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked += lastOp.m_checkAdjust;
                break;
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            case OpParenthesesSubpatternOnceBegin: {
                PatternTerm* term = op.m_term;
                ASSERT(term->quantityCount == 1);

                
                if (term->capture() || term->quantityType == QuantifierGreedy) {
                    m_backtrackingState.link(this);

                    
                    if (term->capture())
                        store32(TrustedImm32(-1), Address(output, (term->parentheses.subpatternId << 1) * sizeof(int)));

                    
                    if (term->quantityType == QuantifierGreedy) {
                        
                        unsigned parenthesesFrameLocation = term->frameLocation;
                        storeToFrame(TrustedImm32(-1), parenthesesFrameLocation);
                        
                        jump(m_ops[op.m_nextOp].m_reentry);
                        
                        
                        op.m_jumps.link(this);
                    }

                    m_backtrackingState.fallthrough();
                }
                break;
            }
            case OpParenthesesSubpatternOnceEnd: {
                PatternTerm* term = op.m_term;

                if (term->quantityType != QuantifierFixedCount) {
                    m_backtrackingState.link(this);

                    
                    
                    
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    Jump hadSkipped = branch32(Equal, Address(stackPointerRegister, parenthesesFrameLocation * sizeof(void*)), TrustedImm32(-1));

                    if (term->quantityType == QuantifierGreedy) {
                        
                        
                        YarrOp& beginOp = m_ops[op.m_previousOp];
                        beginOp.m_jumps.append(hadSkipped);
                    } else {
                        
                        
                        
                        
                        ASSERT(term->quantityType == QuantifierNonGreedy);
                        YarrOp& beginOp = m_ops[op.m_previousOp];
                        hadSkipped.linkTo(beginOp.m_reentry, this);
                    }

                    m_backtrackingState.fallthrough();
                }

                m_backtrackingState.append(op.m_jumps);
                break;
            }

            
            
            
            
            
            case OpParenthesesSubpatternTerminalBegin: {
                
                
                
                
                YarrOp& endOp = m_ops[op.m_nextOp];
                m_backtrackingState.linkTo(endOp.m_reentry, this);
                break;
            }
            case OpParenthesesSubpatternTerminalEnd:
                
                ASSERT(m_backtrackingState.isEmpty());
                m_backtrackingState.append(op.m_jumps);
                break;

            
            case OpParentheticalAssertionBegin: {
                PatternTerm* term = op.m_term;
                YarrOp& endOp = m_ops[op.m_nextOp];

                
                
                
                if (op.m_checkAdjust || term->invert()) {
                     m_backtrackingState.link(this);

                    if (op.m_checkAdjust)
                        add32(Imm32(op.m_checkAdjust), index);

                    
                    
                    
                    
                    if (term->invert())
                        jump(endOp.m_reentry);

                    m_backtrackingState.fallthrough();
                }

                
                
                
                m_backtrackingState.append(endOp.m_jumps);

                m_checked += op.m_checkAdjust;
                break;
            }
            case OpParentheticalAssertionEnd: {
                
                
                

                
                m_backtrackingState.takeBacktracksToJumpList(op.m_jumps, this);

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked -= lastOp.m_checkAdjust;
                break;
            }

            case OpMatchFailed:
                break;
            }

        } while (opIndex);
    }

    
    

    
    
    
    
    
    
    
    
    
    
    void opCompileParenthesesSubpattern(PatternTerm* term)
    {
        YarrOpCode parenthesesBeginOpCode;
        YarrOpCode parenthesesEndOpCode;
        YarrOpCode alternativeBeginOpCode = OpSimpleNestedAlternativeBegin;
        YarrOpCode alternativeNextOpCode = OpSimpleNestedAlternativeNext;
        YarrOpCode alternativeEndOpCode = OpSimpleNestedAlternativeEnd;

        
        
        
        
        
        
        
        if (term->quantityCount == 1 && !term->parentheses.isCopy) {
            
            parenthesesBeginOpCode = OpParenthesesSubpatternOnceBegin;
            parenthesesEndOpCode = OpParenthesesSubpatternOnceEnd;

            
            if (term->parentheses.disjunction->m_alternatives.size() != 1) {
                alternativeBeginOpCode = OpNestedAlternativeBegin;
                alternativeNextOpCode = OpNestedAlternativeNext;
                alternativeEndOpCode = OpNestedAlternativeEnd;
            }
        } else if (term->parentheses.isTerminal) {
            
            parenthesesBeginOpCode = OpParenthesesSubpatternTerminalBegin;
            parenthesesEndOpCode = OpParenthesesSubpatternTerminalEnd;
        } else {
            
            m_shouldFallBack = true;
            return;
        }

        size_t parenBegin = m_ops.size();
        m_ops.append(parenthesesBeginOpCode);

        m_ops.append(alternativeBeginOpCode);
        m_ops.last().m_previousOp = notFound;
        m_ops.last().m_term = term;
        Vector<PatternAlternative*>& alternatives =  term->parentheses.disjunction->m_alternatives;
        for (unsigned i = 0; i < alternatives.size(); ++i) {
            size_t lastOpIndex = m_ops.size() - 1;

            PatternAlternative* nestedAlternative = alternatives[i];
            opCompileAlternative(nestedAlternative);

            size_t thisOpIndex = m_ops.size();
            m_ops.append(YarrOp(alternativeNextOpCode));

            YarrOp& lastOp = m_ops[lastOpIndex];
            YarrOp& thisOp = m_ops[thisOpIndex];

            lastOp.m_alternative = nestedAlternative;
            lastOp.m_nextOp = thisOpIndex;
            thisOp.m_previousOp = lastOpIndex;
            thisOp.m_term = term;
        }
        YarrOp& lastOp = m_ops.last();
        ASSERT(lastOp.m_op == alternativeNextOpCode);
        lastOp.m_op = alternativeEndOpCode;
        lastOp.m_alternative = 0;
        lastOp.m_nextOp = notFound;

        size_t parenEnd = m_ops.size();
        m_ops.append(parenthesesEndOpCode);

        m_ops[parenBegin].m_term = term;
        m_ops[parenBegin].m_previousOp = notFound;
        m_ops[parenBegin].m_nextOp = parenEnd;
        m_ops[parenEnd].m_term = term;
        m_ops[parenEnd].m_previousOp = parenBegin;
        m_ops[parenEnd].m_nextOp = notFound;
    }

    
    
    
    
    
    
    
    
    void opCompileParentheticalAssertion(PatternTerm* term)
    {
        size_t parenBegin = m_ops.size();
        m_ops.append(OpParentheticalAssertionBegin);

        m_ops.append(OpSimpleNestedAlternativeBegin);
        m_ops.last().m_previousOp = notFound;
        m_ops.last().m_term = term;
        Vector<PatternAlternative*>& alternatives =  term->parentheses.disjunction->m_alternatives;
        for (unsigned i = 0; i < alternatives.size(); ++i) {
            size_t lastOpIndex = m_ops.size() - 1;

            PatternAlternative* nestedAlternative = alternatives[i];
            opCompileAlternative(nestedAlternative);

            size_t thisOpIndex = m_ops.size();
            m_ops.append(YarrOp(OpSimpleNestedAlternativeNext));

            YarrOp& lastOp = m_ops[lastOpIndex];
            YarrOp& thisOp = m_ops[thisOpIndex];

            lastOp.m_alternative = nestedAlternative;
            lastOp.m_nextOp = thisOpIndex;
            thisOp.m_previousOp = lastOpIndex;
            thisOp.m_term = term;
        }
        YarrOp& lastOp = m_ops.last();
        ASSERT(lastOp.m_op == OpSimpleNestedAlternativeNext);
        lastOp.m_op = OpSimpleNestedAlternativeEnd;
        lastOp.m_alternative = 0;
        lastOp.m_nextOp = notFound;

        size_t parenEnd = m_ops.size();
        m_ops.append(OpParentheticalAssertionEnd);

        m_ops[parenBegin].m_term = term;
        m_ops[parenBegin].m_previousOp = notFound;
        m_ops[parenBegin].m_nextOp = parenEnd;
        m_ops[parenEnd].m_term = term;
        m_ops[parenEnd].m_previousOp = parenBegin;
        m_ops[parenEnd].m_nextOp = notFound;
    }

    
    
    void opCompileAlternative(PatternAlternative* alternative)
    {
        optimizeAlternative(alternative);

        for (unsigned i = 0; i < alternative->m_terms.size(); ++i) {
            PatternTerm* term = &alternative->m_terms[i];

            switch (term->type) {
            case PatternTerm::TypeParenthesesSubpattern:
                opCompileParenthesesSubpattern(term);
                break;

            case PatternTerm::TypeParentheticalAssertion:
                opCompileParentheticalAssertion(term);
                break;

            default:
                m_ops.append(term);
            }
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    void opCompileBody(PatternDisjunction* disjunction)
    {
        Vector<PatternAlternative*>& alternatives =  disjunction->m_alternatives;
        size_t currentAlternativeIndex = 0;

        
        if (alternatives.size() && alternatives[0]->onceThrough()) {
            m_ops.append(YarrOp(OpBodyAlternativeBegin));
            m_ops.last().m_previousOp = notFound;

            do {
                size_t lastOpIndex = m_ops.size() - 1;
                PatternAlternative* alternative = alternatives[currentAlternativeIndex];
                opCompileAlternative(alternative);

                size_t thisOpIndex = m_ops.size();
                m_ops.append(YarrOp(OpBodyAlternativeNext));

                YarrOp& lastOp = m_ops[lastOpIndex];
                YarrOp& thisOp = m_ops[thisOpIndex];

                lastOp.m_alternative = alternative;
                lastOp.m_nextOp = thisOpIndex;
                thisOp.m_previousOp = lastOpIndex;
                
                ++currentAlternativeIndex;
            } while (currentAlternativeIndex < alternatives.size() && alternatives[currentAlternativeIndex]->onceThrough());

            YarrOp& lastOp = m_ops.last();

            ASSERT(lastOp.m_op == OpBodyAlternativeNext);
            lastOp.m_op = OpBodyAlternativeEnd;
            lastOp.m_alternative = 0;
            lastOp.m_nextOp = notFound;
        }

        if (currentAlternativeIndex == alternatives.size()) {
            m_ops.append(YarrOp(OpMatchFailed));
            return;
        }

        
        size_t repeatLoop = m_ops.size();
        m_ops.append(YarrOp(OpBodyAlternativeBegin));
        m_ops.last().m_previousOp = notFound;
        do {
            size_t lastOpIndex = m_ops.size() - 1;
            PatternAlternative* alternative = alternatives[currentAlternativeIndex];
            ASSERT(!alternative->onceThrough());
            opCompileAlternative(alternative);

            size_t thisOpIndex = m_ops.size();
            m_ops.append(YarrOp(OpBodyAlternativeNext));

            YarrOp& lastOp = m_ops[lastOpIndex];
            YarrOp& thisOp = m_ops[thisOpIndex];

            lastOp.m_alternative = alternative;
            lastOp.m_nextOp = thisOpIndex;
            thisOp.m_previousOp = lastOpIndex;
            
            ++currentAlternativeIndex;
        } while (currentAlternativeIndex < alternatives.size());
        YarrOp& lastOp = m_ops.last();
        ASSERT(lastOp.m_op == OpBodyAlternativeNext);
        lastOp.m_op = OpBodyAlternativeEnd;
        lastOp.m_alternative = 0;
        lastOp.m_nextOp = repeatLoop;
    }

    void generateEnter()
    {
#if WTF_CPU_X86_64
        push(X86Registers::ebp);
        move(stackPointerRegister, X86Registers::ebp);
        push(X86Registers::ebx);
#elif WTF_CPU_X86
        push(X86Registers::ebp);
        move(stackPointerRegister, X86Registers::ebp);
        
        push(X86Registers::ebx);
        push(X86Registers::edi);
        push(X86Registers::esi);
        
    #if WTF_COMPILER_MSVC
        loadPtr(Address(X86Registers::ebp, 2 * sizeof(void*)), input);
        loadPtr(Address(X86Registers::ebp, 3 * sizeof(void*)), index);
        loadPtr(Address(X86Registers::ebp, 4 * sizeof(void*)), length);
        loadPtr(Address(X86Registers::ebp, 5 * sizeof(void*)), output);
    #else
        loadPtr(Address(X86Registers::ebp, 2 * sizeof(void*)), output);
    #endif
#elif WTF_CPU_ARM
        push(ARMRegisters::r4);
        push(ARMRegisters::r5);
        push(ARMRegisters::r6);
#if WTF_CPU_ARM_TRADITIONAL
        push(ARMRegisters::r8); 
#endif
        move(ARMRegisters::r3, output);
#elif WTF_CPU_SH4
        push(SH4Registers::r11);
        push(SH4Registers::r13);
#elif WTF_CPU_MIPS
        
#endif
    }

    void generateReturn()
    {
#if WTF_CPU_X86_64
        pop(X86Registers::ebx);
        pop(X86Registers::ebp);
#elif WTF_CPU_X86
        pop(X86Registers::esi);
        pop(X86Registers::edi);
        pop(X86Registers::ebx);
        pop(X86Registers::ebp);
#elif WTF_CPU_ARM
#if WTF_CPU_ARM_TRADITIONAL
        pop(ARMRegisters::r8); 
#endif
        pop(ARMRegisters::r6);
        pop(ARMRegisters::r5);
        pop(ARMRegisters::r4);
#elif WTF_CPU_SH4
        pop(SH4Registers::r13);
        pop(SH4Registers::r11);
#elif WTF_CPU_MIPS
        
#endif
        ret();
    }

public:
    YarrGenerator(YarrPattern& pattern)
        : m_pattern(pattern)
        , m_shouldFallBack(false)
        , m_checked(0)
    {
    }

    void compile(JSGlobalData* globalData, YarrCodeBlock& jitObject)
    {
        generateEnter();

        if (!m_pattern.m_body->m_hasFixedSize)
            store32(index, Address(output));

        if (m_pattern.m_body->m_callFrameSize)
            subPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

        
        opCompileBody(m_pattern.m_body);

        
        
        if (m_shouldFallBack) {
            jitObject.setFallBack(true);
            return;
        }

        generate();
        backtrack();

        
        
        ExecutablePool *pool;
        bool ok;
        LinkBuffer linkBuffer(this, globalData->regexAllocator, &pool, &ok);
        m_backtrackingState.linkDataLabels(linkBuffer);
        jitObject.set(linkBuffer.finalizeCode());
        jitObject.setFallBack(m_shouldFallBack);
    }

private:
    YarrPattern& m_pattern;

    
    
    bool m_shouldFallBack;

    
    Vector<YarrOp, 128> m_ops;

    
    
    
    
    
    
    
    
    
    
    int m_checked;

    
    BacktrackingState m_backtrackingState;
};

void jitCompile(YarrPattern& pattern, JSGlobalData* globalData, YarrCodeBlock& jitObject)
{
    YarrGenerator(pattern).compile(globalData, jitObject);
}

int execute(YarrCodeBlock& jitObject, const UChar* input, unsigned start, unsigned length, int* output)
{
    return jitObject.execute(input, start, length, output);
}

}}

#endif
