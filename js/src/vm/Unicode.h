





#ifndef vm_Unicode_h
#define vm_Unicode_h

#include "jspubtd.h"

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
    };
};

const char16_t BYTE_ORDER_MARK2 = 0xFFFE;
const char16_t NO_BREAK_SPACE  = 0x00A0;

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
CharInfo(char16_t code)
{
    const size_t shift = 5;
    size_t index = index1[code >> shift];
    index = index2[(index << shift) + (code & ((1 << shift) - 1))];

    return js_charinfo[index];
}

inline bool
IsIdentifierStart(char16_t ch)
{
    








    if (ch < 128)
        return js_isidstart[ch];

    return CharInfo(ch).isLetter();
}

inline bool
IsIdentifierPart(char16_t ch)
{
    

    if (ch < 128)
        return js_isident[ch];

    return CharInfo(ch).isIdentifierPart();
}

inline bool
IsLetter(char16_t ch)
{
    return CharInfo(ch).isLetter();
}

inline bool
IsSpace(char16_t ch)
{
    











    if (ch < 128)
        return js_isspace[ch];

    if (ch == NO_BREAK_SPACE)
        return true;

    return CharInfo(ch).isSpace();
}

inline bool
IsSpaceOrBOM2(char16_t ch)
{
    if (ch < 128)
        return js_isspace[ch];

    
    if (ch == NO_BREAK_SPACE || ch == BYTE_ORDER_MARK2)
        return true;

    return CharInfo(ch).isSpace();
}

inline char16_t
ToUpperCase(char16_t ch)
{
    if (ch < 128) {
        if (ch >= 'a' && ch <= 'z')
            return ch - ('a' - 'A');
        return ch;
    }

    const CharacterInfo& info = CharInfo(ch);

    return uint16_t(ch) + info.upperCase;
}

inline char16_t
ToLowerCase(char16_t ch)
{
    if (ch < 128) {
        if (ch >= 'A' && ch <= 'Z')
            return ch + ('a' - 'A');
        return ch;
    }

    const CharacterInfo& info = CharInfo(ch);

    return uint16_t(ch) + info.lowerCase;
}


inline bool
CanUpperCase(char16_t ch)
{
    if (ch < 128)
        return ch >= 'a' && ch <= 'z';
    return CharInfo(ch).upperCase != 0;
}


inline bool
CanLowerCase(char16_t ch)
{
    if (ch < 128)
        return ch >= 'A' && ch <= 'Z';
    return CharInfo(ch).lowerCase != 0;
}

} 
} 

#endif 
