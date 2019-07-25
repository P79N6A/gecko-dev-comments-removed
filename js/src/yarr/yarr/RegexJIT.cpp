
























#include "RegexJIT.h"

#if ENABLE_ASSEMBLER

#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/MacroAssembler.h"
#include "RegexCompiler.h"

#include "yarr/pcre/pcre.h" 

using namespace WTF;

namespace JSC { namespace Yarr {

class JSGlobalData;

class RegexGenerator : private MacroAssembler {
    friend void jitCompileRegex(JSGlobalData* globalData, RegexCodeBlock& jitObject, const UString& pattern, unsigned& numSubpatterns, const char*& error, bool ignoreCase, bool multiline);

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
#elif WTF_CPU_X86_64
#if WTF_PLATFORM_WIN
    static const RegisterID input = X86Registers::ecx;
    static const RegisterID index = X86Registers::edx;
    static const RegisterID length = X86Registers::r8;
    static const RegisterID output = X86Registers::r9;
#else
    static const RegisterID input = X86Registers::edi;
    static const RegisterID index = X86Registers::esi;
    static const RegisterID length = X86Registers::edx;
    static const RegisterID output = X86Registers::ecx;
#endif

    static const RegisterID regT0 = X86Registers::eax;
    static const RegisterID regT1 = X86Registers::ebx;

    static const RegisterID returnRegister = X86Registers::eax;
#endif

    void optimizeAlternative(PatternAlternative* alternative)
    {
        if (!alternative->m_terms.length())
            return;

        for (unsigned i = 0; i < alternative->m_terms.length() - 1; ++i) {
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
        if (charClass->m_matchesUnicode.length() || charClass->m_rangesUnicode.length()) {
            Jump isAscii = branch32(LessThanOrEqual, character, Imm32(0x7f));
        
            if (charClass->m_matchesUnicode.length()) {
                for (unsigned i = 0; i < charClass->m_matchesUnicode.length(); ++i) {
                    UChar ch = charClass->m_matchesUnicode[i];
                    matchDest.append(branch32(Equal, character, Imm32(ch)));
                }
            }
            
            if (charClass->m_rangesUnicode.length()) {
                for (unsigned i = 0; i < charClass->m_rangesUnicode.length(); ++i) {
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

        if (charClass->m_ranges.length()) {
            unsigned matchIndex = 0;
            JumpList failures; 
            matchCharacterClassRange(character, failures, matchDest, charClass->m_ranges.begin(), charClass->m_ranges.length(), &matchIndex, charClass->m_matches.begin(), charClass->m_matches.length());
            while (matchIndex < charClass->m_matches.length())
                matchDest.append(branch32(Equal, character, Imm32((unsigned short)charClass->m_matches[matchIndex++])));

            failures.link(this);
        } else if (charClass->m_matches.length()) {
            
            js::Vector<char, 16, js::SystemAllocPolicy> matchesAZaz;

            for (unsigned i = 0; i < charClass->m_matches.length(); ++i) {
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

            if (unsigned countAZaz = matchesAZaz.length()) {
                or32(Imm32(32), character);
                for (unsigned i = 0; i < countAZaz; ++i)
                    matchDest.append(branch32(Equal, character, Imm32(matchesAZaz[i])));
            }
        }

        if (charClass->m_matchesUnicode.length() || charClass->m_rangesUnicode.length())
            unicodeFail.link(this);
    }

    
    Jump jumpIfNoAvailableInput(unsigned countToCheck)
    {
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

    void storeToFrame(Imm32 imm, unsigned frameLocation)
    {
        poke(imm, frameLocation);
    }

    DataLabelPtr storeToFrameWithPatch(unsigned frameLocation)
    {
        return storePtrWithPatch(ImmPtr(0), Address(stackPointerRegister, frameLocation * sizeof(void*)));
    }

    void loadFromFrame(unsigned frameLocation, RegisterID reg)
    {
        peek(reg, frameLocation);
    }

    void loadFromFrameAndJump(unsigned frameLocation)
    {
        jump(Address(stackPointerRegister, frameLocation * sizeof(void*)));
    }

    struct AlternativeBacktrackRecord {
        DataLabelPtr dataLabel;
        Label backtrackLocation;

        AlternativeBacktrackRecord(DataLabelPtr dataLabel, Label backtrackLocation)
            : dataLabel(dataLabel)
            , backtrackLocation(backtrackLocation)
        {
        }
    };

    struct TermGenerationState {
        TermGenerationState(PatternDisjunction* disjunction, unsigned checkedTotal)
            : disjunction(disjunction)
            , checkedTotal(checkedTotal)
        {
        }

        void resetAlternative()
        {
            isBackTrackGenerated = false;
            alt = 0;
        }
        bool alternativeValid()
        {
            return alt < disjunction->m_alternatives.length();
        }
        void nextAlternative()
        {
            ++alt;
        }
        PatternAlternative* alternative()
        {
            return disjunction->m_alternatives[alt];
        }

        void resetTerm()
        {
            ASSERT(alternativeValid());
            t = 0;
        }
        bool termValid()
        {
            ASSERT(alternativeValid());
            return t < alternative()->m_terms.length();
        }
        void nextTerm()
        {
            ASSERT(alternativeValid());
            ++t;
        }
        PatternTerm& term()
        {
            ASSERT(alternativeValid());
            return alternative()->m_terms[t];
        }
        bool isLastTerm()
        {
            ASSERT(alternativeValid());
            return (t + 1) == alternative()->m_terms.length();
        }
        bool isMainDisjunction()
        {
            return !disjunction->m_parent;
        }

        PatternTerm& lookaheadTerm()
        {
            ASSERT(alternativeValid());
            ASSERT((t + 1) < alternative()->m_terms.length());
            return alternative()->m_terms[t + 1];
        }
        bool isSinglePatternCharacterLookaheadTerm()
        {
            ASSERT(alternativeValid());
            return ((t + 1) < alternative()->m_terms.length())
                && (lookaheadTerm().type == PatternTerm::TypePatternCharacter)
                && (lookaheadTerm().quantityType == QuantifierFixedCount)
                && (lookaheadTerm().quantityCount == 1);
        }

        int inputOffset()
        {
            return term().inputPosition - checkedTotal;
        }

        void jumpToBacktrack(Jump jump, MacroAssembler* masm)
        {
            if (isBackTrackGenerated)
                jump.linkTo(backtrackLabel, masm);
            else
                backTrackJumps.append(jump);
        }
        void jumpToBacktrack(JumpList& jumps, MacroAssembler* masm)
        {
            if (isBackTrackGenerated)
                jumps.linkTo(backtrackLabel, masm);
            else
                backTrackJumps.append(jumps);
        }
        bool plantJumpToBacktrackIfExists(MacroAssembler* masm)
        {
            if (isBackTrackGenerated) {
                masm->jump(backtrackLabel);
                return true;
            }
            return false;
        }
        void addBacktrackJump(Jump jump)
        {
            backTrackJumps.append(jump);
        }
        void setBacktrackGenerated(Label label)
        {
            isBackTrackGenerated = true;
            backtrackLabel = label;
        }
        void linkAlternativeBacktracks(MacroAssembler* masm)
        {
            isBackTrackGenerated = false;
            backTrackJumps.link(masm);
        }
        void linkAlternativeBacktracksTo(Label label, MacroAssembler* masm)
        {
            isBackTrackGenerated = false;
            backTrackJumps.linkTo(label, masm);
        }
        void propagateBacktrackingFrom(TermGenerationState& nestedParenthesesState, MacroAssembler* masm)
        {
            jumpToBacktrack(nestedParenthesesState.backTrackJumps, masm);
            if (nestedParenthesesState.isBackTrackGenerated)
                setBacktrackGenerated(nestedParenthesesState.backtrackLabel);
        }

        PatternDisjunction* disjunction;
        int checkedTotal;
    private:
        unsigned alt;
        unsigned t;
        JumpList backTrackJumps;
        Label backtrackLabel;
        bool isBackTrackGenerated;
    };

    void generateAssertionBOL(TermGenerationState& state)
    {
        PatternTerm& term = state.term();

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (!term.inputPosition)
                matchDest.append(branch32(Equal, index, Imm32(state.checkedTotal)));

            readCharacter(state.inputOffset() - 1, character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            state.jumpToBacktrack(jump(), this);

            matchDest.link(this);
        } else {
            
            if (term.inputPosition)
                state.jumpToBacktrack(jump(), this);
            else
                state.jumpToBacktrack(branch32(NotEqual, index, Imm32(state.checkedTotal)), this);
        }
    }

    void generateAssertionEOL(TermGenerationState& state)
    {
        PatternTerm& term = state.term();

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (term.inputPosition == state.checkedTotal)
                matchDest.append(atEndOfInput());

            readCharacter(state.inputOffset(), character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            state.jumpToBacktrack(jump(), this);

            matchDest.link(this);
        } else {
            if (term.inputPosition == state.checkedTotal)
                state.jumpToBacktrack(notAtEndOfInput(), this);
            
            else
                state.jumpToBacktrack(jump(), this);
        }
    }

    
    void matchAssertionWordchar(TermGenerationState& state, JumpList& nextIsWordChar, JumpList& nextIsNotWordChar)
    {
        const RegisterID character = regT0;
        PatternTerm& term = state.term();

        if (term.inputPosition == state.checkedTotal)
            nextIsNotWordChar.append(atEndOfInput());

        readCharacter(state.inputOffset(), character);
        matchCharacterClass(character, nextIsWordChar, m_pattern.wordcharCharacterClass());
    }

    void generateAssertionWordBoundary(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        PatternTerm& term = state.term();

        Jump atBegin;
        JumpList matchDest;
        if (!term.inputPosition)
            atBegin = branch32(Equal, index, Imm32(state.checkedTotal));
        readCharacter(state.inputOffset() - 1, character);
        matchCharacterClass(character, matchDest, m_pattern.wordcharCharacterClass());
        if (!term.inputPosition)
            atBegin.link(this);

        
        JumpList nonWordCharThenWordChar;
        JumpList nonWordCharThenNonWordChar;
        if (term.invertOrCapture) {
            matchAssertionWordchar(state, nonWordCharThenNonWordChar, nonWordCharThenWordChar);
            nonWordCharThenWordChar.append(jump());
        } else {
            matchAssertionWordchar(state, nonWordCharThenWordChar, nonWordCharThenNonWordChar);
            nonWordCharThenNonWordChar.append(jump());
        }
        state.jumpToBacktrack(nonWordCharThenNonWordChar, this);

        
        matchDest.link(this);
        JumpList wordCharThenWordChar;
        JumpList wordCharThenNonWordChar;
        if (term.invertOrCapture) {
            matchAssertionWordchar(state, wordCharThenNonWordChar, wordCharThenWordChar);
            wordCharThenWordChar.append(jump());
        } else {
            matchAssertionWordchar(state, wordCharThenWordChar, wordCharThenNonWordChar);
            
        }

        state.jumpToBacktrack(wordCharThenWordChar, this);
        
        nonWordCharThenWordChar.link(this);
        wordCharThenNonWordChar.link(this);
    }

    void generatePatternCharacterSingle(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        UChar ch = state.term().patternCharacter;

        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(state.inputOffset(), character);
            or32(Imm32(32), character);
            state.jumpToBacktrack(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))), this);
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            state.jumpToBacktrack(jumpIfCharNotEquals(ch, state.inputOffset()), this);
        }
    }

    void generatePatternCharacterPair(TermGenerationState& state)
    {
        const RegisterID character = regT0;
#if WTF_CPU_BIG_ENDIAN
        UChar ch2 = state.term().patternCharacter;
        UChar ch1 = state.lookaheadTerm().patternCharacter;
#else
        UChar ch1 = state.term().patternCharacter;
        UChar ch2 = state.lookaheadTerm().patternCharacter;
#endif

        int mask = 0;
        int chPair = ch1 | (ch2 << 16);
        
        if (m_pattern.m_ignoreCase) {
            if (isASCIIAlpha(ch1))
                mask |= 32;
            if (isASCIIAlpha(ch2))
                mask |= 32 << 16;
        }

        if (mask) {
            load32WithUnalignedHalfWords(BaseIndex(input, index, TimesTwo, state.inputOffset() * sizeof(UChar)), character);
            or32(Imm32(mask), character);
            state.jumpToBacktrack(branch32(NotEqual, character, Imm32(chPair | mask)), this);
        } else
            state.jumpToBacktrack(branch32WithUnalignedHalfWords(NotEqual, BaseIndex(input, index, TimesTwo, state.inputOffset() * sizeof(UChar)), Imm32(chPair)), this);
    }

    void generatePatternCharacterFixed(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;
        PatternTerm& term = state.term();
        UChar ch = term.patternCharacter;

        move(index, countRegister);
        sub32(Imm32(term.quantityCount), countRegister);

        Label loop(this);
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            load16(BaseIndex(input, countRegister, TimesTwo, (state.inputOffset() + term.quantityCount) * sizeof(UChar)), character);
            or32(Imm32(32), character);
            state.jumpToBacktrack(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))), this);
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            state.jumpToBacktrack(branch16(NotEqual, BaseIndex(input, countRegister, TimesTwo, (state.inputOffset() + term.quantityCount) * sizeof(UChar)), Imm32(ch)), this);
        }
        add32(Imm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
    }

    void generatePatternCharacterGreedy(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;
        PatternTerm& term = state.term();
        UChar ch = term.patternCharacter;
    
        move(Imm32(0), countRegister);

        JumpList failures;
        Label loop(this);
        failures.append(atEndOfInput());
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(state.inputOffset(), character);
            or32(Imm32(32), character);
            failures.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            failures.append(jumpIfCharNotEquals(ch, state.inputOffset()));
        }

        add32(Imm32(1), countRegister);
        add32(Imm32(1), index);
        if (term.quantityCount != 0xffffffff) {
            branch32(NotEqual, countRegister, Imm32(term.quantityCount)).linkTo(loop, this);
            failures.append(jump());
        } else
            jump(loop);

        Label backtrackBegin(this);
        loadFromFrame(term.frameLocation, countRegister);
        state.jumpToBacktrack(branchTest32(Zero, countRegister), this);
        sub32(Imm32(1), countRegister);
        sub32(Imm32(1), index);

        failures.link(this);

        storeToFrame(countRegister, term.frameLocation);

        state.setBacktrackGenerated(backtrackBegin);
    }

    void generatePatternCharacterNonGreedy(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;
        PatternTerm& term = state.term();
        UChar ch = term.patternCharacter;
    
        move(Imm32(0), countRegister);

        Jump firstTimeDoNothing = jump();

        Label hardFail(this);
        sub32(countRegister, index);
        state.jumpToBacktrack(jump(), this);

        Label backtrackBegin(this);
        loadFromFrame(term.frameLocation, countRegister);

        atEndOfInput().linkTo(hardFail, this);
        if (term.quantityCount != 0xffffffff)
            branch32(Equal, countRegister, Imm32(term.quantityCount), hardFail);
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(state.inputOffset(), character);
            or32(Imm32(32), character);
            branch32(NotEqual, character, Imm32(Unicode::toLower(ch))).linkTo(hardFail, this);
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            jumpIfCharNotEquals(ch, state.inputOffset()).linkTo(hardFail, this);
        }

        add32(Imm32(1), countRegister);
        add32(Imm32(1), index);

        firstTimeDoNothing.link(this);
        storeToFrame(countRegister, term.frameLocation);

        state.setBacktrackGenerated(backtrackBegin);
    }

    void generateCharacterClassSingle(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        PatternTerm& term = state.term();

        JumpList matchDest;
        readCharacter(state.inputOffset(), character);
        matchCharacterClass(character, matchDest, term.characterClass);

        if (term.invertOrCapture)
            state.jumpToBacktrack(matchDest, this);
        else {
            state.jumpToBacktrack(jump(), this);
            matchDest.link(this);
        }
    }

    void generateCharacterClassFixed(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;
        PatternTerm& term = state.term();

        move(index, countRegister);
        sub32(Imm32(term.quantityCount), countRegister);

        Label loop(this);
        JumpList matchDest;
        load16(BaseIndex(input, countRegister, TimesTwo, (state.inputOffset() + term.quantityCount) * sizeof(UChar)), character);
        matchCharacterClass(character, matchDest, term.characterClass);

        if (term.invertOrCapture)
            state.jumpToBacktrack(matchDest, this);
        else {
            state.jumpToBacktrack(jump(), this);
            matchDest.link(this);
        }

        add32(Imm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
    }

    void generateCharacterClassGreedy(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;
        PatternTerm& term = state.term();
    
        move(Imm32(0), countRegister);

        JumpList failures;
        Label loop(this);
        failures.append(atEndOfInput());

        if (term.invertOrCapture) {
            readCharacter(state.inputOffset(), character);
            matchCharacterClass(character, failures, term.characterClass);
        } else {
            JumpList matchDest;
            readCharacter(state.inputOffset(), character);
            matchCharacterClass(character, matchDest, term.characterClass);
            failures.append(jump());
            matchDest.link(this);
        }

        add32(Imm32(1), countRegister);
        add32(Imm32(1), index);
        if (term.quantityCount != 0xffffffff) {
            branch32(NotEqual, countRegister, Imm32(term.quantityCount)).linkTo(loop, this);
            failures.append(jump());
        } else
            jump(loop);

        Label backtrackBegin(this);
        loadFromFrame(term.frameLocation, countRegister);
        state.jumpToBacktrack(branchTest32(Zero, countRegister), this);
        sub32(Imm32(1), countRegister);
        sub32(Imm32(1), index);

        failures.link(this);

        storeToFrame(countRegister, term.frameLocation);

        state.setBacktrackGenerated(backtrackBegin);
    }

    void generateCharacterClassNonGreedy(TermGenerationState& state)
    {
        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;
        PatternTerm& term = state.term();
    
        move(Imm32(0), countRegister);

        Jump firstTimeDoNothing = jump();

        Label hardFail(this);
        sub32(countRegister, index);
        state.jumpToBacktrack(jump(), this);

        Label backtrackBegin(this);
        loadFromFrame(term.frameLocation, countRegister);

        atEndOfInput().linkTo(hardFail, this);
        branch32(Equal, countRegister, Imm32(term.quantityCount), hardFail);

        JumpList matchDest;
        readCharacter(state.inputOffset(), character);
        matchCharacterClass(character, matchDest, term.characterClass);

        if (term.invertOrCapture)
            matchDest.linkTo(hardFail, this);
        else {
            jump(hardFail);
            matchDest.link(this);
        }

        add32(Imm32(1), countRegister);
        add32(Imm32(1), index);

        firstTimeDoNothing.link(this);
        storeToFrame(countRegister, term.frameLocation);

        state.setBacktrackGenerated(backtrackBegin);
    }

    void generateParenthesesDisjunction(PatternTerm& parenthesesTerm, TermGenerationState& state, unsigned alternativeFrameLocation)
    {
        ASSERT((parenthesesTerm.type == PatternTerm::TypeParenthesesSubpattern) || (parenthesesTerm.type == PatternTerm::TypeParentheticalAssertion));
        ASSERT(parenthesesTerm.quantityCount == 1);
    
        PatternDisjunction* disjunction = parenthesesTerm.parentheses.disjunction;
        unsigned preCheckedCount = ((parenthesesTerm.quantityType == QuantifierFixedCount) && (parenthesesTerm.type != PatternTerm::TypeParentheticalAssertion)) ? disjunction->m_minimumSize : 0;

        if (disjunction->m_alternatives.length() == 1) {
            state.resetAlternative();
            ASSERT(state.alternativeValid());
            PatternAlternative* alternative = state.alternative();
            optimizeAlternative(alternative);

            int countToCheck = alternative->m_minimumSize - preCheckedCount;
            if (countToCheck) {
                ASSERT((parenthesesTerm.type == PatternTerm::TypeParentheticalAssertion) || (parenthesesTerm.quantityType != QuantifierFixedCount));

                
                
                
                Jump skip = jump();

                Label backtrackBegin(this);
                sub32(Imm32(countToCheck), index);
                state.addBacktrackJump(jump());
                
                skip.link(this);

                state.setBacktrackGenerated(backtrackBegin);

                state.jumpToBacktrack(jumpIfNoAvailableInput(countToCheck), this);
                state.checkedTotal += countToCheck;
            }

            for (state.resetTerm(); state.termValid(); state.nextTerm())
                generateTerm(state);

            state.checkedTotal -= countToCheck;
        } else {
            JumpList successes;

            for (state.resetAlternative(); state.alternativeValid(); state.nextAlternative()) {

                PatternAlternative* alternative = state.alternative();
                optimizeAlternative(alternative);

                ASSERT(alternative->m_minimumSize >= preCheckedCount);
                int countToCheck = alternative->m_minimumSize - preCheckedCount;
                if (countToCheck) {
                    state.addBacktrackJump(jumpIfNoAvailableInput(countToCheck));
                    state.checkedTotal += countToCheck;
                }

                for (state.resetTerm(); state.termValid(); state.nextTerm())
                    generateTerm(state);

                
                DataLabelPtr dataLabel = storeToFrameWithPatch(alternativeFrameLocation);
                successes.append(jump());

                
                Label backtrackLocation(this);
                
                
                state.plantJumpToBacktrackIfExists(this);
                
                state.linkAlternativeBacktracks(this);

                if (countToCheck) {
                    sub32(Imm32(countToCheck), index);
                    state.checkedTotal -= countToCheck;
                }

                m_backtrackRecords.append(AlternativeBacktrackRecord(dataLabel, backtrackLocation));
            }
            
            
            state.addBacktrackJump(jump());

            
            
            state.setBacktrackGenerated(label());
            loadFromFrameAndJump(alternativeFrameLocation);

            
            
            
            
            

            successes.link(this);
        }
    }

    void generateParenthesesSingle(TermGenerationState& state)
    {
        const RegisterID indexTemporary = regT0;
        PatternTerm& term = state.term();
        PatternDisjunction* disjunction = term.parentheses.disjunction;
        ASSERT(term.quantityCount == 1);

        unsigned preCheckedCount = (term.quantityType == QuantifierFixedCount) ? disjunction->m_minimumSize : 0;

        unsigned parenthesesFrameLocation = term.frameLocation;
        unsigned alternativeFrameLocation = parenthesesFrameLocation;
        if (term.quantityType != QuantifierFixedCount)
            alternativeFrameLocation += RegexStackSpaceForBackTrackInfoParenthesesOnce;

        
        if (!term.invertOrCapture && (term.quantityType == QuantifierFixedCount)) {
            TermGenerationState parenthesesState(disjunction, state.checkedTotal);
            generateParenthesesDisjunction(state.term(), parenthesesState, alternativeFrameLocation);
            
            
            
            state.propagateBacktrackingFrom(parenthesesState, this);
        } else {
            Jump nonGreedySkipParentheses;
            Label nonGreedyTryParentheses;
            if (term.quantityType == QuantifierGreedy)
                storeToFrame(index, parenthesesFrameLocation);
            else if (term.quantityType == QuantifierNonGreedy) {
                storeToFrame(Imm32(-1), parenthesesFrameLocation);
                nonGreedySkipParentheses = jump();
                nonGreedyTryParentheses = label();
                storeToFrame(index, parenthesesFrameLocation);
            }

            
            if (term.invertOrCapture) {
                int inputOffset = state.inputOffset() - preCheckedCount;
                if (inputOffset) {
                    move(index, indexTemporary);
                    add32(Imm32(inputOffset), indexTemporary);
                    store32(indexTemporary, Address(output, (term.parentheses.subpatternId << 1) * sizeof(int)));
                } else
                    store32(index, Address(output, (term.parentheses.subpatternId << 1) * sizeof(int)));
            }

            
            TermGenerationState parenthesesState(disjunction, state.checkedTotal);
            generateParenthesesDisjunction(state.term(), parenthesesState, alternativeFrameLocation);

            Jump success = (term.quantityType == QuantifierFixedCount) ?
                jump() :
                branch32(NotEqual, index, Address(stackPointerRegister, (parenthesesFrameLocation * sizeof(void*))));

            
            Label backtrackFromAfterParens(this);

            if (term.quantityType == QuantifierGreedy) {
                
                loadFromFrame(parenthesesFrameLocation, indexTemporary);
                state.jumpToBacktrack(branch32(Equal, indexTemporary, Imm32(-1)), this);
            } else if (term.quantityType == QuantifierNonGreedy) {
                
                loadFromFrame(parenthesesFrameLocation, indexTemporary);
                branch32(Equal, indexTemporary, Imm32(-1)).linkTo(nonGreedyTryParentheses, this);
            }

            parenthesesState.plantJumpToBacktrackIfExists(this);
            
            parenthesesState.linkAlternativeBacktracks(this);
            if (term.invertOrCapture) {
                store32(Imm32(-1), Address(output, (term.parentheses.subpatternId << 1) * sizeof(int)));
#if 0
                store32(Imm32(-1), Address(output, ((term.parentheses.subpatternId << 1) + 1) * sizeof(int)));
#endif
            }

            if (term.quantityType == QuantifierGreedy)
                storeToFrame(Imm32(-1), parenthesesFrameLocation);
            else
                state.jumpToBacktrack(jump(), this);

            state.setBacktrackGenerated(backtrackFromAfterParens);
            if (term.quantityType == QuantifierNonGreedy)
                nonGreedySkipParentheses.link(this);
            success.link(this);

            
            if (term.invertOrCapture) {
                int inputOffset = state.inputOffset();
                if (inputOffset) {
                    move(index, indexTemporary);
                    add32(Imm32(state.inputOffset()), indexTemporary);
                    store32(indexTemporary, Address(output, ((term.parentheses.subpatternId << 1) + 1) * sizeof(int)));
                } else
                    store32(index, Address(output, ((term.parentheses.subpatternId << 1) + 1) * sizeof(int)));
            }
        }
    }

    void generateParenthesesGreedyNoBacktrack(TermGenerationState& state)
    {
        PatternTerm& parenthesesTerm = state.term();
        PatternDisjunction* disjunction = parenthesesTerm.parentheses.disjunction;
        ASSERT(parenthesesTerm.type == PatternTerm::TypeParenthesesSubpattern);
        ASSERT(parenthesesTerm.quantityCount != 1); 

        TermGenerationState parenthesesState(disjunction, state.checkedTotal);

        Label matchAgain(this);

        storeToFrame(index, parenthesesTerm.frameLocation); 

        for (parenthesesState.resetAlternative(); parenthesesState.alternativeValid(); parenthesesState.nextAlternative()) {

            PatternAlternative* alternative = parenthesesState.alternative();
            optimizeAlternative(alternative);

            int countToCheck = alternative->m_minimumSize;
            if (countToCheck) {
                parenthesesState.addBacktrackJump(jumpIfNoAvailableInput(countToCheck));
                parenthesesState.checkedTotal += countToCheck;
            }

            for (parenthesesState.resetTerm(); parenthesesState.termValid(); parenthesesState.nextTerm())
                generateTerm(parenthesesState);

            
            branch32(NotEqual, index, Address(stackPointerRegister, (parenthesesTerm.frameLocation * sizeof(void*))), matchAgain);

            
            
            parenthesesState.plantJumpToBacktrackIfExists(this);

            parenthesesState.linkAlternativeBacktracks(this);
            

            if (countToCheck) {
                sub32(Imm32(countToCheck), index);
                parenthesesState.checkedTotal -= countToCheck;
            }
        }

        
        
    }

    void generateParentheticalAssertion(TermGenerationState& state)
    {
        PatternTerm& term = state.term();
        PatternDisjunction* disjunction = term.parentheses.disjunction;
        ASSERT(term.quantityCount == 1);
        ASSERT(term.quantityType == QuantifierFixedCount);

        unsigned parenthesesFrameLocation = term.frameLocation;
        unsigned alternativeFrameLocation = parenthesesFrameLocation + RegexStackSpaceForBackTrackInfoParentheticalAssertion;

        int countCheckedAfterAssertion = state.checkedTotal - term.inputPosition;

        if (term.invertOrCapture) {
            
            storeToFrame(index, parenthesesFrameLocation);

            state.checkedTotal -= countCheckedAfterAssertion;
            if (countCheckedAfterAssertion)
                sub32(Imm32(countCheckedAfterAssertion), index);

            TermGenerationState parenthesesState(disjunction, state.checkedTotal);
            generateParenthesesDisjunction(state.term(), parenthesesState, alternativeFrameLocation);
            
            loadFromFrame(parenthesesFrameLocation, index);
            state.jumpToBacktrack(jump(), this);

            
            parenthesesState.linkAlternativeBacktracks(this);
            loadFromFrame(parenthesesFrameLocation, index);

            state.checkedTotal += countCheckedAfterAssertion;
        } else {
            
            storeToFrame(index, parenthesesFrameLocation);

            state.checkedTotal -= countCheckedAfterAssertion;
            if (countCheckedAfterAssertion)
                sub32(Imm32(countCheckedAfterAssertion), index);

            TermGenerationState parenthesesState(disjunction, state.checkedTotal);
            generateParenthesesDisjunction(state.term(), parenthesesState, alternativeFrameLocation);
            
            loadFromFrame(parenthesesFrameLocation, index);
            Jump success = jump();

            parenthesesState.linkAlternativeBacktracks(this);
            loadFromFrame(parenthesesFrameLocation, index);
            state.jumpToBacktrack(jump(), this);

            success.link(this);

            state.checkedTotal += countCheckedAfterAssertion;
        }
    }

    void generateTerm(TermGenerationState& state)
    {
        PatternTerm& term = state.term();

        switch (term.type) {
        case PatternTerm::TypeAssertionBOL:
            generateAssertionBOL(state);
            break;
        
        case PatternTerm::TypeAssertionEOL:
            generateAssertionEOL(state);
            break;
        
        case PatternTerm::TypeAssertionWordBoundary:
            generateAssertionWordBoundary(state);
            break;
        
        case PatternTerm::TypePatternCharacter:
            switch (term.quantityType) {
            case QuantifierFixedCount:
                if (term.quantityCount == 1) {
                    if (state.isSinglePatternCharacterLookaheadTerm() && (state.lookaheadTerm().inputPosition == (term.inputPosition + 1))) {
                        generatePatternCharacterPair(state);
                        state.nextTerm();
                    } else
                        generatePatternCharacterSingle(state);
                } else
                    generatePatternCharacterFixed(state);
                break;
            case QuantifierGreedy:
                generatePatternCharacterGreedy(state);
                break;
            case QuantifierNonGreedy:
                generatePatternCharacterNonGreedy(state);
                break;
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term.quantityType) {
            case QuantifierFixedCount:
                if (term.quantityCount == 1)
                    generateCharacterClassSingle(state);
                else
                    generateCharacterClassFixed(state);
                break;
            case QuantifierGreedy:
                generateCharacterClassGreedy(state);
                break;
            case QuantifierNonGreedy:
                generateCharacterClassNonGreedy(state);
                break;
            }
            break;

        case PatternTerm::TypeBackReference:
            m_shouldFallBack = true;
            break;

        case PatternTerm::TypeForwardReference:
            break;

        case PatternTerm::TypeParenthesesSubpattern:
            if (term.quantityCount == 1 && !term.parentheses.isCopy)
                generateParenthesesSingle(state);
            else if (term.parentheses.isTerminal)
                generateParenthesesGreedyNoBacktrack(state);
            else
                m_shouldFallBack = true;
            break;

        case PatternTerm::TypeParentheticalAssertion:
            generateParentheticalAssertion(state);
            break;
        }
    }

    void generateDisjunction(PatternDisjunction* disjunction)
    {
        TermGenerationState state(disjunction, 0);
        state.resetAlternative();

        
        int countCheckedForCurrentAlternative = 0;
        int countToCheckForFirstAlternative = 0;
        bool hasShorterAlternatives = false;
        bool setRepeatAlternativeLabels = false;
        JumpList notEnoughInputForPreviousAlternative;
        Label firstAlternative;
        Label firstAlternativeInputChecked;

        
        
        
        
        
        if (state.alternativeValid()) {
            PatternAlternative* alternative = state.alternative();
            if (!alternative->onceThrough()) {
                firstAlternative = Label(this);
                setRepeatAlternativeLabels = true;
            }
            countToCheckForFirstAlternative = alternative->m_minimumSize;
            state.checkedTotal += countToCheckForFirstAlternative;
            if (countToCheckForFirstAlternative)
                notEnoughInputForPreviousAlternative.append(jumpIfNoAvailableInput(countToCheckForFirstAlternative));
            countCheckedForCurrentAlternative = countToCheckForFirstAlternative;
        }

        if (setRepeatAlternativeLabels)
            firstAlternativeInputChecked = Label(this);

        while (state.alternativeValid()) {
            PatternAlternative* alternative = state.alternative();
            optimizeAlternative(alternative);

            
            if (!alternative->onceThrough())
                hasShorterAlternatives = hasShorterAlternatives || (countCheckedForCurrentAlternative < countToCheckForFirstAlternative);
            
            for (state.resetTerm(); state.termValid(); state.nextTerm())
                generateTerm(state);

            
            if (m_pattern.m_body->m_callFrameSize)
                addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

            ASSERT(index != returnRegister);
            if (m_pattern.m_body->m_hasFixedSize) {
                move(index, returnRegister);
                if (alternative->m_minimumSize)
                    sub32(Imm32(alternative->m_minimumSize), returnRegister);

                store32(returnRegister, output);
            } else
                load32(Address(output), returnRegister);

            store32(index, Address(output, 4));

            generateReturn();

            state.nextAlternative();

            
            if (state.alternativeValid()) {
                PatternAlternative* nextAlternative = state.alternative();
                if (!setRepeatAlternativeLabels && !nextAlternative->onceThrough()) {
                    
                    
                    state.jumpToBacktrack(jump(), this);
                    
                    countToCheckForFirstAlternative = nextAlternative->m_minimumSize;
                    
                    
                    notEnoughInputForPreviousAlternative.link(this);
                    
                    state.linkAlternativeBacktracks(this);

                    
                    if (countCheckedForCurrentAlternative)
                        sub32(Imm32(countCheckedForCurrentAlternative), index);
                    
                    firstAlternative = Label(this);
                    
                    state.checkedTotal = countToCheckForFirstAlternative;
                    if (countToCheckForFirstAlternative)
                        notEnoughInputForPreviousAlternative.append(jumpIfNoAvailableInput(countToCheckForFirstAlternative));
                    
                    countCheckedForCurrentAlternative = countToCheckForFirstAlternative;
                    
                    firstAlternativeInputChecked = Label(this);

                    setRepeatAlternativeLabels = true;
                } else {
                    int countToCheckForNextAlternative = nextAlternative->m_minimumSize;
                    
                    if (countCheckedForCurrentAlternative > countToCheckForNextAlternative) { 
                        
                        notEnoughInputForPreviousAlternative.link(this);
                        
                        
                        notEnoughInputForPreviousAlternative.append(jumpIfNoAvailableInput(countToCheckForNextAlternative - countCheckedForCurrentAlternative));
                        
                        
                        add32(Imm32(countCheckedForCurrentAlternative - countToCheckForNextAlternative), index);
                        
                        
                        state.linkAlternativeBacktracks(this);
                        
                        
                        sub32(Imm32(countCheckedForCurrentAlternative - countToCheckForNextAlternative), index);
                    } else if (countCheckedForCurrentAlternative < countToCheckForNextAlternative) { 
                        
                        
                        
                        
                        notEnoughInputForPreviousAlternative.link(this);
                        add32(Imm32(countToCheckForNextAlternative - countCheckedForCurrentAlternative), index);
                        notEnoughInputForPreviousAlternative.append(jump());
                        
                        
                        state.linkAlternativeBacktracks(this);
                        notEnoughInputForPreviousAlternative.append(jumpIfNoAvailableInput(countToCheckForNextAlternative - countCheckedForCurrentAlternative));
                    } else { 
                        ASSERT(countCheckedForCurrentAlternative == countToCheckForNextAlternative);
                        
                        
                        
                        
                        state.linkAlternativeBacktracks(this);
                    }
                    state.checkedTotal -= countCheckedForCurrentAlternative;
                    countCheckedForCurrentAlternative = countToCheckForNextAlternative;
                    state.checkedTotal += countCheckedForCurrentAlternative;
                }
            }
        }
        
        

        state.checkedTotal -= countCheckedForCurrentAlternative;

        if (!setRepeatAlternativeLabels) {
            
            
            state.linkAlternativeBacktracks(this);
            notEnoughInputForPreviousAlternative.link(this);
        } else {
            
            
            
            
            
            
            
            
            
            
            
            
            
            int incrementForNextIter = (countToCheckForFirstAlternative - countCheckedForCurrentAlternative) + 1;

            
            if (incrementForNextIter > 0) 
                state.linkAlternativeBacktracks(this);
            else if (m_pattern.m_body->m_hasFixedSize && !incrementForNextIter) 
                state.linkAlternativeBacktracksTo(firstAlternativeInputChecked, this);
            else { 
                state.linkAlternativeBacktracks(this);

                
                if (!m_pattern.m_body->m_hasFixedSize) {
                    move(index, regT0);
                    sub32(Imm32(countCheckedForCurrentAlternative - 1), regT0);
                    store32(regT0, Address(output));
                }

                
                if (incrementForNextIter)
                    add32(Imm32(incrementForNextIter), index);
                jump().linkTo(firstAlternativeInputChecked, this);
            }

            notEnoughInputForPreviousAlternative.link(this);
            
            if (!m_pattern.m_body->m_hasFixedSize) {
                if (countCheckedForCurrentAlternative - 1) {
                    move(index, regT0);
                    sub32(Imm32(countCheckedForCurrentAlternative - 1), regT0);
                    store32(regT0, Address(output));
                } else
                    store32(index, Address(output));
            }
        
            
            jumpIfAvailableInput(incrementForNextIter).linkTo(firstAlternativeInputChecked, this);
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (hasShorterAlternatives)
                jumpIfAvailableInput(-countToCheckForFirstAlternative).linkTo(firstAlternative, this);
            
            
            
        }

        if (m_pattern.m_body->m_callFrameSize)
            addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

        move(Imm32(-1), returnRegister);

        generateReturn();
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
        
    #if WTF_COMPILER_MSVC || WTF_COMPILER_SUNPRO
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
#elif WTF_CPU_SPARC
        save(Imm32(-m_pattern.m_body->m_callFrameSize * sizeof(void*)));
        
        m_pattern.m_body->m_callFrameSize = 0;
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
#elif WTF_CPU_SPARC
        ret_and_restore();
        return;
#elif WTF_CPU_MIPS
        
#endif
        ret();
    }

public:
    RegexGenerator(RegexPattern& pattern)
        : m_pattern(pattern)
        , m_shouldFallBack(false)
    {
    }

    void generate()
    {
        generateEnter();

        if (!m_pattern.m_body->m_hasFixedSize)
            store32(index, Address(output));

        if (m_pattern.m_body->m_callFrameSize)
            subPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

        generateDisjunction(m_pattern.m_body);
    }

    void compile(ExecutableAllocator& allocator, RegexCodeBlock& jitObject)
    {
        generate();

        if (oom()) {
            m_shouldFallBack = true;
            return;
        }

        ExecutablePool *dummy;
        bool ok;
        LinkBuffer patchBuffer(this, &allocator, &dummy, &ok);
        if (!ok) {
            m_shouldFallBack = true;
            return;
        }

        for (unsigned i = 0; i < m_backtrackRecords.length(); ++i)
            patchBuffer.patch(m_backtrackRecords[i].dataLabel, patchBuffer.locationOf(m_backtrackRecords[i].backtrackLocation));

        jitObject.set(patchBuffer.finalizeCode());
    }

    bool shouldFallBack()
    {
        return m_shouldFallBack;
    }

private:
    RegexPattern& m_pattern;
    bool m_shouldFallBack;
    js::Vector<AlternativeBacktrackRecord, 0, js::SystemAllocPolicy> m_backtrackRecords;
};

void jitCompileRegex(ExecutableAllocator& allocator, RegexCodeBlock& jitObject, const UString&patternString, unsigned& numSubpatterns, int &error, bool &fellBack, bool ignoreCase, bool multiline
#ifdef ANDROID
                     , bool forceFallback
#endif
)
{
#ifdef ANDROID
    if (!forceFallback) {
#endif
    fellBack = false;
    RegexPattern pattern(ignoreCase, multiline);
    if ((error = compileRegex(patternString, pattern)))
        return;
    numSubpatterns = pattern.m_numSubpatterns;

    if (!pattern.m_containsBackreferences) {
        RegexGenerator generator(pattern);
        generator.compile(allocator, jitObject);
        if (!generator.shouldFallBack())
            return;
    }
#ifdef ANDROID
    } 
#endif

    fellBack = true;
    JSRegExpIgnoreCaseOption ignoreCaseOption = ignoreCase ? JSRegExpIgnoreCase : JSRegExpDoNotIgnoreCase;
    JSRegExpMultilineOption multilineOption = multiline ? JSRegExpMultiline : JSRegExpSingleLine;
    jitObject.setFallback(jsRegExpCompile(reinterpret_cast<const UChar*>(const_cast<UString &>(patternString).chars()), patternString.length(), ignoreCaseOption, multilineOption, &numSubpatterns, &error));
}

}}

#endif
