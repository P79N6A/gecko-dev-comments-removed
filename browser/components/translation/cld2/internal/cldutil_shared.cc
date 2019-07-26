

















#include "cldutil_shared.h"
#include <string>

#include "cld2tablesummary.h"
#include "integral_types.h"
#include "port.h"
#include "utf8statetable.h"

namespace CLD2 {













static const uint32 kPreSpaceIndicator =  0x00004444;
static const uint32 kPostSpaceIndicator = 0x44440000;


static const uint32 kWordMask0[4] = {
  0xFFFFFFFF, 0x000000FF, 0x0000FFFF, 0x00FFFFFF
};

static const int kMinCJKUTF8CharBytes = 3;

static const int kMinGramCount = 3;
static const int kMaxGramCount = 16;

static const int UTFmax = 4;        




















































uint32 BiHashV2(const char* word_ptr, int bytecount) {
  if (bytecount == 0) {return 0;}
  const uint32* word_ptr32 = reinterpret_cast<const uint32*>(word_ptr);
  uint32 word0, word1;
  if (bytecount <= 4) {
    word0 = UNALIGNED_LOAD32(word_ptr32) & kWordMask0[bytecount & 3];
    word0 = word0 ^ (word0 >> 3);
    return word0;
  }
  
  word0 = UNALIGNED_LOAD32(word_ptr32);
  word0 = word0 ^ (word0 >> 3);
  word1 = UNALIGNED_LOAD32(word_ptr32 + 1) & kWordMask0[bytecount & 3];
  word1 = word1 ^ (word1 << 18);
  return word0 + word1;
}












































uint32 QuadHashV2Mix(const char* word_ptr, int bytecount, uint32 prepost) {
  const uint32* word_ptr32 = reinterpret_cast<const uint32*>(word_ptr);
  uint32 word0, word1, word2;
  if (bytecount <= 4) {
    word0 = UNALIGNED_LOAD32(word_ptr32) & kWordMask0[bytecount & 3];
    word0 = word0 ^ (word0 >> 3);
    return word0 ^ prepost;
  } else if (bytecount <= 8) {
    word0 = UNALIGNED_LOAD32(word_ptr32);
    word0 = word0 ^ (word0 >> 3);
    word1 = UNALIGNED_LOAD32(word_ptr32 + 1) & kWordMask0[bytecount & 3];
    word1 = word1 ^ (word1 << 4);
    return (word0 ^ prepost) + word1;
  }
  
  word0 = UNALIGNED_LOAD32(word_ptr32);
  word0 = word0 ^ (word0 >> 3);
  word1 = UNALIGNED_LOAD32(word_ptr32 + 1);
  word1 = word1 ^ (word1 << 4);
  word2 = UNALIGNED_LOAD32(word_ptr32 + 2) & kWordMask0[bytecount & 3];
  word2 = word2 ^ (word2 << 2);
  return (word0 ^ prepost) + word1 + word2;
}






uint32 QuadHashV2(const char* word_ptr, int bytecount) {
  if (bytecount == 0) {return 0;}
  uint32 prepost = 0;
  if (word_ptr[-1] == ' ') {prepost |= kPreSpaceIndicator;}
  if (word_ptr[bytecount] == ' ') {prepost |= kPostSpaceIndicator;}
  return QuadHashV2Mix(word_ptr, bytecount, prepost);
}





uint32 QuadHashV2Underscore(const char* word_ptr, int bytecount) {
  if (bytecount == 0) {return 0;}
  const char* local_word_ptr = word_ptr;
  int local_bytecount = bytecount;
  uint32 prepost = 0;
  if (local_word_ptr[0] == '_') {
    prepost |= kPreSpaceIndicator;
    ++local_word_ptr;
    --local_bytecount;
  }
  if (local_word_ptr[local_bytecount - 1] == '_') {
    prepost |= kPostSpaceIndicator;
    --local_bytecount;
  }
  return QuadHashV2Mix(local_word_ptr, local_bytecount, prepost);
}










uint64 OctaHash40Mix(const char* word_ptr, int bytecount, uint64 prepost) {
  const uint32* word_ptr32 = reinterpret_cast<const uint32*>(word_ptr);
  uint64 word0;
  uint64 word1;
  uint64 sum;

  if (word_ptr[-1] == ' ') {prepost |= kPreSpaceIndicator;}
  if (word_ptr[bytecount] == ' ') {prepost |= kPostSpaceIndicator;}
  switch ((bytecount - 1) >> 2) {
  case 0:       
    word0 = UNALIGNED_LOAD32(word_ptr32) & kWordMask0[bytecount & 3];
    sum = word0;
    word0 = word0 ^ (word0 >> 3);
    break;
  case 1:       
    word0 = UNALIGNED_LOAD32(word_ptr32);
    sum = word0;
    word0 = word0 ^ (word0 >> 3);
    word1 = UNALIGNED_LOAD32(word_ptr32 + 1) & kWordMask0[bytecount & 3];
    sum += word1;
    word1 = word1 ^ (word1 << 4);
    word0 += word1;
    break;
  case 2:       
    word0 = UNALIGNED_LOAD32(word_ptr32);
    sum = word0;
    word0 = word0 ^ (word0 >> 3);
    word1 = UNALIGNED_LOAD32(word_ptr32 + 1);
    sum += word1;
    word1 = word1 ^ (word1 << 4);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 2) & kWordMask0[bytecount & 3];
    sum += word1;
    word1 = word1 ^ (word1 << 2);
    word0 += word1;
    break;
  case 3:       
    word0 =UNALIGNED_LOAD32(word_ptr32);
    sum = word0;
    word0 = word0 ^ (word0 >> 3);
    word1 = UNALIGNED_LOAD32(word_ptr32 + 1);
    sum += word1;
    word1 = word1 ^ (word1 << 4);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 2);
    sum += word1;
    word1 = word1 ^ (word1 << 2);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 3) & kWordMask0[bytecount & 3];
    sum += word1;
    word1 = word1 ^ (word1 >> 8);
    word0 += word1;
    break;
  case 4:       
    word0 = UNALIGNED_LOAD32(word_ptr32);
    sum = word0;
    word0 = word0 ^ (word0 >> 3);
    word1 = UNALIGNED_LOAD32(word_ptr32 + 1);
    sum += word1;
    word1 = word1 ^ (word1 << 4);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 2);
    sum += word1;
    word1 = word1 ^ (word1 << 2);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 3);
    sum += word1;
    word1 = word1 ^ (word1 >> 8);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 4) & kWordMask0[bytecount & 3];
    sum += word1;
    word1 = word1 ^ (word1 >> 4);
    word0 += word1;
    break;
  default:      
    word0 = UNALIGNED_LOAD32(word_ptr32);
    sum = word0;
    word0 = word0 ^ (word0 >> 3);
    word1 = UNALIGNED_LOAD32(word_ptr32 + 1);
    sum += word1;
    word1 = word1 ^ (word1 << 4);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 2);
    sum += word1;
    word1 = word1 ^ (word1 << 2);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 3);
    sum += word1;
    word1 = word1 ^ (word1 >> 8);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 4);
    sum += word1;
    word1 = word1 ^ (word1 >> 4);
    word0 += word1;
    word1 = UNALIGNED_LOAD32(word_ptr32 + 5) & kWordMask0[bytecount & 3];
    sum += word1;
    word1 = word1 ^ (word1 >> 6);
    word0 += word1;
    break;
  }

  sum += (sum >> 17);             
  sum += (sum >> 9);              
  sum = (sum & 0xff) << 32;
  return (word0 ^ prepost) + sum;
}








uint64 OctaHash40(const char* word_ptr, int bytecount) {
  if (bytecount == 0) {return 0;}
  uint64 prepost = 0;
  if (word_ptr[-1] == ' ') {prepost |= kPreSpaceIndicator;}
  if (word_ptr[bytecount] == ' ') {prepost |= kPostSpaceIndicator;}
  return OctaHash40Mix(word_ptr, bytecount, prepost);
}









uint64 OctaHash40underscore(const char* word_ptr, int bytecount) {
  if (bytecount == 0) {return 0;}
  const char* local_word_ptr = word_ptr;
  int local_bytecount = bytecount;
  uint64 prepost = 0;
  if (local_word_ptr[0] == '_') {
    prepost |= kPreSpaceIndicator;
    ++local_word_ptr;
    --local_bytecount;
  }
  if (local_word_ptr[local_bytecount - 1] == '_') {
    prepost |= kPostSpaceIndicator;
    --local_bytecount;
  }
  return OctaHash40Mix(local_word_ptr, local_bytecount, prepost);
}




uint64 PairHash(uint64 worda_hash, uint64 wordb_hash) {
   return ((worda_hash >> 13) | (worda_hash << (64 - 13))) + wordb_hash;
}









int UniLen(const char* src) {
  const char* src_end = src;
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  return src_end - src;
}


int BiLen(const char* src) {
  const char* src_end = src;
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  return src_end - src;
}


int QuadLen(const char* src) {
  const char* src_end = src;
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  return src_end - src;
}


int OctaLen(const char* src) {
  const char* src_end = src;
  int charcount = 0;
  while (src_end[0] != ' ') {
    src_end += UTF8OneCharLen(src);
    ++charcount;
    if (charcount == 8) {break;}
  }
  return src_end - src;
}

}       





