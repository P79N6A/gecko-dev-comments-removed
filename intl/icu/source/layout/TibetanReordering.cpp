















#include "LETypes.h"
#include "OpenTypeTables.h"
#include "TibetanReordering.h"
#include "LEGlyphStorage.h"


U_NAMESPACE_BEGIN


enum
{
    C_DOTTED_CIRCLE = 0x25CC,
    C_PRE_NUMBER_MARK = 0x0F3F
 };


enum
{
    
    
    
    
    _xx = TibetanClassTable::CC_RESERVED,
    _ba = TibetanClassTable::CC_BASE,
    _sj = TibetanClassTable::CC_SUBJOINED | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_BELOW, 
    _tp = TibetanClassTable::CC_TSA_PHRU  | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_ABOVE,
    _ac = TibetanClassTable::CC_A_CHUNG |  TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_BELOW,
    _cs = TibetanClassTable::CC_COMP_SANSKRIT | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_BELOW,
    _ha = TibetanClassTable::CC_HALANTA | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_BELOW, 
    _bv = TibetanClassTable::CC_BELOW_VOWEL | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_BELOW,
    _av = TibetanClassTable::CC_ABOVE_VOWEL | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_ABOVE,
    _an = TibetanClassTable::CC_ANUSVARA | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_ABOVE,
    _cb = TibetanClassTable::CC_CANDRABINDU | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_ABOVE,
    _vs = TibetanClassTable::CC_VISARGA | TibetanClassTable::CF_DOTTED_CIRCLE| TibetanClassTable::CF_POS_AFTER,
    _as = TibetanClassTable::CC_ABOVE_S_MARK | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_ABOVE,
    _bs = TibetanClassTable::CC_BELOW_S_MARK | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_BELOW,
    _di = TibetanClassTable::CC_DIGIT | TibetanClassTable::CF_DIGIT,
    _pd = TibetanClassTable::CC_PRE_DIGIT_MARK | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_PREDIGIT | TibetanClassTable::CF_POS_BEFORE ,
    _bd = TibetanClassTable::CC_POST_BELOW_DIGIT_M | TibetanClassTable::CF_DOTTED_CIRCLE | TibetanClassTable::CF_POS_AFTER
};





















static const TibetanClassTable::CharClass tibetanCharClasses[] =
{
   
    _xx, _ba, _xx, _xx, _ba, _ba, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _bd, _bd, _xx, _xx, _xx, _xx, _xx, _xx, 
    _di, _di, _di, _di, _di, _di, _di, _di, _di, _di, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _bs, _xx, _bs, _xx, _tp, _xx, _xx, _xx, _xx, _bd, _pd, 
    _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _xx, _ba, _ba, _ba, _ba, _ba, _ba, _ba, 
    _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, 
    _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _xx, _xx, _xx, _xx, _xx, 
    _xx, _ac, _av, _cs, _bv, _bv, _cs, _cs, _cs, _cs, _av, _av, _av, _av, _an, _vs, 
    _av, _cs, _cb, _cb, _ha, _xx, _as, _as, _ba, _ba, _ba, _ba, _xx, _xx, _xx, _xx, 
    _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _xx, _sj, _sj, _sj, _sj, _sj, _sj, _sj, 
    _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, 
    _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _xx, _sj, _sj, 
    _xx, _xx, _xx, _xx, _xx, _xx, _bs, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx,
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, 
};                                                                                  










static const TibetanClassTable tibetanClassTable = {0x0F00, 0x0FFF, tibetanCharClasses};




TibetanClassTable::CharClass TibetanClassTable::getCharClass(LEUnicode ch) const
{
    if (ch < firstChar || ch > lastChar) {
        return CC_RESERVED;
    }
    
    return classTable[ch - firstChar];
}

const TibetanClassTable *TibetanClassTable::getTibetanClassTable()
{
    return &tibetanClassTable;
}



class TibetanReorderingOutput : public UMemory {
private:
    le_int32 fSyllableCount;
    le_int32 fOutIndex;
    LEUnicode *fOutChars;

    LEGlyphStorage &fGlyphStorage;


public:
    TibetanReorderingOutput(LEUnicode *outChars, LEGlyphStorage &glyphStorage)
        : fSyllableCount(0), fOutIndex(0), fOutChars(outChars), fGlyphStorage(glyphStorage)
    {
        
    }

    ~TibetanReorderingOutput()
    {
        
    }

    void reset()
    {
        fSyllableCount += 1;
    }

    void writeChar(LEUnicode ch, le_uint32 charIndex, FeatureMask featureMask)
    {
        LEErrorCode success = LE_NO_ERROR;

        fOutChars[fOutIndex] = ch;

        fGlyphStorage.setCharIndex(fOutIndex, charIndex, success);
        fGlyphStorage.setAuxData(fOutIndex, featureMask, success);

        fOutIndex += 1;
    }

    le_int32 getOutputIndex()
    {
        return fOutIndex;
    }
};



#define ccmpFeatureTag LE_CCMP_FEATURE_TAG
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
#define ccmpFeatureMask 0x00040000UL


#define distFeatureMask 0x00400000UL
#define blwmFeatureMask 0x00200000UL
#define abvmFeatureMask 0x00100000UL
#define mkmkFeatureMask 0x00080000UL

#define tagPref    (ccmpFeatureMask | prefFeatureMask | presFeatureMask | cligFeatureMask | distFeatureMask)
#define tagAbvf    (ccmpFeatureMask | abvfFeatureMask | abvsFeatureMask | cligFeatureMask | distFeatureMask | abvmFeatureMask | mkmkFeatureMask)
#define tagPstf    (ccmpFeatureMask | blwfFeatureMask | blwsFeatureMask | prefFeatureMask | presFeatureMask | pstfFeatureMask | pstsFeatureMask | cligFeatureMask | distFeatureMask | blwmFeatureMask)
#define tagBlwf    (ccmpFeatureMask | blwfFeatureMask | blwsFeatureMask | cligFeatureMask | distFeatureMask | blwmFeatureMask | mkmkFeatureMask)
#define tagDefault (ccmpFeatureMask | prefFeatureMask | blwfFeatureMask | presFeatureMask | blwsFeatureMask | cligFeatureMask | distFeatureMask | abvmFeatureMask | blwmFeatureMask | mkmkFeatureMask)





static const FeatureMap featureMap[] =
{
    
    {ccmpFeatureTag, ccmpFeatureMask},
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













static const le_int8 tibetanStateTable[][TibetanClassTable::CC_COUNT] =
{

     
    
    
    { 1,  2,  4,  3,  8,  7,  9, 10, 14, 13, 17, 18, 19, 19, 20, 21, 21,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, 
    {-1, -1,  4,  3,  8,  7,  9, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1,  5, -1,  8,  7, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1,  4,  6,  8,  7,  9, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1,  5, -1,  8,  7, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1,  8,  7, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 14, 13, 17, -1, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, 11, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, 12, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 14, 17, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 15, 17, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, 17, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 17, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 18, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 21,}, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, 
    

};         


const FeatureMap *TibetanReordering::getFeatureMap(le_int32 &count)
{
    count = featureMapCount;

    return featureMap;
}





le_int32 TibetanReordering::findSyllable(const TibetanClassTable *classTable, const LEUnicode *chars, le_int32 prev, le_int32 charCount)
{
    le_int32 cursor = prev;
    le_int8 state = 0;

    while (cursor < charCount) {
        TibetanClassTable::CharClass charClass = (classTable->getCharClass(chars[cursor]) & TibetanClassTable::CF_CLASS_MASK);

        state = tibetanStateTable[state][charClass];

        if (state < 0) {
            break;
        }

        cursor += 1;
    }

    return cursor;
}




le_int32 TibetanReordering::reorder(const LEUnicode *chars, le_int32 charCount, le_int32,
                                  LEUnicode *outChars, LEGlyphStorage &glyphStorage)
{
    const TibetanClassTable *classTable = TibetanClassTable::getTibetanClassTable();

    TibetanReorderingOutput output(outChars, glyphStorage);
    TibetanClassTable::CharClass charClass;
    le_int32 i, prev = 0;

    
    
    while (prev < charCount) {
        le_int32 syllable = findSyllable(classTable, chars, prev, charCount);  

        output.reset();
       
        
        
        
        
        if (classTable->getCharClass(chars[prev]) & TibetanClassTable::CF_DOTTED_CIRCLE) {
            output.writeChar(C_DOTTED_CIRCLE, prev, tagDefault);        
        }        

        
        for (i = prev; i < syllable; i += 1) {
            charClass = classTable->getCharClass(chars[i]);
           
           if ((TibetanClassTable::CF_DIGIT & charClass) 
              && ( classTable->getCharClass(chars[i+1]) & TibetanClassTable::CF_PREDIGIT))
           {
         		 output.writeChar(C_PRE_NUMBER_MARK, i, tagPref);
                         output.writeChar(chars[i], i+1 , tagPref);
			i += 1;
          } else {
            switch (charClass & TibetanClassTable::CF_POS_MASK) {
            	
            	
            
          	           	
                case TibetanClassTable::CF_POS_ABOVE :
                    output.writeChar(chars[i], i, tagAbvf);
                    break;
                
                case TibetanClassTable::CF_POS_AFTER :
                    output.writeChar(chars[i], i, tagPstf);
                    break;
                
                case TibetanClassTable::CF_POS_BELOW :
                    output.writeChar(chars[i], i, tagBlwf);
                    break;
                
                default:                                       
                    
                   output.writeChar(chars[i], i, tagDefault);
                    break;
            } 
          } 
        } 

        prev = syllable; 
    }

    return output.getOutputIndex();
}


U_NAMESPACE_END
