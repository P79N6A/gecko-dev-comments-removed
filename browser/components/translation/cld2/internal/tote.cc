

















#include "tote.h"
#include "lang_script.h"    

#include <stdio.h>
#include <string.h>         

namespace CLD2 {




Tote::Tote() {
  in_use_mask_ = 0;
  byte_count_ = 0;
  score_count_ = 0;
  
}

Tote::~Tote() {
}

void Tote::Reinit() {
  in_use_mask_ = 0;
  byte_count_ = 0;
  score_count_ = 0;
  
}

void Tote::AddScoreCount() {
  ++score_count_;
}


void Tote::Add(uint8 ikey, int idelta) {
  int key_group = ikey >> 2;
  uint64 groupmask = (1ULL << key_group);
  if ((in_use_mask_ & groupmask) == 0) {
    
    gscore_[key_group] = 0;
    in_use_mask_ |= groupmask;
  }
  score_[ikey] += idelta;
}



void Tote::CurrentTopThreeKeys(int* key3) const {
  key3[0] = -1;
  key3[1] = -1;
  key3[2] = -1;
  int score3[3] = {-1, -1, -1};
  uint64 tempmask = in_use_mask_;
  int base = 0;
  while (tempmask != 0) {
    if (tempmask & 1) {
      
      for (int i = 0; i < 4; ++i) {
        int insert_me = score_[base + i];
        
        if (insert_me > score3[2]) {
          
          int insert_at = 2;
          if (insert_me > score3[1]) {
            score3[2] = score3[1];
            key3[2] = key3[1];
            insert_at = 1;
            if (insert_me > score3[0]) {
              score3[1] = score3[0];
              key3[1] = key3[0];
              insert_at = 0;
            }
          }
          score3[insert_at] = insert_me;
          key3[insert_at] = base + i;
        }
      }
    }
    tempmask >>= 1;
    base += 4;
  }
}





DocTote::DocTote() {
  
  incr_count_ = 0;
  sorted_ = 0;
  memset(closepair_, 0, sizeof(closepair_));
  memset(key_, 0xFF, sizeof(key_));
}

DocTote::~DocTote() {
}

void DocTote::Reinit() {
  
  incr_count_ = 0;
  sorted_ = 0;
  memset(closepair_, 0, sizeof(closepair_));
  memset(key_, 0xFF, sizeof(key_));
  runningscore_.Reinit();
}



void DocTote::Add(uint16 ikey, int ibytes,
                              int score, int ireliability) {
  ++incr_count_;

  
  int sub0 = ikey & 15;
  if (key_[sub0] == ikey) {
    value_[sub0] += ibytes;
    score_[sub0] += score;
    reliability_[sub0] += ireliability * ibytes;
    return;
  }
  
  int sub1 = sub0 ^ 8;
  if (key_[sub1] == ikey) {
    value_[sub1] += ibytes;
    score_[sub1] += score;
    reliability_[sub1] += ireliability * ibytes;
    return;
  }
  
  int sub2 = (ikey & 7) + 16;
  if (key_[sub2] == ikey) {
    value_[sub2] += ibytes;
    score_[sub2] += score;
    reliability_[sub2] += ireliability * ibytes;
    return;
  }

  
  int alloc = -1;
  if (key_[sub0] == kUnusedKey) {
    alloc = sub0;
  } else if (key_[sub1] == kUnusedKey) {
    alloc = sub1;
  } else if (key_[sub2] == kUnusedKey) {
    alloc = sub2;
  } else {
    
    alloc = sub0;
    if (value_[sub1] < value_[alloc]) {alloc = sub1;}
    if (value_[sub2] < value_[alloc]) {alloc = sub2;}
  }
  key_[alloc] = ikey;
  value_[alloc] = ibytes;
  score_[alloc] = score;
  reliability_[alloc] = ireliability * ibytes;
  return;
}


int DocTote::Find(uint16 ikey) {
  if (sorted_) {
    
    for (int sub = 0; sub < kMaxSize_; ++sub) {
      if (key_[sub] == ikey) {return sub;}
    }
    return -1;
  }

  
  int sub0 = ikey & 15;
  if (key_[sub0] == ikey) {
    return sub0;
  }
  int sub1 = sub0 ^ 8;
  if (key_[sub1] == ikey) {
    return sub1;
  }
  int sub2 = (ikey & 7) + 16;
  if (key_[sub2] == ikey) {
    return sub2;
  }

  return -1;
}


int DocTote::CurrentTopKey() {
  int top_key = 0;
  int top_value = -1;
  for (int sub = 0; sub < kMaxSize_; ++sub) {
    if (key_[sub] == kUnusedKey) {continue;}
    if (top_value < value_[sub]) {
      top_value = value_[sub];
      top_key = key_[sub];
    }
  }
  return top_key;
}




void DocTote::Sort(int n) {
  
  for (int sub = 0; sub < n; ++sub) {
    if (key_[sub] == kUnusedKey) {value_[sub] = -1;}

    
    for (int sub2 = sub + 1; sub2 < kMaxSize_; ++sub2) {
      if (key_[sub2] == kUnusedKey) {value_[sub2] = -1;}
      if (value_[sub] < value_[sub2]) {
        
        uint16 tmpk = key_[sub];
        key_[sub] = key_[sub2];
        key_[sub2] = tmpk;

        int tmpv = value_[sub];
        value_[sub] = value_[sub2];
        value_[sub2] = tmpv;

        double tmps = score_[sub];
        score_[sub] = score_[sub2];
        score_[sub2] = tmps;

        int tmpr = reliability_[sub];
        reliability_[sub] = reliability_[sub2];
        reliability_[sub2] = tmpr;
      }
    }
  }
  sorted_ = 1;
}

void DocTote::Dump(FILE* f) {
  fprintf(f, "DocTote::Dump\n");
  for (int sub = 0; sub < kMaxSize_; ++sub) {
    if (key_[sub] != kUnusedKey) {
      Language lang = static_cast<Language>(key_[sub]);
      fprintf(f, "[%2d] %3s %6dB %5dp %4dR,\n", sub, LanguageCode(lang),
              value_[sub], score_[sub], reliability_[sub]);
    }
  }
  fprintf(f, "  %d chunks scored<br>\n", incr_count_);
}

}       

