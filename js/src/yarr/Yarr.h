


























#ifndef Yarr_h
#define Yarr_h

#include <limits.h>
#include "YarrInterpreter.h"
#include "YarrPattern.h"

namespace JSC { namespace Yarr {

#define YarrStackSpaceForBackTrackInfoPatternCharacter 1 // Only for !fixed quantifiers.
#define YarrStackSpaceForBackTrackInfoCharacterClass 1 // Only for !fixed quantifiers.
#define YarrStackSpaceForBackTrackInfoBackReference 2
#define YarrStackSpaceForBackTrackInfoAlternative 1 // One per alternative.
#define YarrStackSpaceForBackTrackInfoParentheticalAssertion 1
#define YarrStackSpaceForBackTrackInfoParenthesesOnce 1 // Only for !fixed quantifiers.
#define YarrStackSpaceForBackTrackInfoParenthesesTerminal 1
#define YarrStackSpaceForBackTrackInfoParentheses 2

static const unsigned quantifyInfinite = UINT_MAX;
static const unsigned offsetNoMatch = (unsigned)-1;



static const unsigned matchLimit = 1000000;

enum JSRegExpResult {
    JSRegExpMatch = 1,
    JSRegExpNoMatch = 0,
    JSRegExpErrorNoMatch = -1,
    JSRegExpErrorHitLimit = -2,
    JSRegExpErrorNoMemory = -3,
    JSRegExpErrorInternal = -4
};

enum YarrCharSize {
    Char8,
    Char16
};

} } 

#endif 

