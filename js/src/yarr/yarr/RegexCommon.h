
























#ifndef RegexCommon_h
#define RegexCommon_h

namespace JSC { namespace Yarr {

enum ErrorCode {
    HitRecursionLimit = -2,
    NoError = 0,
    PatternTooLarge,
    QuantifierOutOfOrder,
    QuantifierWithoutAtom,
    MissingParentheses,
    ParenthesesUnmatched,
    ParenthesesTypeInvalid,
    CharacterClassUnmatched,
    CharacterClassOutOfOrder,
    CharacterClassRangeSingleChar,
    EscapeUnterminated,
    QuantifierTooLarge,
    NumberOfErrorCodes
};

}}

#endif
