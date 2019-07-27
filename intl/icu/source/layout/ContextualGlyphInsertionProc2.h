





#ifndef __CONTEXTUALGLYPHINSERTIONPROCESSOR2_H
#define __CONTEXTUALGLYPHINSERTIONPROCESSOR2_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor2.h"
#include "StateTableProcessor2.h"
#include "ContextualGlyphInsertionProc2.h"
#include "ContextualGlyphInsertion.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class ContextualGlyphInsertionProcessor2 : public StateTableProcessor2
{
public:
    virtual void beginStateTable();

    virtual le_uint16 processStateEntry(LEGlyphStorage &glyphStorage, 
                                        le_int32 &currGlyph, EntryTableIndex2 index, LEErrorCode &success);

    virtual void endStateTable();

    ContextualGlyphInsertionProcessor2(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);
    virtual ~ContextualGlyphInsertionProcessor2();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:
    ContextualGlyphInsertionProcessor2();

    







    void doInsertion(LEGlyphStorage &glyphStorage,
                              le_int16 atGlyph,
                              le_int16 &index,
                              le_int16 count,
                              le_bool isKashidaLike,
                              le_bool isBefore,
                              LEErrorCode &success);


protected:
    le_int32 markGlyph;
    LEReferenceToArrayOf<le_uint16> insertionTable;
    LEReferenceToArrayOf<ContextualGlyphInsertionStateEntry2> entryTable;
    LEReferenceTo<ContextualGlyphInsertionHeader2> contextualGlyphHeader;
};

U_NAMESPACE_END
#endif
