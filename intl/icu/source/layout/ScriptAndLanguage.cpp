




#include "LETypes.h"
#include "OpenTypeTables.h"
#include "OpenTypeUtilities.h"
#include "ScriptAndLanguage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEReferenceTo<LangSysTable> ScriptTable::findLanguage(const LETableReference& base, LETag languageTag, LEErrorCode &success, le_bool exactMatch) const
{
    le_uint16 count = SWAPW(langSysCount);
    Offset langSysTableOffset = exactMatch? 0 : SWAPW(defaultLangSysTableOffset);

    if (count > 0) {
      LEReferenceToArrayOf<TagAndOffsetRecord> langSysRecords(base, success, langSysRecordArray, count);
      Offset foundOffset =
        OpenTypeUtilities::getTagOffset(languageTag, langSysRecords, success);
      
      if (foundOffset != 0 && LE_SUCCESS(success)) {
        langSysTableOffset = foundOffset;
      }
    }
    
    if (langSysTableOffset != 0) {
      return LEReferenceTo<LangSysTable>(base, success, langSysTableOffset);
    }

    return LEReferenceTo<LangSysTable>();
}

LEReferenceTo<ScriptTable> ScriptListTable::findScript(const LETableReference &base, LETag scriptTag, LEErrorCode &success) const
{
    if (LE_FAILURE(success) ) {
      return LEReferenceTo<ScriptTable>(); 
    }
    











    le_uint16 count = SWAPW(scriptCount);

    if (count == 0) {
      return LEReferenceTo<ScriptTable>(); 
    }
    
    
    LEReferenceToArrayOf<ScriptRecord> oneElementTable(base, success, &scriptRecordArray[0], 1);
    
    if( LE_FAILURE(success) ) {
      return LEReferenceTo<ScriptTable>(); 
    }

    le_uint16 limit = ((SWAPW(scriptRecordArray[0].offset) - sizeof(ScriptListTable)) / sizeof(scriptRecordArray)) + ANY_NUMBER;
    Offset scriptTableOffset = 0;
    

    if (count > limit) {
        
        
        LEReferenceToArrayOf<ScriptRecord> scriptRecordArrayRef(base, success, &scriptRecordArray[0], limit);
        for(le_int32 s = 0; (s < limit)&&LE_SUCCESS(success); s += 1) {
          if (SWAPT(scriptRecordArrayRef(s,success).tag) == scriptTag) {
            scriptTableOffset = SWAPW(scriptRecordArrayRef(s,success).offset);
            break;
          }
        }
    } else {
      LEReferenceToArrayOf<ScriptRecord> scriptRecordArrayRef(base, success, &scriptRecordArray[0], count);
      
      scriptTableOffset = OpenTypeUtilities::getTagOffset(scriptTag, scriptRecordArrayRef, success);
    }

    if (scriptTableOffset != 0) {
      return LEReferenceTo<ScriptTable>(base, success, scriptTableOffset);
    }

  return LEReferenceTo<ScriptTable>();
}

LEReferenceTo<LangSysTable>  ScriptListTable::findLanguage(const LETableReference &base, LETag scriptTag, LETag languageTag, LEErrorCode &success, le_bool exactMatch) const 
{
  const LEReferenceTo<ScriptTable> scriptTable = findScript(base, scriptTag, success);

  if (scriptTable.isEmpty()) {
    return LEReferenceTo<LangSysTable>();
  }

  return scriptTable->findLanguage(scriptTable, languageTag, success, exactMatch).reparent(base);
}

U_NAMESPACE_END
