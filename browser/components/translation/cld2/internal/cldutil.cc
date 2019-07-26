


















#include "cldutil.h"
#include <string>

#include "cld2tablesummary.h"
#include "integral_types.h"
#include "port.h"
#include "utf8statetable.h"

namespace CLD2 {












static const int kMinCJKUTF8CharBytes = 3;

static const int kMinGramCount = 3;
static const int kMaxGramCount = 16;

static const int UTFmax = 4;        

  
  static const uint8 kSkipSpaceVowelContinue[256] = {
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,1,0,0,0,1,0,0, 0,1,0,0,0,0,0,1, 0,0,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,
    0,1,0,0,0,1,0,0, 0,1,0,0,0,0,0,1, 0,0,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,

    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  };

  
  static const uint8 kSkipSpaceContinue[256] = {
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  };


  
  static const uint8 kAdvanceOneChar[256] = {
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
  };

  
  static const uint8 kAdvanceOneCharSpace[256] = {
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  };




























void ProcessProbV2Tote(uint32 probs, Tote* tote) {
  uint8 prob123 = (probs >> 0) & 0xff;
  const uint8* prob123_entry = LgProb2TblEntry(prob123);

  uint8 top1 = (probs >> 8) & 0xff;
  if (top1 > 0) {tote->Add(top1, LgProb3(prob123_entry, 0));}
  uint8 top2 = (probs >> 16) & 0xff;
  if (top2 > 0) {tote->Add(top2, LgProb3(prob123_entry, 1));}
  uint8 top3 = (probs >> 24) & 0xff;
  if (top3 > 0) {tote->Add(top3, LgProb3(prob123_entry, 2));}
}


int GetLangScore(uint32 probs, uint8 pslang) {
  uint8 prob123 = (probs >> 0) & 0xff;
  const uint8* prob123_entry = LgProb2TblEntry(prob123);
  int retval = 0;
  uint8 top1 = (probs >> 8) & 0xff;
  if (top1 == pslang) {retval += LgProb3(prob123_entry, 0);}
  uint8 top2 = (probs >> 16) & 0xff;
  if (top2 == pslang) {retval += LgProb3(prob123_entry, 1);}
  uint8 top3 = (probs >> 24) & 0xff;
  if (top3 == pslang) {retval += LgProb3(prob123_entry, 2);}
  return retval;
}










int DoBigramScoreV3(const CLD2TableSummary* bigram_obj,
                         const char* isrc, int srclen, Tote* chunk_tote) {
  int hit_count = 0;
  const char* src = isrc;

  
  const uint8* usrc = reinterpret_cast<const uint8*>(src);
  const uint8* usrclimit1 = usrc + srclen - UTFmax;

  while (usrc < usrclimit1) {
    int len = kAdvanceOneChar[usrc[0]];
    int len2 = kAdvanceOneChar[usrc[len]] + len;

    if ((kMinCJKUTF8CharBytes * 2) <= len2) {      
      
      
      uint32 bihash = BiHashV2(reinterpret_cast<const char*>(usrc), len2);
      uint32 probs = QuadHashV3Lookup4(bigram_obj, bihash);
      
      probs = bigram_obj->kCLDTableInd[probs &
        ~bigram_obj->kCLDTableKeyMask];

      
      if (probs != 0) {
        ProcessProbV2Tote(probs, chunk_tote);
        ++hit_count;
      }
    }
    usrc += len;  
  }

  return hit_count;
}





int GetUniHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer) {
  const char* isrc = &text[letter_offset];
  const char* src = isrc;
  
  const char* srclimit = &text[letter_limit];

  
  const UTF8PropObj* unigram_obj =
    scoringcontext->scoringtables->unigram_obj;
  int next_base = hitbuffer->next_base;
  int next_base_limit = hitbuffer->maxscoringhits;

  
  if (src[0] == ' ') {++src;}   
  while (src < srclimit) {
    const uint8* usrc = reinterpret_cast<const uint8*>(src);
    int len = kAdvanceOneChar[usrc[0]];
    src += len;
    
    
    int propval = UTF8GenericPropertyBigOneByte(unigram_obj, &usrc, &len);
    if (propval > 0) {
      
      int indirect_subscr = propval;
      hitbuffer->base[next_base].offset = src - text;     
      hitbuffer->base[next_base].indirect = indirect_subscr;
      ++next_base;
    }

    if (next_base >= next_base_limit) {break;}
  }

  hitbuffer->next_base = next_base;

  
  int dummy_offset = src - text;
  hitbuffer->base[hitbuffer->next_base].offset = dummy_offset;
  hitbuffer->base[hitbuffer->next_base].indirect = 0;

  return src - text;
}



void GetBiHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer) {
  const char* isrc = &text[letter_offset];
  const char* src = isrc;
  
  const char* srclimit1 = &text[letter_limit];

  
  const CLD2TableSummary* deltabi_obj =
    scoringcontext->scoringtables->deltabi_obj;
  const CLD2TableSummary* distinctbi_obj =
    scoringcontext->scoringtables->distinctbi_obj;
  int next_delta = hitbuffer->next_delta;
  int next_delta_limit = hitbuffer->maxscoringhits;
  int next_distinct = hitbuffer->next_distinct;
  
  int next_distinct_limit = hitbuffer->maxscoringhits - 1;

  while (src < srclimit1) {
    const uint8* usrc = reinterpret_cast<const uint8*>(src);
    int len = kAdvanceOneChar[usrc[0]];
    int len2 = kAdvanceOneChar[usrc[len]] + len;

    if ((kMinCJKUTF8CharBytes * 2) <= len2) {      
      
      uint32 bihash = BiHashV2(src, len2);
      uint32 probs = QuadHashV3Lookup4(deltabi_obj, bihash);
      
      if (probs != 0) {
        
        int indirect_subscr = probs & ~deltabi_obj->kCLDTableKeyMask;
        hitbuffer->delta[next_delta].offset = src - text;
        hitbuffer->delta[next_delta].indirect = indirect_subscr;
        ++next_delta;
      }
      
      probs = QuadHashV3Lookup4(distinctbi_obj, bihash);
      if (probs != 0) {
        int indirect_subscr = probs & ~distinctbi_obj->kCLDTableKeyMask;
        hitbuffer->distinct[next_distinct].offset = src - text;
        hitbuffer->distinct[next_distinct].indirect = indirect_subscr;
        ++next_distinct;
      }
    }
    src += len;  

    
    if (next_delta >= next_delta_limit) {break;}
    if (next_distinct >= next_distinct_limit) {break;}
  }

  hitbuffer->next_delta = next_delta;
  hitbuffer->next_distinct = next_distinct;

  
  int dummy_offset = src - text;
  hitbuffer->delta[hitbuffer->next_delta].offset = dummy_offset;
  hitbuffer->delta[hitbuffer->next_delta].indirect = 0;
  hitbuffer->distinct[hitbuffer->next_distinct].offset = dummy_offset;
  hitbuffer->distinct[hitbuffer->next_distinct].indirect = 0;
}




int GetQuadHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer) {
  const char* isrc = &text[letter_offset];
  const char* src = isrc;
  
  const char* srclimit = &text[letter_limit];

  
  const CLD2TableSummary* quadgram_obj =
    scoringcontext->scoringtables->quadgram_obj;
  const CLD2TableSummary* quadgram_obj2 =
    scoringcontext->scoringtables->quadgram_obj2;
  int next_base = hitbuffer->next_base;
  int next_base_limit = hitbuffer->maxscoringhits;

  
  
  int next_prior_quadhash = 0;
  uint32 prior_quadhash[2] = {0, 0};

  
  if (src[0] == ' ') {++src;}   
  while (src < srclimit) {
    
    const char* src_end = src;
    src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
    src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
    const char* src_mid = src_end;
    src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
    src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
    int len = src_end - src;
    
    uint32 quadhash = QuadHashV2(src, len);

    
    if ((quadhash != prior_quadhash[0]) && (quadhash != prior_quadhash[1])) {
      
      uint32 indirect_flag = 0;   
      const CLD2TableSummary* hit_obj = quadgram_obj;
      uint32 probs = QuadHashV3Lookup4(quadgram_obj, quadhash);
      if ((probs == 0) && (quadgram_obj2->kCLDTableSize != 0)) {
        
        
        indirect_flag = 0x80000000u;
        hit_obj = quadgram_obj2;
        probs = QuadHashV3Lookup4(quadgram_obj2, quadhash);
      }
      if (probs != 0) {
        
        prior_quadhash[next_prior_quadhash] = quadhash;
        next_prior_quadhash = (next_prior_quadhash + 1) & 1;

        
        int indirect_subscr = probs & ~hit_obj->kCLDTableKeyMask;
        hitbuffer->base[next_base].offset = src - text;     
        
        hitbuffer->base[next_base].indirect = indirect_subscr | indirect_flag;
        ++next_base;
      }
    }

    
    if (src_end[0] == ' ') {
      src = src_end;
    } else {
      src = src_mid;
    }

    
    
    if (src < srclimit) {
      src += kAdvanceOneCharSpaceVowel[(uint8)src[0]];
    } else {
      
      src = srclimit;
    }

    if (next_base >= next_base_limit) {break;}
  }

  hitbuffer->next_base = next_base;

  
  int dummy_offset = src - text;
  hitbuffer->base[hitbuffer->next_base].offset = dummy_offset;
  hitbuffer->base[hitbuffer->next_base].indirect = 0;

  return src - text;
}










void GetOctaHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer) {
  const char* isrc = &text[letter_offset];
  const char* src = isrc;
  
  const char* srclimit = &text[letter_limit + 1];

  
  const CLD2TableSummary* deltaocta_obj =
    scoringcontext->scoringtables->deltaocta_obj;
  int next_delta = hitbuffer->next_delta;
  int next_delta_limit = hitbuffer->maxscoringhits;

  const CLD2TableSummary* distinctocta_obj =
    scoringcontext->scoringtables->distinctocta_obj;
  int next_distinct = hitbuffer->next_distinct;
  
  int next_distinct_limit = hitbuffer->maxscoringhits - 1;

  
  
  int next_prior_octahash = 0;
  uint64 prior_octahash[2] = {0, 0};

  
  int charcount = 0;
  
  if (src[0] == ' ') {++src;}

  
  const char* prior_word_start = src;
  const char* word_start = src;
  const char* word_end = word_start;
  while (src < srclimit) {
    
    if (src[0] == ' ') {
      int len = word_end - word_start;
      
      uint64 wordhash40 = OctaHash40(word_start, len);
      uint32 probs;

      
      
      if ((wordhash40 != prior_octahash[0]) &&
          (wordhash40 != prior_octahash[1])) {
        
        prior_octahash[next_prior_octahash] = wordhash40;
        next_prior_octahash = 1 - next_prior_octahash;    

        
        
        
        
        
        
        uint64 tmp_prior_hash = prior_octahash[next_prior_octahash];
        if ((tmp_prior_hash != 0) && (tmp_prior_hash != wordhash40)) {
          uint64 pair_hash = PairHash(tmp_prior_hash, wordhash40);
          probs = OctaHashV3Lookup4(distinctocta_obj, pair_hash);
          if (probs != 0) {
            int indirect_subscr = probs & ~distinctocta_obj->kCLDTableKeyMask;
            hitbuffer->distinct[next_distinct].offset = prior_word_start - text;
            hitbuffer->distinct[next_distinct].indirect = indirect_subscr;
            ++next_distinct;
          }
        }

        
        probs = OctaHashV3Lookup4(distinctocta_obj, wordhash40);
        if (probs != 0) {
          int indirect_subscr = probs & ~distinctocta_obj->kCLDTableKeyMask;
          hitbuffer->distinct[next_distinct].offset = word_start - text;
          hitbuffer->distinct[next_distinct].indirect = indirect_subscr;
          ++next_distinct;
        }

        
        probs = OctaHashV3Lookup4(deltaocta_obj, wordhash40);
        if (probs != 0) {
          
          int indirect_subscr = probs & ~deltaocta_obj->kCLDTableKeyMask;
          hitbuffer->delta[next_delta].offset = word_start - text;
          hitbuffer->delta[next_delta].indirect = indirect_subscr;
          ++next_delta;
        }
      }

      
      charcount = 0;
      prior_word_start = word_start;
      word_start = src + 1;   
      word_end = word_start;
    } else {
      ++charcount;
    }

    
    src += UTF8OneCharLen(src);
    if (charcount <= 8) {
      word_end = src;
    }
    
    if (next_delta >= next_delta_limit) {break;}
    if (next_distinct >= next_distinct_limit) {break;}
  }

  hitbuffer->next_delta = next_delta;
  hitbuffer->next_distinct = next_distinct;

  
  int dummy_offset = src - text;
  hitbuffer->delta[hitbuffer->next_delta].offset = dummy_offset;
  hitbuffer->delta[hitbuffer->next_delta].indirect = 0;
  hitbuffer->distinct[hitbuffer->next_distinct].offset = dummy_offset;
  hitbuffer->distinct[hitbuffer->next_distinct].indirect = 0;
}



















int ReliabilityDelta(int value1, int value2, int gramcount) {
  int max_reliability_percent = 100;
  if (gramcount < 8) {
    max_reliability_percent = 12 * gramcount;
  }
  int fully_reliable_thresh = (gramcount * 5) >> 3;     
  if (fully_reliable_thresh < kMinGramCount) {          
    fully_reliable_thresh = kMinGramCount;
  } else if (fully_reliable_thresh > kMaxGramCount) {
    fully_reliable_thresh = kMaxGramCount;
  }

  int delta = value1 - value2;
  if (delta >= fully_reliable_thresh) {return max_reliability_percent;}
  if (delta <= 0) {return 0;}
  return minint(max_reliability_percent,
                     (100 * delta) / fully_reliable_thresh);
}














static const double kRatio100 = 1.5;
static const double kRatio0 = 4.0;
int ReliabilityExpected(int actual_score_1kb, int expected_score_1kb) {
  if (expected_score_1kb == 0) {return 100;}    
  if (actual_score_1kb == 0) {return 0;}        
  double ratio;
  if (expected_score_1kb > actual_score_1kb) {
    ratio = (1.0 * expected_score_1kb) / actual_score_1kb;
  } else {
    ratio = (1.0 * actual_score_1kb) / expected_score_1kb;
  }
  
  
  
  if (ratio <= kRatio100) {return 100;}
  if (ratio > kRatio0) {return 0;}

  int percent_good = 100.0 * (kRatio0 - ratio) / (kRatio0 - kRatio100);
  return percent_good;
}




uint32 MakeLangProb(Language lang, int qprob) {
  uint32 pslang = PerScriptNumber(ULScript_Latin, lang);
  uint32 retval = (pslang << 8) | kLgProbV2TblBackmap[qprob];
  return retval;
}

}       





