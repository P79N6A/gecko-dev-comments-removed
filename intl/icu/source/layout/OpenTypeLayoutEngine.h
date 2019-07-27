




#ifndef __OPENTYPELAYOUTENGINE_H
#define __OPENTYPELAYOUTENGINE_H

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "LEFontInstance.h"
#include "LayoutEngine.h"
#include "LETableReference.h"

#include "GlyphSubstitutionTables.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositioningTables.h"

U_NAMESPACE_BEGIN


























class U_LAYOUT_API OpenTypeLayoutEngine : public LayoutEngine
{
public:
    
















    OpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                            le_int32 typoFlags, const LEReferenceTo<GlyphSubstitutionTableHeader> &gsubTable, LEErrorCode &success);

    










    OpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
			 le_int32 typoFlags, LEErrorCode &success);

    




    virtual ~OpenTypeLayoutEngine();

    











    static LETag getScriptTag(le_int32 scriptCode);
    











    static LETag getV2ScriptTag(le_int32 scriptCode);

    









    static LETag getLangSysTag(le_int32 languageCode);

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

    




    static const LETag languageTags[];

private:

    



    void setScriptAndLanguageTags();

    


    static const LETag scriptTags[];

    


    void applyTypoFlags();

protected:
    





    FeatureMask fFeatureMask;

    






    const FeatureMap *fFeatureMap;

    




    le_int32 fFeatureMapCount;

    






    le_bool fFeatureOrder;

    




    LEReferenceTo<GlyphSubstitutionTableHeader> fGSUBTable;

    




    LEReferenceTo<GlyphDefinitionTableHeader> fGDEFTable;

    




    LEReferenceTo<GlyphPositioningTableHeader> fGPOSTable;
    
    







    LEGlyphFilter *fSubstitutionFilter;

    




    LETag fScriptTag;
  
    




    LETag fScriptTagV2;

    




    LETag fLangSysTag;

    
























    virtual le_int32 characterProcessing(const LEUnicode [], le_int32 offset, le_int32 count, le_int32 max, le_bool ,
            LEUnicode *&, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    




























    virtual le_int32 glyphProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEGlyphStorage &glyphStorage, LEErrorCode &success);
 
    virtual le_int32 glyphSubstitution(le_int32 count, le_int32 max, le_bool rightToLeft, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    
























    virtual le_int32 glyphPostProcessing(LEGlyphStorage &tempGlyphStorage, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    





















    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    














    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, LEGlyphStorage &glyphStorage, LEErrorCode &success);

    






    virtual void reset();
};

U_NAMESPACE_END
#endif

