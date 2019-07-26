


























#include "yarr/YarrJIT.h"

#include "assembler/assembler/LinkBuffer.h"
#include "yarr/Yarr.h"
#include "yarr/YarrCanonicalizeUCS2.h"

#if JS_ION

using namespace WTF;

namespace JSC { namespace Yarr {

template<YarrJITCompileMode compileMode>
class YarrGenerator : private MacroAssembler {
    friend void jitCompile(JSGlobalData*, YarrCodeBlock& jitObject, const String& pattern, unsigned& numSubpatterns, const char*& error, bool ignoreCase, bool multiline);

#if WTF_CPU_ARM
    static const RegisterID input = ARMRegisters::r0;
    static const RegisterID index = ARMRegisters::r1;
    static const RegisterID length = ARMRegisters::r2;
    static const RegisterID output = ARMRegisters::r4;

    static const RegisterID regT0 = ARMRegisters::r5;
    static const RegisterID regT1 = ARMRegisters::r6;

    static const RegisterID returnRegister = ARMRegisters::r0;
    static const RegisterID returnRegister2 = ARMRegisters::r1;
#elif WTF_CPU_MIPS
    static const RegisterID input = MIPSRegisters::a0;
    static const RegisterID index = MIPSRegisters::a1;
    static const RegisterID length = MIPSRegisters::a2;
    static const RegisterID output = MIPSRegisters::a3;

    static const RegisterID regT0 = MIPSRegisters::t4;
    static const RegisterID regT1 = MIPSRegisters::t5;

    static const RegisterID returnRegister = MIPSRegisters::v0;
    static const RegisterID returnRegister2 = MIPSRegisters::v1;
#elif WTF_CPU_SH4
    static const RegisterID input = SH4Registers::r4;
    static const RegisterID index = SH4Registers::r5;
    static const RegisterID length = SH4Registers::r6;
    static const RegisterID output = SH4Registers::r7;

    static const RegisterID regT0 = SH4Registers::r0;
    static const RegisterID regT1 = SH4Registers::r1;

    static const RegisterID returnRegister = SH4Registers::r0;
    static const RegisterID returnRegister2 = SH4Registers::r1;
#elif WTF_CPU_SPARC
    static const RegisterID input = SparcRegisters::i0;
    static const RegisterID index = SparcRegisters::i1;
    static const RegisterID length = SparcRegisters::i2;
    static const RegisterID output = SparcRegisters::i3;

    static const RegisterID regT0 = SparcRegisters::i4;
    static const RegisterID regT1 = SparcRegisters::i5;

    static const RegisterID returnRegister = SparcRegisters::i0;
#elif WTF_CPU_X86
    static const RegisterID input = X86Registers::eax;
    static const RegisterID index = X86Registers::edx;
    static const RegisterID length = X86Registers::ecx;
    static const RegisterID output = X86Registers::edi;

    static const RegisterID regT0 = X86Registers::ebx;
    static const RegisterID regT1 = X86Registers::esi;

    static const RegisterID returnRegister = X86Registers::eax;
    static const RegisterID returnRegister2 = X86Registers::edx;
#elif WTF_CPU_X86_64
# if WTF_PLATFORM_WIN
    static const RegisterID input = X86Registers::ecx;
    static const RegisterID index = X86Registers::edx;
    static const RegisterID length = X86Registers::r8;
    static const RegisterID output = X86Registers::r9;
# else
    static const RegisterID input = X86Registers::edi;
    static const RegisterID index = X86Registers::esi;
    static const RegisterID length = X86Registers::edx;
    static const RegisterID output = X86Registers::ecx;
# endif

    static const RegisterID regT0 = X86Registers::eax;
    static const RegisterID regT1 = X86Registers::ebx;

    static const RegisterID returnRegister = X86Registers::eax;

# if !WTF_PLATFORM_WIN
    
    static const RegisterID returnRegister2 = X86Registers::edx;
# endif
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
            ExtendedAddress tableEntry(character, reinterpret_cast<intptr_t>(charClass->m_table));
            matchDest.append(branchTest8(charClass->m_tableInverted ? Zero : NonZero, tableEntry));
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

    Jump jumpIfCharNotEquals(UChar ch, int inputPosition, RegisterID character)
    {
        readCharacter(inputPosition, character);

        
        
        ASSERT(!m_pattern.m_ignoreCase || isASCIIAlpha(ch) || isCanonicallyUnique(ch));
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            or32(TrustedImm32(0x20), character);
            ch |= 0x20;
        }

        return branch32(NotEqual, character, Imm32(ch));
    }

    void readCharacter(int inputPosition, RegisterID reg)
    {
        if (m_charSize == Char8)
            load8(BaseIndex(input, index, TimesOne, inputPosition * sizeof(char)), reg);
        else
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

    void initCallFrame()
    {
        unsigned callFrameSize = m_pattern.m_body->m_callFrameSize;
        if (callFrameSize)
            subPtr(Imm32(callFrameSize * sizeof(void*)), stackPointerRegister);
    }
    void removeCallFrame()
    {
        unsigned callFrameSize = m_pattern.m_body->m_callFrameSize;
        if (callFrameSize)
            addPtr(Imm32(callFrameSize * sizeof(void*)), stackPointerRegister);
    }

    
    void setSubpatternStart(RegisterID reg, unsigned subpattern)
    {
        ASSERT(subpattern);
        
        store32(reg, Address(output, (subpattern << 1) * sizeof(int)));
    }
    void setSubpatternEnd(RegisterID reg, unsigned subpattern)
    {
        ASSERT(subpattern);
        
        store32(reg, Address(output, ((subpattern << 1) + 1) * sizeof(int)));
    }
    void clearSubpatternStart(unsigned subpattern)
    {
        ASSERT(subpattern);
        
        store32(TrustedImm32(-1), Address(output, (subpattern << 1) * sizeof(int)));
    }

    
    
    
    
    
    
    
    
    
    void setMatchStart(RegisterID reg)
    {
        ASSERT(!m_pattern.m_body->m_hasFixedSize);
        if (compileMode == IncludeSubpatterns)
            store32(reg, output);
        else
            move(reg, output);
    }
    void getMatchStart(RegisterID reg)
    {
        ASSERT(!m_pattern.m_body->m_hasFixedSize);
        if (compileMode == IncludeSubpatterns)
            load32(output, reg);
        else
            move(output, reg);
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

        
        
        Jump m_zeroLengthMatch;

        
        
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

    
    

    
    
    
    
    bool backtrackTermDefault(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        m_backtrackingState.append(op.m_jumps);
        return true;
    }

    bool generateAssertionBOL(size_t opIndex)
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
        return true;
    }
    bool backtrackAssertionBOL(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    bool generateAssertionEOL(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (term->inputPosition == m_checked)
                matchDest.append(atEndOfInput());

            readCharacter(term->inputPosition - m_checked, character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            op.m_jumps.append(jump());

            matchDest.link(this);
        } else {
            if (term->inputPosition == m_checked)
                op.m_jumps.append(notAtEndOfInput());
            
            else
                op.m_jumps.append(jump());
        }
        return true;
    }
    bool backtrackAssertionEOL(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
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

    bool generateAssertionWordBoundary(size_t opIndex)
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
        return true;
    }
    bool backtrackAssertionWordBoundary(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    bool generatePatternCharacterOnce(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];

        if (op.m_isDeadCode)
            return true;

        
        
        ASSERT(opIndex + 1 < m_ops.size());
        YarrOp* nextOp = &m_ops[opIndex + 1];

        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        if ((ch > 0xff) && (m_charSize == Char8)) {
            
            op.m_jumps.append(jump());
            return true;
        }

        const RegisterID character = regT0;
        int maxCharactersAtOnce = m_charSize == Char8 ? 4 : 2;
        unsigned ignoreCaseMask = 0;
#if CPU(BIG_ENDIAN)
        int allCharacters = ch << (m_charSize == Char8 ? 24 : 16);
#else
        int allCharacters = ch;
#endif
        int numberCharacters;
        int startTermPosition = term->inputPosition;

        
        
        ASSERT(!m_pattern.m_ignoreCase || isASCIIAlpha(ch) || isCanonicallyUnique(ch));

        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch))
#if CPU(BIG_ENDIAN)
            ignoreCaseMask |= 32 << (m_charSize == Char8 ? 24 : 16);
#else
            ignoreCaseMask |= 32;
#endif

        for (numberCharacters = 1; numberCharacters < maxCharactersAtOnce && nextOp->m_op == OpTerm; ++numberCharacters, nextOp = &m_ops[opIndex + numberCharacters]) {
            PatternTerm* nextTerm = nextOp->m_term;

            if (nextTerm->type != PatternTerm::TypePatternCharacter
                || nextTerm->quantityType != QuantifierFixedCount
                || nextTerm->quantityCount != 1
                || nextTerm->inputPosition != (startTermPosition + numberCharacters))
                break;

            nextOp->m_isDeadCode = true;

#if CPU(BIG_ENDIAN)
            int shiftAmount = (m_charSize == Char8 ? 24 : 16) - ((m_charSize == Char8 ? 8 : 16) * numberCharacters);
#else
            int shiftAmount = (m_charSize == Char8 ? 8 : 16) * numberCharacters;
#endif

            UChar currentCharacter = nextTerm->patternCharacter;

            if ((currentCharacter > 0xff) && (m_charSize == Char8)) {
                
                op.m_jumps.append(jump());
                return true;
            }

            
            
            ASSERT(!m_pattern.m_ignoreCase || isASCIIAlpha(currentCharacter) || isCanonicallyUnique(currentCharacter));

            allCharacters |= (currentCharacter << shiftAmount);

            if ((m_pattern.m_ignoreCase) && (isASCIIAlpha(currentCharacter)))
                ignoreCaseMask |= 32 << shiftAmount;
        }

        if (m_charSize == Char8) {
            switch (numberCharacters) {
            case 1:
                op.m_jumps.append(jumpIfCharNotEquals(ch, startTermPosition - m_checked, character));
                return true;
            case 2: {
                BaseIndex address(input, index, TimesOne, (startTermPosition - m_checked) * sizeof(LChar));
                load16Unaligned(address, character);
                break;
            }
            case 3: {
                BaseIndex highAddress(input, index, TimesOne, (startTermPosition - m_checked) * sizeof(LChar));
                load16Unaligned(highAddress, character);
                if (ignoreCaseMask)
                    or32(Imm32(ignoreCaseMask), character);
                op.m_jumps.append(branch32(NotEqual, character, Imm32((allCharacters & 0xffff) | ignoreCaseMask)));
                op.m_jumps.append(jumpIfCharNotEquals(allCharacters >> 16, startTermPosition + 2 - m_checked, character));
                return true;
            }
            case 4: {
                BaseIndex address(input, index, TimesOne, (startTermPosition - m_checked) * sizeof(LChar));
                load32WithUnalignedHalfWords(address, character);
                break;
            }
            }
        } else {
            switch (numberCharacters) {
            case 1:
                op.m_jumps.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked, character));
                return true;
            case 2:
                BaseIndex address(input, index, TimesTwo, (term->inputPosition - m_checked) * sizeof(UChar));
                load32WithUnalignedHalfWords(address, character);
                break;
            }
        }

        if (ignoreCaseMask)
            or32(Imm32(ignoreCaseMask), character);
        op.m_jumps.append(branch32(NotEqual, character, Imm32(allCharacters | ignoreCaseMask)));
        return true;
    }
    bool backtrackPatternCharacterOnce(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    bool generatePatternCharacterFixed(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(index, countRegister);
        if (term->quantityCount.hasOverflowed())
            return false;
        sub32(Imm32(term->quantityCount.unsafeGet()), countRegister);

        Label loop(this);
        int offset;
        if ((Checked<int>(term->inputPosition - m_checked + Checked<int64_t>(term->quantityCount)) * static_cast<int>(m_charSize == Char8 ? sizeof(char) : sizeof(UChar))).safeGet(offset))
            return false;
        BaseIndex address(input, countRegister, m_charScale, offset);

        if (m_charSize == Char8)
            load8(address, character);
        else
            load16(address, character);

        
        
        ASSERT(!m_pattern.m_ignoreCase || isASCIIAlpha(ch) || isCanonicallyUnique(ch));
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            or32(TrustedImm32(0x20), character);
            ch |= 0x20;
        }

        op.m_jumps.append(branch32(NotEqual, character, Imm32(ch)));
        add32(TrustedImm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);

        return true;
    }
    bool backtrackPatternCharacterFixed(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    bool generatePatternCharacterGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);

        
        if (!((ch > 0xff) && (m_charSize == Char8))) {
            JumpList failures;
            Label loop(this);
            failures.append(atEndOfInput());
            failures.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked, character));

            add32(TrustedImm32(1), countRegister);
            add32(TrustedImm32(1), index);
            if (term->quantityCount == quantifyInfinite) {
                jump(loop);
            } else {
                if (term->quantityCount.hasOverflowed())
                    return false;
                branch32(NotEqual, countRegister, Imm32(term->quantityCount.unsafeGet())).linkTo(loop, this);
            }

            failures.link(this);
        }
        op.m_reentry = label();

        storeToFrame(countRegister, term->frameLocation);
        return true;
    }
    bool backtrackPatternCharacterGreedy(size_t opIndex)
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

        return true;
    }

    bool generatePatternCharacterNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);
        op.m_reentry = label();
        storeToFrame(countRegister, term->frameLocation);
        return true;
    }
    bool backtrackPatternCharacterNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);

        
        if (!((ch > 0xff) && (m_charSize == Char8))) {
            JumpList nonGreedyFailures;
            nonGreedyFailures.append(atEndOfInput());
            if (term->quantityCount != quantifyInfinite) {
                if (term->quantityCount.hasOverflowed())
                    return false;
                nonGreedyFailures.append(branch32(Equal, countRegister, Imm32(term->quantityCount.unsafeGet())));
            }
            nonGreedyFailures.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked, character));

            add32(TrustedImm32(1), countRegister);
            add32(TrustedImm32(1), index);

            jump(op.m_reentry);
            nonGreedyFailures.link(this);
        }

        sub32(countRegister, index);
        m_backtrackingState.fallthrough();

        return true;
    }

    bool generateCharacterClassOnce(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        JumpList matchDest;
        readCharacter(term->inputPosition - m_checked, character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            op.m_jumps.append(matchDest);
        else {
            op.m_jumps.append(jump());
            matchDest.link(this);
        }
        return true;
    }
    bool backtrackCharacterClassOnce(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    bool generateCharacterClassFixed(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(index, countRegister);
        if (term->quantityCount.hasOverflowed())
            return false;
        sub32(Imm32(term->quantityCount.unsafeGet()), countRegister);

        Label loop(this);
        JumpList matchDest;

        int offset;
        Checked<int64_t> checkedOffset(term->inputPosition - m_checked + Checked<int64_t>(term->quantityCount));

        if (m_charSize == Char8) {
            if ((Checked<int>(checkedOffset) * static_cast<int>(sizeof(char))).safeGet(offset))
                return false;
            load8(BaseIndex(input, countRegister, TimesOne, offset), character);
        } else {
            if ((Checked<int>(checkedOffset) * static_cast<int>(sizeof(UChar))).safeGet(offset))
                return false;
            load16(BaseIndex(input, countRegister, TimesTwo, offset), character);
        }
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            op.m_jumps.append(matchDest);
        else {
            op.m_jumps.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
        return true;
    }
    bool backtrackCharacterClassFixed(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    bool generateCharacterClassGreedy(size_t opIndex)
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
            unsigned quantityCount;
            if (term->quantityCount.safeGet(quantityCount))
                return false;
            branch32(NotEqual, countRegister, Imm32(quantityCount)).linkTo(loop, this);
            failures.append(jump());
        } else
            jump(loop);

        failures.link(this);
        op.m_reentry = label();

        storeToFrame(countRegister, term->frameLocation);
        return true;
    }
    bool backtrackCharacterClassGreedy(size_t opIndex)
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

        return true;
    }

    bool generateCharacterClassNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);
        op.m_reentry = label();
        storeToFrame(countRegister, term->frameLocation);
        return true;
    }
    bool backtrackCharacterClassNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        JumpList nonGreedyFailures;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);

        nonGreedyFailures.append(atEndOfInput());
        if (term->quantityCount.hasOverflowed())
            return false;
        nonGreedyFailures.append(branch32(Equal, countRegister, Imm32(term->quantityCount.unsafeGet())));

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

        return true;
    }

    bool generateDotStarEnclosure(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID matchPos = regT1;

        JumpList foundBeginningNewLine;
        JumpList saveStartIndex;
        JumpList foundEndingNewLine;

        ASSERT(!m_pattern.m_body->m_hasFixedSize);
        getMatchStart(matchPos);

        saveStartIndex.append(branchTest32(Zero, matchPos));
        Label findBOLLoop(this);
        sub32(TrustedImm32(1), matchPos);
        if (m_charSize == Char8)
            load8(BaseIndex(input, matchPos, TimesOne, 0), character);
        else
            load16(BaseIndex(input, matchPos, TimesTwo, 0), character);
        matchCharacterClass(character, foundBeginningNewLine, m_pattern.newlineCharacterClass());
        branchTest32(NonZero, matchPos).linkTo(findBOLLoop, this);
        saveStartIndex.append(jump());

        foundBeginningNewLine.link(this);
        add32(TrustedImm32(1), matchPos); 
        saveStartIndex.link(this);

        if (!m_pattern.m_multiline && term->anchors.bolAnchor)
            op.m_jumps.append(branchTest32(NonZero, matchPos));

        ASSERT(!m_pattern.m_body->m_hasFixedSize);
        setMatchStart(matchPos);

        move(index, matchPos);

        Label findEOLLoop(this);
        foundEndingNewLine.append(branch32(Equal, matchPos, length));
        if (m_charSize == Char8)
            load8(BaseIndex(input, matchPos, TimesOne, 0), character);
        else
            load16(BaseIndex(input, matchPos, TimesTwo, 0), character);
        matchCharacterClass(character, foundEndingNewLine, m_pattern.newlineCharacterClass());
        add32(TrustedImm32(1), matchPos);
        jump(findEOLLoop);

        foundEndingNewLine.link(this);

        if (!m_pattern.m_multiline && term->anchors.eolAnchor)
            op.m_jumps.append(branch32(NotEqual, matchPos, length));

        move(matchPos, index);
        return true;
    }

    bool backtrackDotStarEnclosure(size_t opIndex)
    {
        return backtrackTermDefault(opIndex);
    }

    
    
    
    bool generateTerm(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        switch (term->type) {
        case PatternTerm::TypePatternCharacter:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    return generatePatternCharacterOnce(opIndex);
                else
                    return generatePatternCharacterFixed(opIndex);
                break;
            case QuantifierGreedy:
                return generatePatternCharacterGreedy(opIndex);
            case QuantifierNonGreedy:
                return generatePatternCharacterNonGreedy(opIndex);
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    return generateCharacterClassOnce(opIndex);
                else
                    return generateCharacterClassFixed(opIndex);
                break;
            case QuantifierGreedy:
                return generateCharacterClassGreedy(opIndex);
            case QuantifierNonGreedy:
                return generateCharacterClassNonGreedy(opIndex);
            }
            break;

        case PatternTerm::TypeAssertionBOL:
            return generateAssertionBOL(opIndex);

        case PatternTerm::TypeAssertionEOL:
            return generateAssertionEOL(opIndex);

        case PatternTerm::TypeAssertionWordBoundary:
            return generateAssertionWordBoundary(opIndex);

        case PatternTerm::TypeForwardReference:
            return true;

        case PatternTerm::TypeParenthesesSubpattern:
        case PatternTerm::TypeParentheticalAssertion:
            ASSERT_NOT_REACHED();
            return false;
        case PatternTerm::TypeBackReference:
            return false;
        case PatternTerm::TypeDotStarEnclosure:
            return generateDotStarEnclosure(opIndex);
        }

        return false;
    }
    bool backtrackTerm(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        switch (term->type) {
        case PatternTerm::TypePatternCharacter:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    return backtrackPatternCharacterOnce(opIndex);
                else
                    return backtrackPatternCharacterFixed(opIndex);
            case QuantifierGreedy:
                return backtrackPatternCharacterGreedy(opIndex);
            case QuantifierNonGreedy:
                return backtrackPatternCharacterNonGreedy(opIndex);
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    return backtrackCharacterClassOnce(opIndex);
                else
                    return backtrackCharacterClassFixed(opIndex);
            case QuantifierGreedy:
                return backtrackCharacterClassGreedy(opIndex);
            case QuantifierNonGreedy:
                return backtrackCharacterClassNonGreedy(opIndex);
            }
            break;

        case PatternTerm::TypeAssertionBOL:
            return backtrackAssertionBOL(opIndex);

        case PatternTerm::TypeAssertionEOL:
            return backtrackAssertionEOL(opIndex);

        case PatternTerm::TypeAssertionWordBoundary:
            return backtrackAssertionWordBoundary(opIndex);

        case PatternTerm::TypeForwardReference:
            return true;

        case PatternTerm::TypeParenthesesSubpattern:
        case PatternTerm::TypeParentheticalAssertion:
            ASSERT_NOT_REACHED();
            return false;

        case PatternTerm::TypeDotStarEnclosure:
            return backtrackDotStarEnclosure(opIndex);

        case PatternTerm::TypeBackReference:
            return false;
        }
        return true;
    }

    bool generate()
    {
        
        ASSERT(m_ops.size());
        size_t opIndex = 0;

        do {
            YarrOp& op = m_ops[opIndex];
            switch (op.m_op) {

            case OpTerm:
                if (!generateTerm(opIndex))
                    return false;
                break;

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            case OpBodyAlternativeBegin: {
                PatternAlternative* alternative = op.m_alternative;

                
                
                op.m_jumps.append(jumpIfNoAvailableInput(alternative->m_minimumSize));
                
                
                op.m_reentry = label();

                if (alternative->m_minimumSize > INT_MAX)
                    return false;
                m_checked = alternative->m_minimumSize;
                break;
            }
            case OpBodyAlternativeNext:
            case OpBodyAlternativeEnd: {
                PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                PatternAlternative* alternative = op.m_alternative;

                

                
#if !WTF_CPU_SPARC
                removeCallFrame();
#endif

                
                
                
                ASSERT(index != returnRegister);
                if (m_pattern.m_body->m_hasFixedSize) {
                    move(index, returnRegister);
                    if (priorAlternative->m_minimumSize)
                        sub32(Imm32(priorAlternative->m_minimumSize), returnRegister);
                    if (compileMode == IncludeSubpatterns)
                        store32(returnRegister, output);
                } else
                    getMatchStart(returnRegister);
                if (compileMode == IncludeSubpatterns)
                    store32(index, Address(output, 4));
#if WTF_CPU_X86_64
                
                move32(returnRegister, returnRegister);
                lshiftPtr(Imm32(32), index);
                orPtr(index, returnRegister);
#else
                move(index, returnRegister2);
#endif

                generateReturn();

                
                

                if (op.m_op == OpBodyAlternativeNext) {
                    
                    
                    
                    
                    op.m_reentry = label();
                    if (alternative->m_minimumSize > priorAlternative->m_minimumSize) {
                        add32(Imm32(alternative->m_minimumSize - priorAlternative->m_minimumSize), index);
                        op.m_jumps.append(jumpIfNoAvailableInput());
                    } else if (priorAlternative->m_minimumSize > alternative->m_minimumSize)
                        sub32(Imm32(priorAlternative->m_minimumSize - alternative->m_minimumSize), index);
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

                if (term->quantityType != QuantifierFixedCount && !m_ops[op.m_previousOp].m_alternative->m_minimumSize) {
                    
                    
                    op.m_zeroLengthMatch = branch32(Equal, index, Address(stackPointerRegister, term->frameLocation * sizeof(void*)));
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

                if (term->quantityType != QuantifierFixedCount && !m_ops[op.m_previousOp].m_alternative->m_minimumSize) {
                    
                    
                    op.m_zeroLengthMatch = branch32(Equal, index, Address(stackPointerRegister, term->frameLocation * sizeof(void*)));
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

                
                
                
                
                
                
                if (term->capture() && compileMode == IncludeSubpatterns) {
                    int inputOffset = term->inputPosition - m_checked;
                    if (term->quantityType == QuantifierFixedCount)
                        inputOffset -= term->parentheses.disjunction->m_minimumSize;
                    if (inputOffset) {
                        move(index, indexTemporary);
                        add32(Imm32(inputOffset), indexTemporary);
                        setSubpatternStart(indexTemporary, term->parentheses.subpatternId);
                    } else
                        setSubpatternStart(index, term->parentheses.subpatternId);
                }
                break;
            }
            case OpParenthesesSubpatternOnceEnd: {
                PatternTerm* term = op.m_term;
                const RegisterID indexTemporary = regT0;
                ASSERT(term->quantityCount == 1);

#ifndef NDEBUG
                
                
                if (term->quantityType != QuantifierFixedCount && !term->parentheses.disjunction->m_minimumSize) {
                    Jump pastBreakpoint;
                    pastBreakpoint = branch32(NotEqual, index, Address(stackPointerRegister, term->frameLocation * sizeof(void*)));
                    breakpoint();
                    pastBreakpoint.link(this);
                }
#endif

                
                
                
                
                
                
                if (term->capture() && compileMode == IncludeSubpatterns) {
                    int inputOffset = term->inputPosition - m_checked;
                    if (inputOffset) {
                        move(index, indexTemporary);
                        add32(Imm32(inputOffset), indexTemporary);
                        setSubpatternEnd(indexTemporary, term->parentheses.subpatternId);
                    } else
                        setSubpatternEnd(index, term->parentheses.subpatternId);
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
                YarrOp& beginOp = m_ops[op.m_previousOp];
#ifndef NDEBUG
                PatternTerm* term = op.m_term;

                
                
                Jump pastBreakpoint;
                pastBreakpoint = branch32(NotEqual, index, Address(stackPointerRegister, term->frameLocation * sizeof(void*)));
                breakpoint();
                pastBreakpoint.link(this);
#endif

                
                
                jump(beginOp.m_reentry);

                
                
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
#if !WTF_CPU_SPARC
                removeCallFrame();
#endif
#if WTF_CPU_X86_64
                move(TrustedImm32(int(WTF::notFound)), returnRegister);
#else
                move(TrustedImmPtr((void*)WTF::notFound), returnRegister);
                move(TrustedImm32(0), returnRegister2);
#endif
                generateReturn();
                break;
            }

            ++opIndex;
        } while (opIndex < m_ops.size());

        return true;
    }

    bool backtrack()
    {
        
        size_t opIndex = m_ops.size();
        ASSERT(opIndex);

        do {
            --opIndex;
            YarrOp& op = m_ops[opIndex];
            switch (op.m_op) {

            case OpTerm:
                if (!backtrackTerm(opIndex))
                    return false;
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
                    
                    
                    
                    
                    if (m_pattern.m_body->m_hasFixedSize
                        && (alternative->m_minimumSize > beginOp->m_alternative->m_minimumSize)
                        && (alternative->m_minimumSize - beginOp->m_alternative->m_minimumSize == 1))
                        m_backtrackingState.linkTo(beginOp->m_reentry, this);
                    else {
                        
                        
                        m_backtrackingState.link(this);

                        
                        if (!m_pattern.m_body->m_hasFixedSize) {
                            if (alternative->m_minimumSize == 1)
                                setMatchStart(index);
                            else {
                                move(index, regT0);
                                if (alternative->m_minimumSize)
                                    sub32(Imm32(alternative->m_minimumSize - 1), regT0);
                                else
                                    add32(TrustedImm32(1), regT0);
                                setMatchStart(regT0);
                            }
                        }

                        
                        
                        if (alternative->m_minimumSize > beginOp->m_alternative->m_minimumSize) {
                            
                            
                            unsigned delta = alternative->m_minimumSize - beginOp->m_alternative->m_minimumSize;
                            ASSERT(delta);
                            if (delta != 1)
                                sub32(Imm32(delta - 1), index);
                            jump(beginOp->m_reentry);
                        } else {
                            
                            
                            unsigned delta = beginOp->m_alternative->m_minimumSize - alternative->m_minimumSize;
                            if (delta != 0xFFFFFFFFu) {
                                
                                add32(Imm32(delta + 1), index);
                                checkInput().linkTo(beginOp->m_reentry, this);
                            }
                        }
                    }
                }

                
                
                
                
                
                
                
                Label firstInputCheckFailed(this);

                
                
                
                
                
                
                YarrOp* prevOp = beginOp;
                YarrOp* nextOp = &m_ops[beginOp->m_nextOp];
                while (nextOp->m_op != OpBodyAlternativeEnd) {
                    prevOp->m_jumps.link(this);

                    
                    
                    if (prevOp->m_alternative->m_minimumSize > nextOp->m_alternative->m_minimumSize) {
                        
                        
                        
                        unsigned delta = prevOp->m_alternative->m_minimumSize - nextOp->m_alternative->m_minimumSize;
                        sub32(Imm32(delta), index);
                        Jump fail = jumpIfNoAvailableInput();
                        add32(Imm32(delta), index);
                        jump(nextOp->m_reentry);
                        fail.link(this);
                    } else if (prevOp->m_alternative->m_minimumSize < nextOp->m_alternative->m_minimumSize)
                        add32(Imm32(nextOp->m_alternative->m_minimumSize - prevOp->m_alternative->m_minimumSize), index);
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
                    
                    setMatchStart(index);
                    needsToUpdateMatchStart = false;
                }

                
                
                
                
                ASSERT(alternative->m_minimumSize >= m_pattern.m_body->m_minimumSize);
                if (alternative->m_minimumSize == m_pattern.m_body->m_minimumSize) {
                    
                    
                    add32(TrustedImm32(1), index);
                } else {
                    
                    
                    unsigned delta = (alternative->m_minimumSize - m_pattern.m_body->m_minimumSize) - 1;
                    if (delta)
                        sub32(Imm32(delta), index);
                }
                Jump matchFailed = jumpIfNoAvailableInput();

                if (needsToUpdateMatchStart) {
                    if (!m_pattern.m_body->m_minimumSize)
                        setMatchStart(index);
                    else {
                        move(index, regT0);
                        sub32(Imm32(m_pattern.m_body->m_minimumSize), regT0);
                        setMatchStart(regT0);
                    }
                }

                
                
                
                if (beginOp->m_alternative->m_minimumSize == m_pattern.m_body->m_minimumSize)
                    jump(beginOp->m_reentry);
                else {
                    if (beginOp->m_alternative->m_minimumSize > m_pattern.m_body->m_minimumSize)
                        add32(Imm32(beginOp->m_alternative->m_minimumSize - m_pattern.m_body->m_minimumSize), index);
                    else
                        sub32(Imm32(m_pattern.m_body->m_minimumSize - beginOp->m_alternative->m_minimumSize), index);
                    checkInput().linkTo(beginOp->m_reentry, this);
                    jump(firstInputCheckFailed);
                }

                
                
                matchFailed.link(this);

#if !WTF_CPU_SPARC
                removeCallFrame();
#endif
#if WTF_CPU_X86_64
                move(TrustedImm32(int(WTF::notFound)), returnRegister);
#else
                move(TrustedImmPtr((void*)WTF::notFound), returnRegister);
                move(TrustedImm32(0), returnRegister2);
#endif
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

                
                if (op.m_zeroLengthMatch.isSet())
                    m_backtrackingState.append(op.m_zeroLengthMatch);

                
                

                
                
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

                
                if (op.m_zeroLengthMatch.isSet())
                    m_backtrackingState.append(op.m_zeroLengthMatch);

                
                
                
                
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

                
                if ((term->capture() && compileMode == IncludeSubpatterns) || term->quantityType == QuantifierGreedy) {
                    m_backtrackingState.link(this);

                    
                    if (term->capture() && compileMode == IncludeSubpatterns)
                        clearSubpatternStart(term->parentheses.subpatternId);

                    
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

        return true;
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
            
            
            
            
            
            
            Vector<PatternAlternative*>& alternatives = term->parentheses.disjunction->m_alternatives;
            if (alternatives.size() != 1) {
                for (unsigned i = 0; i < alternatives.size(); ++i) {
                    if (alternatives[i]->m_minimumSize == 0) {
                        m_shouldFallBack = true;
                        return;
                    }
                }
            }

            
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
        
        zeroExtend32ToPtr(index, index);
        zeroExtend32ToPtr(length, length);
#elif WTF_CPU_X86
        push(X86Registers::ebp);
        move(stackPointerRegister, X86Registers::ebp);
        
        push(X86Registers::ebx);
        push(X86Registers::edi);
        push(X86Registers::esi);
        
# if WTF_COMPILER_MSVC || WTF_COMPILER_SUNCC
        loadPtr(Address(X86Registers::ebp, 2 * sizeof(void*)), input);
        loadPtr(Address(X86Registers::ebp, 3 * sizeof(void*)), index);
        loadPtr(Address(X86Registers::ebp, 4 * sizeof(void*)), length);
        if (compileMode == IncludeSubpatterns)
            loadPtr(Address(X86Registers::ebp, 5 * sizeof(void*)), output);
# else
        if (compileMode == IncludeSubpatterns)
            loadPtr(Address(X86Registers::ebp, 2 * sizeof(void*)), output);
# endif
#elif WTF_CPU_ARM
        push(ARMRegisters::r4);
        push(ARMRegisters::r5);
        push(ARMRegisters::r6);
# if WTF_CPU_ARM_TRADITIONAL
        push(ARMRegisters::r8); 
# endif
        if (compileMode == IncludeSubpatterns)
            move(ARMRegisters::r3, output);
#elif WTF_CPU_SH4
        push(SH4Registers::r11);
        push(SH4Registers::r13);
#elif WTF_CPU_SPARC
        save(Imm32(-m_pattern.m_body->m_callFrameSize * sizeof(void*)));
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
# if WTF_CPU_ARM_TRADITIONAL
        pop(ARMRegisters::r8); 
# endif
        pop(ARMRegisters::r6);
        pop(ARMRegisters::r5);
        pop(ARMRegisters::r4);
#elif WTF_CPU_SH4
        pop(SH4Registers::r13);
        pop(SH4Registers::r11);
#elif WTF_CPU_SPARC
        ret_and_restore();
        return;
#elif WTF_CPU_MIPS
        
#endif
        ret();
    }

public:
    YarrGenerator(YarrPattern& pattern, YarrCharSize charSize)
        : m_pattern(pattern)
        , m_charSize(charSize)
        , m_charScale(m_charSize == Char8 ? TimesOne: TimesTwo)
        , m_shouldFallBack(false)
        , m_checked(0)
    {
    }

    void compile(JSGlobalData* globalData, YarrCodeBlock& jitObject)
    {
        generateEnter();

        Jump hasInput = checkInput();
#if WTF_CPU_X86_64
        move(TrustedImm32(int(WTF::notFound)), returnRegister);
#else
        move(TrustedImmPtr((void*)WTF::notFound), returnRegister);
        move(TrustedImm32(0), returnRegister2);
#endif
        generateReturn();
        hasInput.link(this);

        if (compileMode == IncludeSubpatterns) {
            for (unsigned i = 0; i < m_pattern.m_numSubpatterns + 1; ++i)
                store32(TrustedImm32(-1), Address(output, (i << 1) * sizeof(int)));
        }

        if (!m_pattern.m_body->m_hasFixedSize)
            setMatchStart(index);

        initCallFrame();

        
        opCompileBody(m_pattern.m_body);

        
        
        if (m_shouldFallBack) {
            jitObject.setFallBack(true);
            return;
        }

        if (!generate() || !backtrack()) {
            jitObject.setFallBack(true);
            return;
        }

        
        ExecutablePool *pool;
        bool ok;
        LinkBuffer linkBuffer(this, globalData->regexAllocator, &pool, &ok, REGEXP_CODE);

        
        if (linkBuffer.unsafeCode() == nullptr) {
            jitObject.setFallBack(true);
            return;
        }

        m_backtrackingState.linkDataLabels(linkBuffer);

        if (compileMode == MatchOnly) {
#if YARR_8BIT_CHAR_SUPPORT
            if (m_charSize == Char8)
                jitObject.set8BitCodeMatchOnly(linkBuffer.finalizeCode());
            else
#endif
                jitObject.set16BitCodeMatchOnly(linkBuffer.finalizeCode());
        } else {
#if YARR_8BIT_CHAR_SUPPORT
            if (m_charSize == Char8)
                jitObject.set8BitCode(linkBuffer.finalizeCode());
            else
#endif
                jitObject.set16BitCode(linkBuffer.finalizeCode());
        }
        jitObject.setFallBack(m_shouldFallBack);
    }

private:
    YarrPattern& m_pattern;

    YarrCharSize m_charSize;

    Scale m_charScale;

    
    
    bool m_shouldFallBack;

    
    Vector<YarrOp, 128> m_ops;

    
    
    
    
    
    
    
    
    
    
    int m_checked;

    
    BacktrackingState m_backtrackingState;
};

void jitCompile(YarrPattern& pattern, YarrCharSize charSize, JSGlobalData* globalData, YarrCodeBlock& jitObject, YarrJITCompileMode mode)
{
    if (mode == MatchOnly)
        YarrGenerator<MatchOnly>(pattern, charSize).compile(globalData, jitObject);
    else
        YarrGenerator<IncludeSubpatterns>(pattern, charSize).compile(globalData, jitObject);
}

}}

#endif
