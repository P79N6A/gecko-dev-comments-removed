





#ifndef __CONTEXTUALSUBSTITUTIONSUBTABLES_H
#define __CONTEXTUALSUBSTITUTIONSUBTABLES_H






#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "GlyphIterator.h"
#include "LookupProcessor.h"
#include "LETableReference.h"

U_NAMESPACE_BEGIN

struct SubstitutionLookupRecord
{
    le_uint16  sequenceIndex;
    le_uint16  lookupListIndex;
};

struct ContextualSubstitutionBase : GlyphSubstitutionSubtable
{
    static le_bool matchGlyphIDs(
        const TTGlyphID *glyphArray, le_uint16 glyphCount, GlyphIterator *glyphIterator,
        le_bool backtrack = FALSE);

    static le_bool matchGlyphClasses(
        const le_uint16 *classArray, le_uint16 glyphCount, GlyphIterator *glyphIterator,
        const ClassDefinitionTable *classDefinitionTable, le_bool backtrack = FALSE);

    static le_bool matchGlyphCoverages(
        const Offset *coverageTableOffsetArray, le_uint16 glyphCount,
        GlyphIterator *glyphIterator, const char *offsetBase, le_bool backtrack = FALSE);

    static void applySubstitutionLookups(
        const LookupProcessor *lookupProcessor, 
        const SubstitutionLookupRecord *substLookupRecordArray,
        le_uint16 substCount,
        GlyphIterator *glyphIterator,
        const LEFontInstance *fontInstance,
        le_int32 position,
        LEErrorCode& success);
};

struct ContextualSubstitutionSubtable : ContextualSubstitutionBase
{
    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};

struct ContextualSubstitutionFormat1Subtable : ContextualSubstitutionSubtable
{
    le_uint16  subRuleSetCount;
    Offset  subRuleSetTableOffsetArray[ANY_NUMBER];

    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};
LE_VAR_ARRAY(ContextualSubstitutionFormat1Subtable, subRuleSetTableOffsetArray)


struct SubRuleSetTable
{
    le_uint16  subRuleCount;
    Offset  subRuleTableOffsetArray[ANY_NUMBER];

};
LE_VAR_ARRAY(SubRuleSetTable, subRuleTableOffsetArray)


struct SubRuleTable
{
    le_uint16  glyphCount;
    le_uint16  substCount;
    TTGlyphID inputGlyphArray[ANY_NUMBER];
  
};
LE_VAR_ARRAY(SubRuleTable, inputGlyphArray)

struct ContextualSubstitutionFormat2Subtable : ContextualSubstitutionSubtable
{
    Offset  classDefTableOffset;
    le_uint16  subClassSetCount;
    Offset  subClassSetTableOffsetArray[ANY_NUMBER];

    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};
LE_VAR_ARRAY(ContextualSubstitutionFormat2Subtable, subClassSetTableOffsetArray)


struct SubClassSetTable
{
    le_uint16  subClassRuleCount;
    Offset  subClassRuleTableOffsetArray[ANY_NUMBER];
};
LE_VAR_ARRAY(SubClassSetTable, subClassRuleTableOffsetArray)



struct SubClassRuleTable
{
    le_uint16  glyphCount;
    le_uint16  substCount;
    le_uint16  classArray[ANY_NUMBER];
  
};
LE_VAR_ARRAY(SubClassRuleTable, classArray)






struct ContextualSubstitutionFormat3Subtable
{
    le_uint16  substFormat;
    le_uint16  glyphCount;
    le_uint16  substCount;
    Offset  coverageTableOffsetArray[ANY_NUMBER];
  

    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};
LE_VAR_ARRAY(ContextualSubstitutionFormat3Subtable, coverageTableOffsetArray)

struct ChainingContextualSubstitutionSubtable : ContextualSubstitutionBase
{
    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};

struct ChainingContextualSubstitutionFormat1Subtable : ChainingContextualSubstitutionSubtable
{
    le_uint16  chainSubRuleSetCount;
    Offset  chainSubRuleSetTableOffsetArray[ANY_NUMBER];

    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};
LE_VAR_ARRAY(ChainingContextualSubstitutionFormat1Subtable, chainSubRuleSetTableOffsetArray)


struct ChainSubRuleSetTable
{
    le_uint16  chainSubRuleCount;
    Offset  chainSubRuleTableOffsetArray[ANY_NUMBER];

};
LE_VAR_ARRAY(ChainSubRuleSetTable, chainSubRuleTableOffsetArray)


struct ChainSubRuleTable
{
    le_uint16  backtrackGlyphCount;
    TTGlyphID backtrackGlyphArray[ANY_NUMBER];
  
  
  
  
  
  
};
LE_VAR_ARRAY(ChainSubRuleTable, backtrackGlyphArray)

struct ChainingContextualSubstitutionFormat2Subtable : ChainingContextualSubstitutionSubtable
{
    Offset  backtrackClassDefTableOffset;
    Offset  inputClassDefTableOffset;
    Offset  lookaheadClassDefTableOffset;
    le_uint16  chainSubClassSetCount;
    Offset  chainSubClassSetTableOffsetArray[ANY_NUMBER];

    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};
LE_VAR_ARRAY(ChainingContextualSubstitutionFormat2Subtable, chainSubClassSetTableOffsetArray)

struct ChainSubClassSetTable
{
    le_uint16  chainSubClassRuleCount;
    Offset  chainSubClassRuleTableOffsetArray[ANY_NUMBER];
};
LE_VAR_ARRAY(ChainSubClassSetTable, chainSubClassRuleTableOffsetArray)



struct ChainSubClassRuleTable
{
    le_uint16  backtrackGlyphCount;
    le_uint16  backtrackClassArray[ANY_NUMBER];
  
  
  
  
  
  
};
LE_VAR_ARRAY(ChainSubClassRuleTable, backtrackClassArray)





struct ChainingContextualSubstitutionFormat3Subtable
{
    le_uint16  substFormat;
    le_uint16  backtrackGlyphCount;
    Offset  backtrackCoverageTableOffsetArray[ANY_NUMBER];
  
  
  
  
  
  

    le_uint32  process(const LookupProcessor *lookupProcessor, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode& success) const;
};
LE_VAR_ARRAY(ChainingContextualSubstitutionFormat3Subtable, backtrackCoverageTableOffsetArray)


U_NAMESPACE_END
#endif
