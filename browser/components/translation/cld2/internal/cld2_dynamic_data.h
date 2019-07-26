













#ifndef CLD2_INTERNAL_CLD2_DYNAMIC_DATA_H_
#define CLD2_INTERNAL_CLD2_DYNAMIC_DATA_H_

#include "integral_types.h"
#include "cld2tablesummary.h"
#include "utf8statetable.h"
#include "scoreonescriptspan.h"

















































































































namespace CLD2DynamicData {

static const char* DATA_FILE_MARKER = "cld2_data_file00";
static const int DATA_FILE_MARKER_LENGTH = 16; 


bool mem_compare(const void* data1, const void* data2, const int length);


void setDebug(int debug);



typedef struct {
  CLD2::uint32 kCLDTableSizeOne;
  CLD2::uint32 kCLDTableSize;
  CLD2::uint32 kCLDTableKeyMask;
  CLD2::uint32 kCLDTableBuildDate;
  CLD2::uint32 startOf_kCLDTable;
  CLD2::uint32 lengthOf_kCLDTable;
  CLD2::uint32 startOf_kCLDTableInd;
  CLD2::uint32 lengthOf_kCLDTableInd;
  CLD2::uint32 startOf_kRecognizedLangScripts;
  CLD2::uint32 lengthOf_kRecognizedLangScripts;
} TableHeader;





typedef struct {
  
  char sanityString[DATA_FILE_MARKER_LENGTH];
  CLD2::uint32 totalFileSizeBytes;

  
  CLD2::uint32 utf8PropObj_state0;
  CLD2::uint32 utf8PropObj_state0_size;
  CLD2::uint32 utf8PropObj_total_size;
  CLD2::uint32 utf8PropObj_max_expand;
  CLD2::uint32 utf8PropObj_entry_shift;
  CLD2::uint32 utf8PropObj_bytes_per_entry;
  CLD2::uint32 utf8PropObj_losub;
  CLD2::uint32 utf8PropObj_hiadd;
  CLD2::uint32 startOf_utf8PropObj_state_table;
  CLD2::uint32 lengthOf_utf8PropObj_state_table;
  CLD2::uint32 startOf_utf8PropObj_remap_base;
  CLD2::uint32 lengthOf_utf8PropObj_remap_base;
  CLD2::uint32 startOf_utf8PropObj_remap_string;
  CLD2::uint32 lengthOf_utf8PropObj_remap_string;
  CLD2::uint32 startOf_utf8PropObj_fast_state;
  CLD2::uint32 lengthOf_utf8PropObj_fast_state;

  
  CLD2::uint32 startOf_kAvgDeltaOctaScore;
  CLD2::uint32 lengthOf_kAvgDeltaOctaScore;

  
  CLD2::uint32 numTablesEncoded;
  TableHeader* tableHeaders;
} FileHeader;




CLD2::uint32 calculateHeaderSize(CLD2::uint32 numTables);


void dumpHeader(FileHeader* header);



bool verify(const CLD2::ScoringTables* realData, const CLD2::ScoringTables* loadedData);


bool isLittleEndian();


bool coreAssumptionsOk();

} 
#endif  
