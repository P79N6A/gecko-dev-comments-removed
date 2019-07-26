



#ifndef Unicode_h__
#define Unicode_h__

#include "mozilla/StandardInteger.h"

#include "jspubtd.h"

#ifdef DEBUG
#include <stdio.h> 
#endif

extern const bool js_isidstart[];
extern const bool js_isident[];
extern const bool js_isspace[];

namespace js {
namespace unicode {











































struct CharFlag {
    enum temp {
        SPACE  = 1 << 0,
        LETTER = 1 << 1,
        IDENTIFIER_PART = 1 << 2,
        NO_DELTA = 1 << 3
    };
};

const jschar BYTE_ORDER_MARK2 = 0xFFFE;
const jschar NO_BREAK_SPACE  = 0x00A0;

class CharacterInfo {
    



















  public:
    uint16_t upperCase;
    uint16_t lowerCase;
    uint8_t flags;

    inline bool isSpace() const {
        return flags & CharFlag::SPACE;
    }

    inline bool isLetter() const {
        return flags & CharFlag::LETTER;
    }

    inline bool isIdentifierPart() const {
        return flags & (CharFlag::IDENTIFIER_PART | CharFlag::LETTER);
    }
};

extern const uint8_t index1[];
extern const uint8_t index2[];
extern const CharacterInfo js_charinfo[];

inline const CharacterInfo&
CharInfo(jschar code)
{
    const size_t shift = 5;
    size_t index = index1[code >> shift];
    index = index2[(index << shift) + (code & ((1 << shift) - 1))];

    return js_charinfo[index];
}

inline bool
IsIdentifierStart(jschar ch)
{
    








    if (ch < 128)
        return js_isidstart[ch];

    return CharInfo(ch).isLetter();
}

inline bool
IsIdentifierPart(jschar ch)
{
    

    if (ch < 128)
        return js_isident[ch];

    return CharInfo(ch).isIdentifierPart();
}

inline bool
IsLetter(jschar ch)
{
    return CharInfo(ch).isLetter();
}

inline bool
IsSpace(jschar ch)
{
    











    if (ch < 128)
        return js_isspace[ch];

    if (ch == NO_BREAK_SPACE)
        return true;

    return CharInfo(ch).isSpace();
}

inline bool
IsSpaceOrBOM2(jschar ch)
{
    if (ch < 128)
        return js_isspace[ch];

    
    if (ch == NO_BREAK_SPACE || ch == BYTE_ORDER_MARK2)
        return true;

    return CharInfo(ch).isSpace();
}

inline jschar
ToUpperCase(jschar ch)
{
    const CharacterInfo &info = CharInfo(ch);

    



    if (info.flags & CharFlag::NO_DELTA)
        return info.upperCase;

    return uint16_t(ch) + info.upperCase;
}

inline jschar
ToLowerCase(jschar ch)
{
    const CharacterInfo &info = CharInfo(ch);

    if (info.flags & CharFlag::NO_DELTA)
        return info.lowerCase;

    return uint16_t(ch) + info.lowerCase;
}

} 
} 

#endif 
