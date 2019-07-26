

















#ifndef UTIL_UTF8_OFFSETMAP_H_
#define UTIL_UTF8_OFFSETMAP_H_

#include <string>                       
#include "integral_types.h"             






















namespace CLD2 {

class OffsetMap {
 public:
  
  OffsetMap();
  ~OffsetMap();

  
  void Clear();

  
  
  void Copy(int bytes);

  
  
  void Insert(int bytes);

  
  
  void Delete(int bytes);

  
  void Printmap(const char* filename);

  
  
  
  void Reset();

  
  int MapBack(int aprimeoffset);

  
  int MapForward(int aoffset);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void ComposeOffsetMap(OffsetMap* g, OffsetMap* f, OffsetMap* h);

  
  void DumpWindow();

  
  void StuffIt(const std::string& diffs, int max_aoffset, int max_aprimeoffset);

 private:
  enum MapOp {PREFIX_OP, COPY_OP, INSERT_OP, DELETE_OP};

  void Flush();
  void FlushAll();
  void MaybeFlushAll();
  void Emit(MapOp op, int len);

  void SetLeft();
  void SetRight();

  
  
  int Backup(int sub);

  
  
  int ParseNext(int sub, MapOp* op, int* length);

  
  
  int ParsePrevious(int sub, MapOp* op, int* length);

  void PrintPosition(const char* str);

  bool MoveRight();     
  bool MoveLeft();      
  void DumpString();

  
  
  static bool CopyInserts(OffsetMap* source, OffsetMap* dest);

  
  
  static bool CopyDeletes(OffsetMap* source, OffsetMap* dest);

  std::string diffs_;
  MapOp pending_op_;
  uint32 pending_length_;

  
  int next_diff_sub_;
  int current_lo_aoffset_;
  int current_hi_aoffset_;
  int current_lo_aprimeoffset_;
  int current_hi_aprimeoffset_;
  int current_diff_;
  int max_aoffset_;
  int max_aprimeoffset_;
};

}  

#endif

