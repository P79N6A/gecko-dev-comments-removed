

















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_TOTE_H_
#define I18N_ENCODINGS_CLD2_INTERNAL_TOTE_H_

#include <stdio.h>
#include "integral_types.h"        

namespace CLD2 {










class Tote {
 public:
  Tote();
  ~Tote();
  void Reinit();
  void AddScoreCount();
  void Add(uint8 ikey, int idelta);
  void AddBytes(int ibytes) {byte_count_ += ibytes;}
  void CurrentTopThreeKeys(int* key3) const;
  int GetScoreCount() const {return score_count_;}
  int GetByteCount() const {return byte_count_;}
  int GetScore(int i) const {return score_[i];}
  void SetScoreCount(uint16 v) {score_count_ = v;}
  void SetScore(int i, int v) {score_[i] = v;}

 private:
  uint64 in_use_mask_;      
                            
  int byte_count_;          
  int score_count_;         
  union {
    uint64 gscore_[64];     
    uint16 score_[256];     
  };

};





class DocTote {
 public:
  DocTote();
  ~DocTote();
  void Reinit();
  void Add(uint16 ikey, int ibytes, int score, int ireliability);
  int Find(uint16 ikey);
  void AddClosePair(int subscr, int val) {closepair_[subscr] += val;}
  int CurrentTopKey();
  Tote* RunningScore() {return &runningscore_;}
  void Sort(int n);
  void Dump(FILE* f);

  int GetIncrCount() const {return incr_count_;}
  int GetClosePair(int subscr) const {return closepair_[subscr];}
  int MaxSize() const {return kMaxSize_;}
  uint16 Key(int i) const {return key_[i];}
  int Value(int i) const {return value_[i];}      
  int Score(int i) const {return score_[i];}      
  int Reliability(int i) const {return reliability_[i];}
  void SetKey(int i, int v) {key_[i] = v;}
  void SetValue(int i, int v) {value_[i] = v;}
  void SetScore(int i, int v) {score_[i] = v;}
  void SetReliability(int i, int v) {reliability_[i] = v;}

  static const uint16 kUnusedKey = 0xFFFF;

 private:
  static const int kMaxSize_ = 24;
  static const int kMaxClosePairSize_ = 8;

  int incr_count_;         
  int sorted_;             
  Tote runningscore_;      
                           
  
  int closepair_[kMaxClosePairSize_];
  uint16 key_[kMaxSize_];   
  int value_[kMaxSize_];    
  int score_[kMaxSize_];    
  int reliability_[kMaxSize_];  
};

}       

#endif  
