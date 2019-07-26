


















#include "offsetmap.h"

#include <string.h>                     
#include <stdio.h>                      
#include <algorithm>                    

using namespace std;

namespace CLD2 {


OffsetMap::OffsetMap() {
  Clear();
}

OffsetMap::~OffsetMap() {
}






void OffsetMap::Clear() {
  diffs_.clear();
  pending_op_ = COPY_OP;
  pending_length_ = 0;
  next_diff_sub_ = 0;
  current_lo_aoffset_ = 0;
  current_hi_aoffset_ = 0;
  current_lo_aprimeoffset_ = 0;
  current_hi_aprimeoffset_ = 0;
  current_diff_ = 0;
  max_aoffset_ = 0;           
  max_aprimeoffset_ = 0;      
}

static inline char OpPart(const char c) {
  return (c >> 6) & 3;
}
static inline char LenPart(const char c) {
  return c & 0x3f;
}


void OffsetMap::Printmap(const char* filename) {
  FILE* fout;
  bool needs_close = false;
  if (strcmp(filename, "stdout") == 0) {
    fout = stdout;
  } else if (strcmp(filename, "stderr") == 0) {
    fout = stderr;
  } else {
    fout = fopen(filename, "w");
    needs_close = true;
  }
  if (fout == NULL) {
    fprintf(stderr, "%s did not open\n", filename);
    return;
  }

  Flush();    
  fprintf(fout, "Offsetmap: %ld bytes\n", diffs_.size());
  for (int i = 0; i < static_cast<int>(diffs_.size()); ++i) {
    fprintf(fout, "%c%02d ", "&=+-"[OpPart(diffs_[i])], LenPart(diffs_[i]));
    if ((i % 20) == 19) {fprintf(fout, "\n");}
  }
  fprintf(fout, "\n");
  if (needs_close) {
    fclose(fout);
  }
}


void OffsetMap::Reset() {
  MaybeFlushAll();

  next_diff_sub_ = 0;
  current_lo_aoffset_ = 0;
  current_hi_aoffset_ = 0;
  current_lo_aprimeoffset_ = 0;
  current_hi_aprimeoffset_ = 0;
  current_diff_ = 0;
}



void OffsetMap::Copy(int bytes) {
  if (bytes == 0) {return;}
  max_aoffset_ += bytes;           
  max_aprimeoffset_ += bytes;      
  if (pending_op_ == COPY_OP) {
    pending_length_ += bytes;
  } else {
    Flush();
    pending_op_ = COPY_OP;
    pending_length_ = bytes;
  }
}



void OffsetMap::Insert(int bytes){
  if (bytes == 0) {return;}
  max_aprimeoffset_ += bytes;      
  if (pending_op_ == INSERT_OP) {
    pending_length_ += bytes;
  } else if ((bytes == 1) &&
             (pending_op_ == DELETE_OP) && (pending_length_ == 1)) {
    
    
    pending_op_ = COPY_OP;
  } else {
    Flush();
    pending_op_ = INSERT_OP;
    pending_length_ = bytes;
  }
}



void OffsetMap::Delete(int bytes){
  if (bytes == 0) {return;}
  max_aoffset_ += bytes;           
  if (pending_op_ == DELETE_OP) {
    pending_length_ += bytes;
    } else if ((bytes == 1) &&
               (pending_op_ == INSERT_OP) && (pending_length_ == 1)) {
      
      
      pending_op_ = COPY_OP;
  } else {
    Flush();
    pending_op_ = DELETE_OP;
    pending_length_ = bytes;
  }
}

void OffsetMap::Flush() {
  if (pending_length_ == 0) {
    return;
  }
  
  
  if ((pending_op_ == COPY_OP) && !diffs_.empty()) {
    char c = diffs_[diffs_.size() - 1];
    MapOp prior_op = static_cast<MapOp>(OpPart(c));
    int prior_len = LenPart(c);
    if ((prior_op == COPY_OP) && ((prior_len + pending_length_) <= 0x3f)) {
      diffs_[diffs_.size() - 1] += pending_length_;
      pending_length_ = 0;
      return;
    }
  }
  if (pending_length_ > 0x3f) {
    bool non_zero_emitted = false;
    for (int shift = 30; shift > 0; shift -= 6) {
      int prefix = (pending_length_ >> shift) & 0x3f;
      if ((prefix > 0) || non_zero_emitted) {
        Emit(PREFIX_OP, prefix);
        non_zero_emitted = true;
      }
    }
  }
  Emit(pending_op_, pending_length_ & 0x3f);
  pending_length_ = 0;
}



void OffsetMap::FlushAll() {
  Copy(1);
  Flush();
}


void OffsetMap::MaybeFlushAll() {
  if ((0 < pending_length_) || diffs_.empty()) {
    FlushAll();
  }
}


void OffsetMap::Emit(MapOp op, int len) {
  char c = (static_cast<char>(op) << 6) | (len & 0x3f);
  diffs_.push_back(c);
}

void OffsetMap::DumpString() {
  for (int i = 0; i < static_cast<int>(diffs_.size()); ++i) {
    fprintf(stderr, "%c%02d ", "&=+-"[OpPart(diffs_[i])], LenPart(diffs_[i]));
  }
  fprintf(stderr, "\n");

  
  fprintf(stderr, "       op      A =>  A'     (A forward-maps to A')\n");
  int aoffset = 0;
  int aprimeoffset = 0;
  int length = 0;
  for (int i = 0; i < static_cast<int>(diffs_.size()); ++i) {
    char c = diffs_[i];
    MapOp op = static_cast<MapOp>(OpPart(c));
    int len = LenPart(c);
    length = (length << 6) + len;
    if (op == COPY_OP) {
      aoffset += length;
      aprimeoffset += length;
      length = 0;
    } else if (op == INSERT_OP) {
      aoffset += 0;
      aprimeoffset += length;
      length = 0;
    } else if (op == DELETE_OP) {
      aoffset += length;
      aprimeoffset += 0;
      length = 0;
    } else {              
      
    }
    fprintf(stderr, "[%3d] %c%02d %6d %6d%s\n",
            i, "&=+-"[op], len,
            aoffset, aprimeoffset,
            (next_diff_sub_ == i) ? " <==next_diff_sub_" : "");

  }
  fprintf(stderr, "\n");
}

void OffsetMap::DumpWindow() {
  fprintf(stderr, "DumpWindow(A => A'): max_aoffset_ = %d, "
                  "max_aprimeoffset_ = %d, next_diff_sub_ = %d<br>\n",
          max_aoffset_, max_aprimeoffset_, next_diff_sub_);
  fprintf(stderr, "A  [%u..%u)\n",
          current_lo_aoffset_, current_hi_aoffset_);
  fprintf(stderr, "A' [%u..%u)\n",
          current_lo_aprimeoffset_, current_hi_aprimeoffset_);
  fprintf(stderr, "  diff = %d\n", current_diff_);
  DumpString();
}

























void OffsetMap::SetLeft() {
   current_lo_aoffset_ = 0;
   current_hi_aoffset_ = 0;
   current_lo_aprimeoffset_ = 0;
   current_hi_aprimeoffset_ = 0;
   current_diff_ = 0;
   next_diff_sub_ = 0;
}

void OffsetMap::SetRight() {
   current_lo_aoffset_ = max_aoffset_;
   current_hi_aoffset_ = max_aoffset_;
   current_lo_aprimeoffset_ = max_aprimeoffset_;
   current_hi_aprimeoffset_ = max_aprimeoffset_;
   current_diff_ = max_aprimeoffset_ - max_aoffset_;
   next_diff_sub_ = 0;
}



int OffsetMap::Backup(int sub) {
  if (sub <= 0) {return 0;}
  --sub;
  while ((0 < sub) &&
         (static_cast<MapOp>(OpPart(diffs_[sub - 1]) == PREFIX_OP))) {
    --sub;
  }
  return sub;
}



int OffsetMap::ParseNext(int sub, MapOp* op, int* length) {
   *op = PREFIX_OP;
   *length = 0;
   char c;
   while ((sub < static_cast<int>(diffs_.size())) && (*op == PREFIX_OP)) {
     c = diffs_[sub++];
     *op = static_cast<MapOp>(OpPart(c));
     int len = LenPart(c);
     *length = (*length << 6) + len;
   }
   
   
   return sub;
}



int OffsetMap::ParsePrevious(int sub, MapOp* op, int* length) {
  sub = Backup(sub);
  return ParseNext(sub, op, length);
}


void OffsetMap::PrintPosition(const char* str) {
  MapOp op = PREFIX_OP;
  int length = 0;
  if ((0 < next_diff_sub_) && (next_diff_sub_ <= static_cast<int>(diffs_.size()))) {
    op = static_cast<MapOp>(OpPart(diffs_[next_diff_sub_ - 1]));
    length = LenPart(diffs_[next_diff_sub_ - 1]);
  }
  fprintf(stderr, "%s[%d] %c%02d = A[%d..%d) ==> A'[%d..%d)\n",
          str,
          next_diff_sub_, "&=+-"[op], length,
          current_lo_aoffset_, current_hi_aoffset_,
          current_lo_aprimeoffset_, current_hi_aprimeoffset_);
}



bool OffsetMap::MoveRight() {
  
  if (next_diff_sub_ >= static_cast<int>(diffs_.size())) {
    SetRight();
    return false;
  }
  
  MapOp op;
  int length;
  bool retval = true;
  
  next_diff_sub_ = ParseNext(next_diff_sub_, &op, &length);

  current_lo_aoffset_ = current_hi_aoffset_;
  current_lo_aprimeoffset_ = current_hi_aprimeoffset_;
  if (op == COPY_OP) {
    current_hi_aoffset_ = current_lo_aoffset_ + length;
    current_hi_aprimeoffset_ = current_lo_aprimeoffset_ + length;
  } else if (op == INSERT_OP) {
    current_hi_aoffset_ = current_lo_aoffset_ + 0;
    current_hi_aprimeoffset_ = current_lo_aprimeoffset_ + length;
  } else if (op == DELETE_OP) {
    current_hi_aoffset_ = current_lo_aoffset_ + length;
    current_hi_aprimeoffset_ = current_lo_aprimeoffset_ + 0;
  } else {
    SetRight();
    retval = false;
  }
  current_diff_ = current_lo_aprimeoffset_ - current_lo_aoffset_;
  return retval;
}



bool OffsetMap::MoveLeft() {
  
  if (next_diff_sub_ <= 0) {
    SetLeft();
    return false;
  }
  
  next_diff_sub_ = Backup(next_diff_sub_);
  if (next_diff_sub_ <= 0) {
    SetLeft();
    return false;
  }
  
  MapOp op;
  int length;
  bool retval = true;
  
  next_diff_sub_ = ParsePrevious(next_diff_sub_, &op, &length);

  current_hi_aoffset_ = current_lo_aoffset_;
  current_hi_aprimeoffset_ = current_lo_aprimeoffset_;
  if (op == COPY_OP) {
    current_lo_aoffset_ = current_hi_aoffset_ - length;
    current_lo_aprimeoffset_ = current_hi_aprimeoffset_ - length;
  } else if (op == INSERT_OP) {
    current_lo_aoffset_ = current_hi_aoffset_ - 0;
    current_lo_aprimeoffset_ = current_hi_aprimeoffset_ - length;
  } else if (op == DELETE_OP) {
    current_lo_aoffset_ = current_hi_aoffset_ - length;
    current_lo_aprimeoffset_ = current_hi_aprimeoffset_ - 0;
  } else {
    SetLeft();
    retval = false;
  }
  current_diff_ = current_lo_aprimeoffset_ - current_lo_aoffset_;
  return true;
}


int OffsetMap::MapBack(int aprimeoffset){
  MaybeFlushAll();
  if (aprimeoffset < 0) {return 0;}
  if (max_aprimeoffset_ <= aprimeoffset) {
     return (aprimeoffset - max_aprimeoffset_) + max_aoffset_;
  }

  
  
  bool ok = true;
  while (ok && (aprimeoffset < current_lo_aprimeoffset_)) {
    ok = MoveLeft();
  }
  while (ok && (current_hi_aprimeoffset_ <= aprimeoffset)) {
    ok = MoveRight();
  }
  

  int aoffset = aprimeoffset - current_diff_;
  if (aoffset >= current_hi_aoffset_) {
    
    aoffset = current_hi_aoffset_;
  }
  return aoffset;
}


int OffsetMap::MapForward(int aoffset){
  MaybeFlushAll();
  if (aoffset < 0) {return 0;}
  if (max_aoffset_ <= aoffset) {
     return (aoffset - max_aoffset_) + max_aprimeoffset_;
  }

  
  
  bool ok = true;
  while (ok && (aoffset < current_lo_aoffset_)) {
    ok = MoveLeft();
  }
  while (ok && (current_hi_aoffset_ <= aoffset)) {
    ok = MoveRight();
  }

  int aprimeoffset = aoffset + current_diff_;
  if (aprimeoffset >= current_hi_aprimeoffset_) {
    
    aprimeoffset = current_hi_aprimeoffset_;
  }
  return aprimeoffset;
}



bool OffsetMap::CopyInserts(OffsetMap* source, OffsetMap* dest) {
  bool ok = true;
  while (ok && (source->next_diff_sub_ != source->diffs_.size())) {
    ok = source->MoveRight();
    if (source->current_lo_aoffset_ != source->current_hi_aoffset_) {
      return false;
    }
    dest->Insert(
        source->current_hi_aprimeoffset_ - source->current_lo_aprimeoffset_);
  }
  return true;
}


bool OffsetMap::CopyDeletes(OffsetMap* source, OffsetMap* dest) {
  bool ok = true;
  while (ok && (source->next_diff_sub_ != source->diffs_.size())) {
    ok = source->MoveRight();
    if (source->current_lo_aprimeoffset_ != source->current_hi_aprimeoffset_) {
      return false;
    }
    dest->Delete(source->current_hi_aoffset_ - source->current_lo_aoffset_);
  }
  return true;
}


void OffsetMap::ComposeOffsetMap(
    OffsetMap* g, OffsetMap* f, OffsetMap* h) {
  h->Clear();
  f->Reset();
  g->Reset();

  int lo = 0;
  for (;;) {
     
     
     if (lo >= g->current_hi_aoffset_ && CopyInserts(g, h)) {
       if (lo >= f->current_hi_aprimeoffset_ && CopyDeletes(f, h)) {
          
          
       }

       
       
       
       
       
       h->Flush();
       return;
     }

     
     
     if (lo >= f->current_hi_aprimeoffset_) {
       if (!CopyDeletes(f, h)) {
          
          
       }
     }

     
     int hi = min(f->current_hi_aprimeoffset_, g->current_hi_aoffset_);
     if (f->current_lo_aoffset_ != f->current_hi_aoffset_ &&
         g->current_lo_aprimeoffset_ != g->current_hi_aprimeoffset_) {
       h->Copy(hi - lo);
     } else if (f->current_lo_aoffset_ != f->current_hi_aoffset_) {
       h->Delete(hi - lo);
     } else if (g->current_lo_aprimeoffset_ != g->current_hi_aprimeoffset_) {
       h->Insert(hi - lo);
     }

     lo = hi;
  }
}


void OffsetMap::StuffIt(const string& diffs,
                        int max_aoffset, int max_aprimeoffset) {
  Clear();
  diffs_ = diffs;
  max_aoffset_ = max_aoffset;
  max_aprimeoffset_ = max_aprimeoffset;
}


}  

