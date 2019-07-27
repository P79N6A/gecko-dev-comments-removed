








#include "LETypes.h"
#include "OpenTypeTables.h"
#include "KhmerReordering.h"
#include "LEGlyphStorage.h"


U_NAMESPACE_BEGIN


enum
{
    C_SIGN_ZWNJ     = 0x200C,
    C_SIGN_ZWJ      = 0x200D,
    C_DOTTED_CIRCLE = 0x25CC,
    C_RO            = 0x179A,
    C_VOWEL_AA      = 0x17B6,
    C_SIGN_NIKAHIT  = 0x17C6,
    C_VOWEL_E       = 0x17C1,
    C_COENG         = 0x17D2
};


enum
{
    
    
    
    
    _xx = KhmerClassTable::CC_RESERVED,
    _sa = KhmerClassTable::CC_SIGN_ABOVE | KhmerClassTable::CF_DOTTED_CIRCLE | KhmerClassTable::CF_POS_ABOVE,
    _sp = KhmerClassTable::CC_SIGN_AFTER | KhmerClassTable::CF_DOTTED_CIRCLE| KhmerClassTable::CF_POS_AFTER,
    _c1 = KhmerClassTable::CC_CONSONANT | KhmerClassTable::CF_CONSONANT,
    _c2 = KhmerClassTable::CC_CONSONANT2 | KhmerClassTable::CF_CONSONANT,
    _c3 = KhmerClassTable::CC_CONSONANT3 | KhmerClassTable::CF_CONSONANT,
    _rb = KhmerClassTable::CC_ROBAT | KhmerClassTable::CF_POS_ABOVE | KhmerClassTable::CF_DOTTED_CIRCLE,
    _cs = KhmerClassTable::CC_CONSONANT_SHIFTER | KhmerClassTable::CF_DOTTED_CIRCLE | KhmerClassTable::CF_SHIFTER,
    _dl = KhmerClassTable::CC_DEPENDENT_VOWEL | KhmerClassTable::CF_POS_BEFORE | KhmerClassTable::CF_DOTTED_CIRCLE, 
    _db = KhmerClassTable::CC_DEPENDENT_VOWEL | KhmerClassTable::CF_POS_BELOW | KhmerClassTable::CF_DOTTED_CIRCLE,
    _da = KhmerClassTable::CC_DEPENDENT_VOWEL | KhmerClassTable::CF_POS_ABOVE | KhmerClassTable::CF_DOTTED_CIRCLE | KhmerClassTable::CF_ABOVE_VOWEL,
    _dr = KhmerClassTable::CC_DEPENDENT_VOWEL | KhmerClassTable::CF_POS_AFTER | KhmerClassTable::CF_DOTTED_CIRCLE,
    _co = KhmerClassTable::CC_COENG | KhmerClassTable::CF_COENG | KhmerClassTable::CF_DOTTED_CIRCLE,
    
    
    _va = _da | KhmerClassTable::CF_SPLIT_VOWEL,
    _vr = _dr | KhmerClassTable::CF_SPLIT_VOWEL
};




















static const KhmerClassTable::CharClass khmerCharClasses[] =
{
    _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, 
    _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c2, _c1, _c1, _c1, _c3, _c3, 
    _c1, _c3, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, 
    _c1, _c1, _c1, _c1, _dr, _dr, _dr, _da, _da, _da, _da, _db, _db, _db, _va, _vr, 
    _vr, _dl, _dl, _dl, _vr, _vr, _sa, _sp, _sp, _cs, _cs, _sa, _rb, _sa, _sa, _sa, 
    _sa, _sa, _co, _sa, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _sa, _xx, _xx, 
};                                                                                  










static const KhmerClassTable khmerClassTable = {0x1780, 0x17df, khmerCharClasses};






KhmerClassTable::CharClass KhmerClassTable::getCharClass(LEUnicode ch) const
{

    if (ch == C_SIGN_ZWJ) {
        return CC_ZERO_WIDTH_J_MARK;
    }

    if (ch == C_SIGN_ZWNJ) {
        return CC_ZERO_WIDTH_NJ_MARK;
    }

    if (ch < firstChar || ch > lastChar) {
        return CC_RESERVED;
    }
    
    return classTable[ch - firstChar];
}

const KhmerClassTable *KhmerClassTable::getKhmerClassTable()
{
    return &khmerClassTable;
}



class KhmerReorderingOutput : public UMemory {
private:
    le_int32 fSyllableCount;
    le_int32 fOutIndex;
    LEUnicode *fOutChars;

    LEGlyphStorage &fGlyphStorage;


public:
    KhmerReorderingOutput(LEUnicode *outChars, LEGlyphStorage &glyphStorage)
        : fSyllableCount(0), fOutIndex(0), fOutChars(outChars), fGlyphStorage(glyphStorage)
    {
        
    }

    ~KhmerReorderingOutput()
    {
        
    }

    void reset()
    {
        fSyllableCount += 1;
    }

    void writeChar(LEUnicode ch, le_uint32 charIndex, FeatureMask charFeatures)
    {
        LEErrorCode success = LE_NO_ERROR;

        fOutChars[fOutIndex] = ch;

        fGlyphStorage.setCharIndex(fOutIndex, charIndex, success);
        fGlyphStorage.setAuxData(fOutIndex, charFeatures | (fSyllableCount & LE_GLYPH_GROUP_MASK), success);

        fOutIndex += 1;
    }

    le_int32 getOutputIndex()
    {
        return fOutIndex;
    }
};


#define blwfFeatureTag LE_BLWF_FEATURE_TAG
#define pstfFeatureTag LE_PSTF_FEATURE_TAG
#define presFeatureTag LE_PRES_FEATURE_TAG
#define blwsFeatureTag LE_BLWS_FEATURE_TAG
#define abvsFeatureTag LE_ABVS_FEATURE_TAG
#define pstsFeatureTag LE_PSTS_FEATURE_TAG

#define blwmFeatureTag LE_BLWM_FEATURE_TAG
#define abvmFeatureTag LE_ABVM_FEATURE_TAG
#define distFeatureTag LE_DIST_FEATURE_TAG

#define prefFeatureTag LE_PREF_FEATURE_TAG
#define abvfFeatureTag LE_ABVF_FEATURE_TAG
#define cligFeatureTag LE_CLIG_FEATURE_TAG
#define mkmkFeatureTag LE_MKMK_FEATURE_TAG

#define prefFeatureMask 0x80000000UL
#define blwfFeatureMask 0x40000000UL
#define abvfFeatureMask 0x20000000UL
#define pstfFeatureMask 0x10000000UL 
#define presFeatureMask 0x08000000UL
#define blwsFeatureMask 0x04000000UL
#define abvsFeatureMask 0x02000000UL
#define pstsFeatureMask 0x01000000UL
#define cligFeatureMask 0x00800000UL
#define distFeatureMask 0x00400000UL
#define blwmFeatureMask 0x00200000UL
#define abvmFeatureMask 0x00100000UL
#define mkmkFeatureMask 0x00080000UL

#define tagPref    (prefFeatureMask | presFeatureMask | cligFeatureMask | distFeatureMask)
#define tagAbvf    (abvfFeatureMask | abvsFeatureMask | cligFeatureMask | distFeatureMask | abvmFeatureMask | mkmkFeatureMask)
#define tagPstf    (blwfFeatureMask | blwsFeatureMask | prefFeatureMask | presFeatureMask | pstfFeatureMask | pstsFeatureMask | cligFeatureMask | distFeatureMask | blwmFeatureMask)
#define tagBlwf    (blwfFeatureMask | blwsFeatureMask | cligFeatureMask | distFeatureMask | blwmFeatureMask | mkmkFeatureMask)
#define tagDefault (prefFeatureMask | blwfFeatureMask | presFeatureMask | blwsFeatureMask | cligFeatureMask | distFeatureMask | abvmFeatureMask | blwmFeatureMask | mkmkFeatureMask)





static const FeatureMap featureMap[] =
{
    
    {prefFeatureTag, prefFeatureMask},
    {blwfFeatureTag, blwfFeatureMask},
    {abvfFeatureTag, abvfFeatureMask},
    {pstfFeatureTag, pstfFeatureMask}, 
    {presFeatureTag, presFeatureMask},
    {blwsFeatureTag, blwsFeatureMask},
    {abvsFeatureTag, abvsFeatureMask},
    {pstsFeatureTag, pstsFeatureMask},
    {cligFeatureTag, cligFeatureMask},
    
    
    {distFeatureTag, distFeatureMask},
    {blwmFeatureTag, blwmFeatureMask},
    {abvmFeatureTag, abvmFeatureMask},
    {mkmkFeatureTag, mkmkFeatureMask},
};

static const le_int32 featureMapCount = LE_ARRAY_SIZE(featureMap);






























































static const le_int8 khmerStateTable[][KhmerClassTable::CC_COUNT] =
{


    { 1,  2,  2,  2,  1,  1,  1,  6,  1,  1,  1,  2}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, 
    {-1, -1, -1, -1,  3,  4,  5,  6, 16, 17,  1, -1}, 
    {-1, -1, -1, -1, -1,  4, -1, -1, 16, -1, -1, -1}, 
                                                      
    {-1, -1, -1, -1, 15, -1, -1,  6, 16, 17,  1, 14}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 20, -1,  1, -1}, 
    {-1,  7,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1}, 
    {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, 
    {-1, -1, -1, -1, 12, 13, -1, -1, 16, 17,  1, 14}, 
    {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, 
    {-1, 11, 11, 11, -1, -1, -1, -1, -1, -1, -1, -1}, 
    {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, 
    {-1, -1, -1, -1, -1, 13, -1, -1, 16, -1, -1, -1}, 
    {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 17,  1, 18}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, 18}, 
    {-1, -1, -1, -1, -1, -1, -1, 19, -1, -1, -1, -1}, 
    {-1,  1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1}, 

};         


const FeatureMap *KhmerReordering::getFeatureMap(le_int32 &count)
{
    count = featureMapCount;

    return featureMap;
}





le_int32 KhmerReordering::findSyllable(const KhmerClassTable *classTable, const LEUnicode *chars, le_int32 prev, le_int32 charCount)
{
    le_int32 cursor = prev;
    le_int8 state = 0;

    while (cursor < charCount) {
        KhmerClassTable::CharClass charClass = (classTable->getCharClass(chars[cursor]) & KhmerClassTable::CF_CLASS_MASK);

        state = khmerStateTable[state][charClass];

        if (state < 0) {
            break;
        }

        cursor += 1;
    }

    return cursor;
}




le_int32 KhmerReordering::reorder(const LEUnicode *chars, le_int32 charCount, le_int32 ,
                                  LEUnicode *outChars, LEGlyphStorage &glyphStorage)
{
    const KhmerClassTable *classTable = KhmerClassTable::getKhmerClassTable();

    KhmerReorderingOutput output(outChars, glyphStorage);
    KhmerClassTable::CharClass charClass;
    le_int32 i, prev = 0, coengRo;

    
    
    
    while (prev < charCount) {
        le_int32 syllable = findSyllable(classTable, chars, prev, charCount);

        output.reset();
   
        
        
        
        coengRo = -1;  
        for (i = prev; i < syllable; i += 1) {
            charClass = classTable->getCharClass(chars[i]);
            
            
            
            if (charClass & KhmerClassTable::CF_SPLIT_VOWEL) {
                output.writeChar(C_VOWEL_E, i, tagPref);
                break; 
            }
            
            
            if (charClass & KhmerClassTable::CF_POS_BEFORE) {
                output.writeChar(chars[i], i, tagPref);
                break; 
            }
            
            
            
            
            
            if ( (charClass & KhmerClassTable::CF_COENG) && (i + 1 < syllable) &&
                 ( (classTable->getCharClass(chars[i + 1]) & KhmerClassTable::CF_CLASS_MASK) == KhmerClassTable::CC_CONSONANT2) )
            {
                    coengRo = i;
            }
        }
        
        
        if (coengRo > -1) {
            output.writeChar(C_COENG, coengRo, tagPref);
            output.writeChar(C_RO, coengRo + 1, tagPref);
        }
        
        
        
        
        
        if (classTable->getCharClass(chars[prev]) & KhmerClassTable::CF_DOTTED_CIRCLE) {
            output.writeChar(C_DOTTED_CIRCLE, prev, tagDefault);        
        }        

        
        for (i = prev; i < syllable; i += 1) {
            charClass = classTable->getCharClass(chars[i]);
            
            
            if (charClass & KhmerClassTable::CF_POS_BEFORE) {
                continue;
            }
            
            
            if (i == coengRo) {
                i += 1;
                continue;
            }
            
            switch (charClass & KhmerClassTable::CF_POS_MASK) {
                case KhmerClassTable::CF_POS_ABOVE :
                    output.writeChar(chars[i], i, tagAbvf);
                    break;
                
                case KhmerClassTable::CF_POS_AFTER :
                    output.writeChar(chars[i], i, tagPstf);
                    break;
                
                case KhmerClassTable::CF_POS_BELOW :
                    output.writeChar(chars[i], i, tagBlwf);
                    break;
                
                default:
                    
                    
                    if ( (charClass & KhmerClassTable::CF_COENG) && i + 1 < syllable ) {
                        if ( (classTable->getCharClass(chars[i + 1]) & KhmerClassTable::CF_CLASS_MASK) 
                              == KhmerClassTable::CC_CONSONANT3) {
                            output.writeChar(chars[i], i, tagPstf);
                            i += 1;
                            output.writeChar(chars[i], i, tagPstf);
                        }
                        else {
                            output.writeChar(chars[i], i, tagBlwf);
                            i += 1;
                            output.writeChar(chars[i], i, tagBlwf);
                        }
                        break;
                    }
                    
                    
                    
                    
                    
                    if ( (charClass & KhmerClassTable::CF_SHIFTER) && (i + 1 < syllable) ) {
                        if ((classTable->getCharClass(chars[i + 1]) & KhmerClassTable::CF_ABOVE_VOWEL)
                            || (i + 2 < syllable
                                && ( (classTable->getCharClass(chars[i + 1]) & KhmerClassTable::CF_CLASS_MASK) == C_VOWEL_AA)
                                && ( (classTable->getCharClass(chars[i + 2]) & KhmerClassTable::CF_CLASS_MASK) == C_SIGN_NIKAHIT))
                            || (i + 3 < syllable && (classTable->getCharClass(chars[i + 3]) & KhmerClassTable::CF_ABOVE_VOWEL))
                            || (i + 4 < syllable
                                && ( (classTable->getCharClass(chars[i + 3]) & KhmerClassTable::CF_CLASS_MASK) == C_VOWEL_AA)
                                && ( (classTable->getCharClass(chars[i + 4]) & KhmerClassTable::CF_CLASS_MASK) == C_SIGN_NIKAHIT) ) ) 
                        {
                            output.writeChar(chars[i], i, tagBlwf);
                            break;
                        }
                        
                    }
                    
                    output.writeChar(chars[i], i, tagDefault);
                    break;
            } 
        } 

        prev = syllable; 
    }

    return output.getOutputIndex();
}


U_NAMESPACE_END
