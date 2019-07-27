





#ifndef __INDICREARRANGEMENTPROCESSOR2_H
#define __INDICREARRANGEMENTPROCESSOR2_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "StateTableProcessor2.h"
#include "IndicRearrangement.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class IndicRearrangementProcessor2 : public StateTableProcessor2
{
public:
    virtual void beginStateTable();

    virtual le_uint16 processStateEntry(LEGlyphStorage &glyphStorage, le_int32 &currGlyph, EntryTableIndex2 index, LEErrorCode &success);

    virtual void endStateTable();

    void doRearrangementAction(LEGlyphStorage &glyphStorage, IndicRearrangementVerb verb) const;

    IndicRearrangementProcessor2(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);
    virtual ~IndicRearrangementProcessor2();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

protected:
    le_int32 firstGlyph;
    le_int32 lastGlyph;

    LEReferenceToArrayOf<IndicRearrangementStateEntry2> entryTable;
    LEReferenceTo<IndicRearrangementSubtableHeader2> indicRearrangementSubtableHeader;

};

U_NAMESPACE_END
#endif
