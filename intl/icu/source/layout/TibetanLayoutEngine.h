














#ifndef __TIBETANLAYOUTENGINE_H
#define __TIBETANLAYOUTENGINE_H











U_NAMESPACE_BEGIN









class TibetanOpenTypeLayoutEngine : public OpenTypeLayoutEngine
{
public:
    

















    TibetanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                            le_int32 typoFlags, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTable, LEErrorCode &success);

    













    TibetanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                                le_int32 typoFlags, LEErrorCode &success);

    




   virtual ~TibetanOpenTypeLayoutEngine();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:

    




















    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif

