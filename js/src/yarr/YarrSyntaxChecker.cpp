




























#include "YarrSyntaxChecker.h"

#include "YarrParser.h"

namespace JSC { namespace Yarr {

class SyntaxChecker {
public:
    void assertionBOL() {}
    void assertionEOL() {}
    void assertionWordBoundary(bool) {}
    void atomPatternCharacter(UChar) {}
    void atomBuiltInCharacterClass(BuiltInCharacterClassID, bool) {}
    void atomCharacterClassBegin(bool = false) {}
    void atomCharacterClassAtom(UChar) {}
    void atomCharacterClassRange(UChar, UChar) {}
    void atomCharacterClassBuiltIn(BuiltInCharacterClassID, bool) {}
    void atomCharacterClassEnd() {}
    void atomParenthesesSubpatternBegin(bool = true) {}
    void atomParentheticalAssertionBegin(bool = false) {}
    void atomParenthesesEnd() {}
    void atomBackReference(unsigned) {}
    void quantifyAtom(unsigned, unsigned, bool) {}
    void disjunction() {}
};

ErrorCode checkSyntax(const UString& pattern)
{
    SyntaxChecker syntaxChecker;
    return parse(syntaxChecker, pattern);
}

}} 
