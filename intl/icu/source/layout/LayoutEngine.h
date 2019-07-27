



#ifndef __LAYOUTENGINE_H
#define __LAYOUTENGINE_H

#include "LETypes.h"






U_NAMESPACE_BEGIN

class LEFontInstance;
class LEGlyphFilter;
class LEGlyphStorage;



















































class U_LAYOUT_API LayoutEngine : public UObject {
public:
#ifndef U_HIDE_INTERNAL_API
    
    static const le_int32 kTypoFlagKern;
    
    static const le_int32 kTypoFlagLiga;	
#endif  

protected:
    




    LEGlyphStorage *fGlyphStorage;

    






    const LEFontInstance *fFontInstance;

    






    le_int32 fScriptCode;

    






    le_int32 fLanguageCode;

    




    le_int32 fTypoFlags;

    





    le_bool fFilterZeroWidth;

#ifndef U_HIDE_INTERNAL_API
    















    LayoutEngine(const LEFontInstance *fontInstance,
                 le_int32 scriptCode,
                 le_int32 languageCode,
                 le_int32 typoFlags,
                 LEErrorCode &success);
#endif  

    
    
    






    LayoutEngine();

    





















    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    

























    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    












    virtual void positionGlyphs(LEGlyphStorage &glyphStorage, float x, float y, LEErrorCode &success);

    



















    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    












    virtual const void *getFontTable(LETag tableTag, size_t &length) const;

    


    virtual const void *getFontTable(LETag tableTag) const { size_t ignored; return getFontTable(tableTag, ignored); }

    
























    virtual void mapCharsToGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, le_bool mirror, LEGlyphStorage &glyphStorage, LEErrorCode &success);

#ifndef U_HIDE_INTERNAL_API
    











    static void adjustMarkGlyphs(LEGlyphStorage &glyphStorage, LEGlyphFilter *markFilter, LEErrorCode &success);


    

















    static void adjustMarkGlyphs(const LEUnicode chars[], le_int32 charCount, le_bool reverse, LEGlyphStorage &glyphStorage, LEGlyphFilter *markFilter, LEErrorCode &success);
#endif  

public:
    







    virtual ~LayoutEngine();

    
























    virtual le_int32 layoutChars(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft, float x, float y, LEErrorCode &success);

    








    le_int32 getGlyphCount() const;

    









    void getGlyphs(LEGlyphID glyphs[], LEErrorCode &success) const;

    











    virtual void getGlyphs(le_uint32 glyphs[], le_uint32 extraBits, LEErrorCode &success) const;

    









    void getCharIndices(le_int32 charIndices[], LEErrorCode &success) const;

    










    void getCharIndices(le_int32 charIndices[], le_int32 indexBase, LEErrorCode &success) const;

    










    void getGlyphPositions(float positions[], LEErrorCode &success) const;

    













    void getGlyphPosition(le_int32 glyphIndex, float &x, float &y, LEErrorCode &success) const;

    






    virtual void reset();

    















    static LayoutEngine *layoutEngineFactory(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, LEErrorCode &success);

    



    static LayoutEngine *layoutEngineFactory(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typo_flags, LEErrorCode &success);

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

};

U_NAMESPACE_END
#endif
