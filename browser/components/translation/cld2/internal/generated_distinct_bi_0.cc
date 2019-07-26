
















#include "cld2tablesummary.h"

namespace CLD2 {

static const uint32 kDistinctBiTableBuildDate = 20130101;    
static const uint32 kDistinctBiTableSize = 1;       
static const uint32 kDistinctBiTableKeyMask = 0xffffffff;    
static const char* const kDistinctBiTableRecognizedLangScripts = "";


static const IndirectProbBucket4 kDistinctBiTable[kDistinctBiTableSize] = {
  
  
  { {0x00000000,0x00000000,0x00000000,0x00000000}},	
};

static const uint32 kDistinctBiTableSizeOne = 1;    
static const uint32 kDistinctBiTableIndSize = 1;       
static const uint32 kDistinctBiTableInd[kDistinctBiTableIndSize] = {
  
  0x00000000, };

extern const CLD2TableSummary kDistinctBiTable_obj = {
  kDistinctBiTable,
  kDistinctBiTableInd,
  kDistinctBiTableSizeOne,
  kDistinctBiTableSize,
  kDistinctBiTableKeyMask,
  kDistinctBiTableBuildDate,
  kDistinctBiTableRecognizedLangScripts,
};

}       


