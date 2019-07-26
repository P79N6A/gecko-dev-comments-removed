













#ifndef CLD2_INTERNAL_CLD2_DYNAMIC_DATA_LOADER_H_
#define CLD2_INTERNAL_CLD2_DYNAMIC_DATA_LOADER_H_

#include "scoreonescriptspan.h"
#include "cld2_dynamic_data.h"

namespace CLD2DynamicDataLoader {




CLD2DynamicData::FileHeader* loadHeader(const char* fileName);








CLD2::ScoringTables* loadDataFile(const char* fileName,
  void** mmapAddressOut, int* mmapLengthOut);











void unloadData(CLD2::ScoringTables** scoringTables,
  void** mmapAddress, int* mmapLength);

} 
#endif  
