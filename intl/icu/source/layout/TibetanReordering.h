














#ifndef __TIBETANREORDERING_H
#define __TIBETANREORDERING_H









U_NAMESPACE_BEGIN

class LEGlyphStorage;













































    

struct TibetanClassTable    
{
    enum CharClassValues  
                          
                          
                          
    {
        CC_RESERVED             =  0, 
        CC_BASE                 =  1, 
        CC_SUBJOINED            =  2, 
        CC_TSA_PHRU             =  3, 
        CC_A_CHUNG              =  4, 
        CC_COMP_SANSKRIT        =  5, 
        CC_HALANTA              =  6, 
        CC_BELOW_VOWEL          =  7, 
        CC_ABOVE_VOWEL          =  8, 
        CC_ANUSVARA             =  9, 
        CC_CANDRABINDU          = 10, 
        CC_VISARGA              = 11, 
        CC_ABOVE_S_MARK         = 12, 
        CC_BELOW_S_MARK         = 13, 
        CC_DIGIT                = 14, 
        CC_PRE_DIGIT_MARK       = 15, 
        CC_POST_BELOW_DIGIT_M   = 16, 
        CC_COUNT                = 17  
    };

    enum CharClassFlags
    {
        CF_CLASS_MASK    = 0x0000FFFF,

        CF_DOTTED_CIRCLE = 0x04000000,  
        CF_DIGIT         = 0x01000000,  
        CF_PREDIGIT      = 0x02000000,  

        
        CF_POS_BEFORE    = 0x00080000,
        CF_POS_BELOW     = 0x00040000,
        CF_POS_ABOVE     = 0x00020000,
        CF_POS_AFTER     = 0x00010000,
        CF_POS_MASK      = 0x000f0000
    };

    typedef le_uint32 CharClass;

    typedef le_int32 ScriptFlags;

    LEUnicode firstChar;   
    LEUnicode lastChar;    
    const CharClass *classTable;

    CharClass getCharClass(LEUnicode ch) const;

    static const TibetanClassTable *getTibetanClassTable();
};


class TibetanReordering  {
public:
    static le_int32 reorder(const LEUnicode *theChars, le_int32 charCount, le_int32 scriptCode,
        LEUnicode *outChars, LEGlyphStorage &glyphStorage);

    static const FeatureMap *getFeatureMap(le_int32 &count);

private:
    
    TibetanReordering();

    static le_int32 findSyllable(const TibetanClassTable *classTable, const LEUnicode *chars, le_int32 prev, le_int32 charCount);

};


U_NAMESPACE_END
#endif
