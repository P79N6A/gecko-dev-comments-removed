




#include "mozilla/ArrayUtils.h"

#include "nsHTMLEntities.h"



#include "nsString.h"
#include "nsCRT.h"
#include "pldhash.h"

using namespace mozilla;

struct EntityNode {
  const char* mStr; 
  int32_t       mUnicode;
};

struct EntityNodeEntry : public PLDHashEntryHdr
{
  const EntityNode* node;
}; 

static bool
  matchNodeString(PLDHashTable*, const PLDHashEntryHdr* aHdr,
                  const void* key)
{
  const EntityNodeEntry* entry = static_cast<const EntityNodeEntry*>(aHdr);
  const char* str = static_cast<const char*>(key);
  return (nsCRT::strcmp(entry->node->mStr, str) == 0);
}

static bool
  matchNodeUnicode(PLDHashTable*, const PLDHashEntryHdr* aHdr,
                   const void* key)
{
  const EntityNodeEntry* entry = static_cast<const EntityNodeEntry*>(aHdr);
  const int32_t ucode = NS_PTR_TO_INT32(key);
  return (entry->node->mUnicode == ucode);
}

static PLDHashNumber
  hashUnicodeValue(PLDHashTable*, const void* key)
{
  
  return PLDHashNumber(NS_PTR_TO_INT32(key));
  }


static const PLDHashTableOps EntityToUnicodeOps = {
  PL_DHashStringKey,
  matchNodeString,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr,
}; 

static const PLDHashTableOps UnicodeToEntityOps = {
  hashUnicodeValue,
  matchNodeUnicode,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr,
};

static PLDHashTable gEntityToUnicode;
static PLDHashTable gUnicodeToEntity;
static nsrefcnt gTableRefCnt = 0;

#define HTML_ENTITY(_name, _value) { #_name, _value },
static const EntityNode gEntityArray[] = {
#include "nsHTMLEntityList.h"
};
#undef HTML_ENTITY

#define NS_HTML_ENTITY_COUNT ((int32_t)ArrayLength(gEntityArray))

nsresult
nsHTMLEntities::AddRefTable(void)
{
  if (!gTableRefCnt) {
    if (!PL_DHashTableInit(&gEntityToUnicode, &EntityToUnicodeOps,
                           sizeof(EntityNodeEntry),
                           fallible_t(), NS_HTML_ENTITY_COUNT)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!PL_DHashTableInit(&gUnicodeToEntity, &UnicodeToEntityOps,
                           sizeof(EntityNodeEntry),
                           fallible_t(), NS_HTML_ENTITY_COUNT)) {
      PL_DHashTableFinish(&gEntityToUnicode);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    for (const EntityNode *node = gEntityArray,
                 *node_end = ArrayEnd(gEntityArray);
         node < node_end; ++node) {

      
      EntityNodeEntry* entry =
        static_cast<EntityNodeEntry*>
                   (PL_DHashTableAdd(&gEntityToUnicode, node->mStr));
      NS_ASSERTION(entry, "Error adding an entry");
      
      if (!entry->node)
        entry->node = node;

      
      entry = static_cast<EntityNodeEntry*>
                         (PL_DHashTableAdd(&gUnicodeToEntity,
                                           NS_INT32_TO_PTR(node->mUnicode)));
      NS_ASSERTION(entry, "Error adding an entry");
      
      if (!entry->node)
        entry->node = node;
    }
#ifdef DEBUG
    PL_DHashMarkTableImmutable(&gUnicodeToEntity);
    PL_DHashMarkTableImmutable(&gEntityToUnicode);
#endif
  }
  ++gTableRefCnt;
  return NS_OK;
}

void
nsHTMLEntities::ReleaseTable(void)
{
  if (--gTableRefCnt != 0)
    return;

  if (gEntityToUnicode.ops) {
    PL_DHashTableFinish(&gEntityToUnicode);
  }
  if (gUnicodeToEntity.ops) {
    PL_DHashTableFinish(&gUnicodeToEntity);
  }
}

int32_t
nsHTMLEntities::EntityToUnicode(const nsCString& aEntity)
{
  NS_ASSERTION(gEntityToUnicode.ops, "no lookup table, needs addref");
  if (!gEntityToUnicode.ops)
    return -1;

    
    

    if(';'==aEntity.Last()) {
      nsAutoCString temp(aEntity);
      temp.Truncate(aEntity.Length()-1);
      return EntityToUnicode(temp);
    }
      
  EntityNodeEntry* entry = 
    static_cast<EntityNodeEntry*>
               (PL_DHashTableLookup(&gEntityToUnicode, aEntity.get()));

  if (!entry || PL_DHASH_ENTRY_IS_FREE(entry))
  return -1;
        
  return entry->node->mUnicode;
}


int32_t 
nsHTMLEntities::EntityToUnicode(const nsAString& aEntity) {
  nsAutoCString theEntity; theEntity.AssignWithConversion(aEntity);
  if(';'==theEntity.Last()) {
    theEntity.Truncate(theEntity.Length()-1);
  }

  return EntityToUnicode(theEntity);
}


const char*
nsHTMLEntities::UnicodeToEntity(int32_t aUnicode)
{
  NS_ASSERTION(gUnicodeToEntity.ops, "no lookup table, needs addref");
  EntityNodeEntry* entry =
    static_cast<EntityNodeEntry*>
               (PL_DHashTableLookup(&gUnicodeToEntity, NS_INT32_TO_PTR(aUnicode)));
                   
  if (!entry || PL_DHASH_ENTRY_IS_FREE(entry))
  return nullptr;
    
  return entry->node->mStr;
}

#ifdef DEBUG
#include <stdio.h>

class nsTestEntityTable {
public:
   nsTestEntityTable() {
     int32_t value;
     nsHTMLEntities::AddRefTable();

     
     for (int i = 0; i < NS_HTML_ENTITY_COUNT; ++i) {
       nsAutoString entity; entity.AssignWithConversion(gEntityArray[i].mStr);

       value = nsHTMLEntities::EntityToUnicode(entity);
       NS_ASSERTION(value != -1, "can't find entity");
       NS_ASSERTION(value == gEntityArray[i].mUnicode, "bad unicode value");

       entity.AssignWithConversion(nsHTMLEntities::UnicodeToEntity(value));
       NS_ASSERTION(entity.EqualsASCII(gEntityArray[i].mStr), "bad entity name");
     }

     
     value = nsHTMLEntities::EntityToUnicode(nsAutoCString("@"));
     NS_ASSERTION(value == -1, "found @");
     value = nsHTMLEntities::EntityToUnicode(nsAutoCString("zzzzz"));
     NS_ASSERTION(value == -1, "found zzzzz");
     nsHTMLEntities::ReleaseTable();
   }
};

#endif

