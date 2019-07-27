






#ifndef __LEGLYPHSTORAGE_H
#define __LEGLYPHSTORAGE_H

#include "LETypes.h"
#include "LEInsertionList.h"






U_NAMESPACE_BEGIN














class U_LAYOUT_API LEGlyphStorage : public UObject, protected LEInsertionCallback
{
private:
    




    le_int32   fGlyphCount;

    




    LEGlyphID *fGlyphs;
 
    




    le_int32  *fCharIndices;

    




    float     *fPositions;

    




    le_uint32 *fAuxData;


    




    LEInsertionList *fInsertionList;

    




    le_int32 fSrcIndex;

    




    le_int32 fDestIndex;

protected:
    














    virtual le_bool applyInsertion(le_int32 atPosition, le_int32 count, LEGlyphID newGlyphs[]);

public:

    






    LEGlyphStorage();

    




    ~LEGlyphStorage();

    






    inline le_int32 getGlyphCount() const;

    









    void getGlyphs(LEGlyphID glyphs[], LEErrorCode &success) const;

    











    void getGlyphs(le_uint32 glyphs[], le_uint32 extraBits, LEErrorCode &success) const;

    









    void getCharIndices(le_int32 charIndices[], LEErrorCode &success) const;

    










    void getCharIndices(le_int32 charIndices[], le_int32 indexBase, LEErrorCode &success) const;

    










    void getGlyphPositions(float positions[], LEErrorCode &success) const;

    













    void getGlyphPosition(le_int32 glyphIndex, float &x, float &y, LEErrorCode &success) const;

    











    void allocateGlyphArray(le_int32 initialGlyphCount, le_bool rightToLeft, LEErrorCode &success);

    









    le_int32 allocatePositions(LEErrorCode &success);

    








    le_int32 allocateAuxData(LEErrorCode &success);

    







    void getAuxData(le_uint32 auxData[], LEErrorCode &success) const;

    









    LEGlyphID getGlyphID(le_int32 glyphIndex, LEErrorCode &success) const;

    









    le_int32  getCharIndex(le_int32 glyphIndex, LEErrorCode &success) const;


    









    le_uint32 getAuxData(le_int32 glyphIndex, LEErrorCode &success) const;

    









    inline LEGlyphID &operator[](le_int32 glyphIndex) const;

    
















    LEGlyphID *insertGlyphs(le_int32 atIndex, le_int32 insertCount, LEErrorCode& success);

    

















    LEGlyphID *insertGlyphs(le_int32 atIndex, le_int32 insertCount);

    












    void moveGlyph(le_int32 fromPosition, le_int32 toPosition, le_uint32 marker);

    











    le_int32 applyInsertions();

    








    void setGlyphID(le_int32 glyphIndex, LEGlyphID glyphID, LEErrorCode &success);

    








    void setCharIndex(le_int32 glyphIndex, le_int32 charIndex, LEErrorCode &success);

    









    void setPosition(le_int32 glyphIndex, float x, float y, LEErrorCode &success);

    









    void adjustPosition(le_int32 glyphIndex, float xAdjust, float yAdjust, LEErrorCode &success);

    








    void setAuxData(le_int32 glyphIndex, le_uint32 auxData, LEErrorCode &success);

    









    void adoptGlyphArray(LEGlyphStorage &from);

    









    void adoptCharIndicesArray(LEGlyphStorage &from);

    









    void adoptPositionArray(LEGlyphStorage &from);

    









    void adoptAuxDataArray(LEGlyphStorage &from);

    








    void adoptGlyphCount(LEGlyphStorage &from);

    






    void adoptGlyphCount(le_int32 newGlyphCount);

    







    void reset();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();
};

inline le_int32 LEGlyphStorage::getGlyphCount() const
{
    return fGlyphCount;
}

inline LEGlyphID &LEGlyphStorage::operator[](le_int32 glyphIndex) const
{
    return fGlyphs[glyphIndex];
}


U_NAMESPACE_END
#endif

