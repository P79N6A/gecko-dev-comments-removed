


















#ifdef COMPILER_MSVC



#pragma warning ( disable : 4309 )
#endif

#include "utf8statetable.h"

#include <stdint.h>                     
#include <string.h>                     

#include "integral_types.h"        
#include "stringpiece.h"
#include "offsetmap.h"


namespace CLD2 {

static const int kReplaceAndResumeFlag = 0x80; 
                                               
                                               
static const int kHtmlPlaintextFlag = 0x80;    
                                               























































































































static inline bool InStateZero(const UTF8ScanObj* st, const uint8* Tbl) {
  const uint8* Tbl0 = &st->state_table[st->state0];
  return (static_cast<uint32>(Tbl - Tbl0) < st->state0_size);
}

static inline bool InStateZero_2(const UTF8ReplaceObj_2* st,
                                 const unsigned short int* Tbl) {
  const unsigned short int* Tbl0 =  &st->state_table[st->state0];
  
  return (static_cast<uint32>(Tbl - Tbl0) < st->state0_size);
}




static bool IsPropObj(const UTF8StateMachineObj& obj) {
  return obj.fast_state == NULL
      && obj.max_expand == 0;
}

static bool IsPropObj_2(const UTF8StateMachineObj_2& obj) {
  return obj.fast_state == NULL
      && obj.max_expand == 0;
}

static bool IsScanObj(const UTF8StateMachineObj& obj) {
  return obj.fast_state != NULL
      && obj.max_expand == 0;
}

static bool IsReplaceObj(const UTF8StateMachineObj& obj) {
  
  
  return obj.max_expand > 0;
}

static bool IsReplaceObj_2(const UTF8StateMachineObj_2& obj) {
  return obj.max_expand > 0;
}




uint8 UTF8GenericProperty(const UTF8PropObj* st,
                          const uint8** src,
                          int* srclen) {
  if (*srclen <= 0) {
    return 0;
  }

  const uint8* lsrc = *src;
  const uint8* Tbl_0 = &st->state_table[st->state0];
  const uint8* Tbl = Tbl_0;
  int e;
  int eshift = st->entry_shift;

  
  unsigned char c = lsrc[0];
  if (static_cast<signed char>(c) >= 0) {           
    e = Tbl[c];
    *src += 1;
    *srclen -= 1;
  } else if (((c & 0xe0) == 0xc0) && (*srclen >= 2)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    *src += 2;
    *srclen -= 2;
  } else if (((c & 0xf0) == 0xe0) && (*srclen >= 3)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
    *src += 3;
    *srclen -= 3;
  }else if (((c & 0xf8) == 0xf0) && (*srclen >= 4)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[3]];
    *src += 4;
    *srclen -= 4;
  } else {                                                
    e = 0;
    *src += 1;
    *srclen -= 1;
  }
  return e;
}

bool UTF8HasGenericProperty(const UTF8PropObj& st, const char* src) {
  const uint8* lsrc = reinterpret_cast<const uint8*>(src);
  const uint8* Tbl_0 = &st.state_table[st.state0];
  const uint8* Tbl = Tbl_0;
  int e;
  int eshift = st.entry_shift;

  
  unsigned char c = lsrc[0];
  if (static_cast<signed char>(c) >= 0) {           
    e = Tbl[c];
  } else if ((c & 0xe0) == 0xc0) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
  } else if ((c & 0xf0) == 0xe0) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
  } else {                             
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[3]];
  }
  return e;
}









uint8 UTF8GenericPropertyBigOneByte(const UTF8PropObj* st,
                          const uint8** src,
                          int* srclen) {
  if (*srclen <= 0) {
    return 0;
  }

  const uint8* lsrc = *src;
  const uint8* Tbl_0 = &st->state_table[st->state0];
  const uint8* Tbl = Tbl_0;
  int e;
  int eshift = st->entry_shift;

  
  unsigned char c = lsrc[0];
  if (static_cast<signed char>(c) >= 0) {           
    e = Tbl[c];
    *src += 1;
    *srclen -= 1;
  } else if (((c & 0xe0) == 0xc0) && (*srclen >= 2)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    *src += 2;
    *srclen -= 2;
  } else if (((c & 0xf0) == 0xe0) && (*srclen >= 3)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << (eshift + 4)];  
    e = (reinterpret_cast<const int8*>(Tbl))[lsrc[1]];
    Tbl = &Tbl[e << eshift];          
    e = Tbl[lsrc[2]];
    *src += 3;
    *srclen -= 3;
  }else if (((c & 0xf8) == 0xf0) && (*srclen >= 4)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << (eshift + 4)];  
    e = (reinterpret_cast<const int8*>(Tbl))[lsrc[2]];
    Tbl = &Tbl[e << eshift];          
    e = Tbl[lsrc[3]];
    *src += 4;
    *srclen -= 4;
  } else {                                                
    e = 0;
    *src += 1;
    *srclen -= 1;
  }
  return e;
}



bool UTF8HasGenericPropertyBigOneByte(const UTF8PropObj& st, const char* src) {
  const uint8* lsrc = reinterpret_cast<const uint8*>(src);
  const uint8* Tbl_0 = &st.state_table[st.state0];
  const uint8* Tbl = Tbl_0;
  int e;
  int eshift = st.entry_shift;

  
  unsigned char c = lsrc[0];
  if (static_cast<signed char>(c) >= 0) {           
    e = Tbl[c];
  } else if ((c & 0xe0) == 0xc0) {    
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
  } else if ((c & 0xf0) == 0xe0) {    
    e = Tbl[c];
    Tbl = &Tbl_0[e << (eshift + 4)];  
    e = (reinterpret_cast<const int8*>(Tbl))[lsrc[1]];
    Tbl = &Tbl[e << eshift];          
    e = Tbl[lsrc[2]];
  } else {                            
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << (eshift + 4)];  
    e = (reinterpret_cast<const int8*>(Tbl))[lsrc[2]];
    Tbl = &Tbl[e << eshift];          
    e = Tbl[lsrc[3]];
  }
  return e;
}






uint8 UTF8GenericPropertyTwoByte(const UTF8PropObj_2* st,
                          const uint8** src,
                          int* srclen) {
  if (*srclen <= 0) {
    return 0;
  }

  const uint8* lsrc = *src;
  const unsigned short* Tbl_0 = &st->state_table[st->state0];
  const unsigned short* Tbl = Tbl_0;
  int e;
  int eshift = st->entry_shift;

  
  unsigned char c = lsrc[0];
  if (static_cast<signed char>(c) >= 0) {           
    e = Tbl[c];
    *src += 1;
    *srclen -= 1;
  } else if (((c & 0xe0) == 0xc0) && (*srclen >= 2)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    *src += 2;
    *srclen -= 2;
  } else if (((c & 0xf0) == 0xe0) && (*srclen >= 3)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
    *src += 3;
    *srclen -= 3;
  }else if (((c & 0xf8) == 0xf0) && (*srclen >= 4)) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[3]];
    *src += 4;
    *srclen -= 4;
  } else {                                                
    e = 0;
    *src += 1;
    *srclen -= 1;
  }
  return e;
}


bool UTF8HasGenericPropertyTwoByte(const UTF8PropObj_2& st, const char* src) {
  const uint8* lsrc = reinterpret_cast<const uint8*>(src);
  const unsigned short* Tbl_0 = &st.state_table[st.state0];
  const unsigned short* Tbl = Tbl_0;
  int e;
  int eshift = st.entry_shift;

  
  unsigned char c = lsrc[0];
  if (static_cast<signed char>(c) >= 0) {           
    e = Tbl[c];
  } else if ((c & 0xe0) == 0xc0) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
  } else if ((c & 0xf0) == 0xe0) {     
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
  } else {                             
    e = Tbl[c];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[1]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[2]];
    Tbl = &Tbl_0[e << eshift];
    e = Tbl[lsrc[3]];
  }
  return e;
}














int UTF8GenericScan(const UTF8ScanObj* st,
                    const StringPiece& str,
                    int* bytes_consumed) {
  int eshift = st->entry_shift;       
  

  const uint8* isrc =
    reinterpret_cast<const uint8*>(str.data());
  const uint8* src = isrc;
  const int len = str.length();
  const uint8* srclimit = isrc + len;
  const uint8* srclimit8 = srclimit - 7;
  *bytes_consumed = 0;
  if (len == 0) return kExitOK;

  const uint8* Tbl_0 = &st->state_table[st->state0];

DoAgain:
  
  int e = 0;
  uint8 c;

  
  
  
  
  const uint8* Tbl2 = &st->fast_state[0];
  uint32 losub = st->losub;
  uint32 hiadd = st->hiadd;
  while (src < srclimit8) {
    uint32 s0123 = (reinterpret_cast<const uint32 *>(src))[0];
    uint32 s4567 = (reinterpret_cast<const uint32 *>(src))[1];
    src += 8;
    
    uint32 temp = (s0123 - losub) | (s0123 + hiadd) |
                  (s4567 - losub) | (s4567 + hiadd);
    if ((temp & 0x80808080) != 0) {
      
      int e0123 = (Tbl2[src[-8]] | Tbl2[src[-7]]) |
                  (Tbl2[src[-6]] | Tbl2[src[-5]]);
      if (e0123 != 0) {src -= 8; break;}    
      e0123 = (Tbl2[src[-4]] | Tbl2[src[-3]]) |
              (Tbl2[src[-2]] | Tbl2[src[-1]]);
      if (e0123 != 0) {src -= 4; break;}    
      
    }
  }
  

  
  
  const uint8* Tbl = Tbl_0;
  while (src < srclimit) {
    c = *src;
    e = Tbl[c];
    src++;
    if (e >= kExitIllegalStructure) {break;}
    Tbl = &Tbl_0[e << eshift];
  }
  


  
  
  
  
  
  
  

  if (e >= kExitIllegalStructure) {
    
    src--;
    
    if (!InStateZero(st, Tbl)) {
      do {src--;} while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
    }
  } else if (!InStateZero(st, Tbl)) {
    
    e = kExitIllegalStructure;
    do {src--;} while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
  } else {
    
    e = kExitOK;
  }

  if (e == kExitDoAgain) {
    
    goto DoAgain;
  }

  *bytes_consumed = src - isrc;
  return e;
}





int UTF8GenericScanFastAscii(const UTF8ScanObj* st,
                    const StringPiece& str,
                    int* bytes_consumed) {
  const uint8* isrc =
    reinterpret_cast<const uint8*>(str.data());
  const uint8* src = isrc;
  const int len = str.length();
  const uint8* srclimit = isrc + len;
  const uint8* srclimit8 = srclimit - 7;
  *bytes_consumed = 0;
  if (len == 0) return kExitOK;

  int n;
  int rest_consumed;
  int exit_reason;
  do {
    
    while ((src < srclimit8) &&
           (((reinterpret_cast<const uint32*>(src)[0] |
              reinterpret_cast<const uint32*>(src)[1]) & 0x80808080) == 0)) {
      src += 8;
    }
    
    n = src - isrc;
    StringPiece str2(str.data() + n, str.length() - n);
    exit_reason = UTF8GenericScan(st, str2, &rest_consumed);
    src += rest_consumed;
  } while ( exit_reason == kExitDoAgain );

  *bytes_consumed = src - isrc;
  return exit_reason;
}




static int DoSpecialFixup(const unsigned char c,
                    const unsigned char** srcp, const unsigned char* srclimit,
                    unsigned char** dstp, unsigned char* dstlimit) {
  return 0;
}






static int UTF8GenericReplaceInternal(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed,
                    OffsetMap* offsetmap) {
  int eshift = st->entry_shift;
  int nEntries = (1 << eshift);       
  const uint8* isrc = reinterpret_cast<const uint8*>(istr.data());
  const int ilen = istr.length();
  const uint8* copystart = isrc;
  const uint8* src = isrc;
  const uint8* srclimit = src + ilen;
  *bytes_consumed = 0;
  *bytes_filled = 0;
  *chars_changed = 0;

  const uint8* odst = reinterpret_cast<const uint8*>(ostr.data());
  const int olen = ostr.length();
  uint8* dst = const_cast<uint8*>(odst);
  uint8* dstlimit = dst + olen;

  int total_changed = 0;

  
  
  if ((dstlimit - dst) < (srclimit - src)) {
    if (offsetmap != NULL) {
      offsetmap->Copy(src - copystart);
      copystart = src;
    }
    return kExitDstSpaceFull;
  }
  const uint8* Tbl_0 = &st->state_table[st->state0];

 Do_state_table:
  
  const uint8* Tbl = Tbl_0;
  int e = 0;
  uint8 c = 0;

 Do_state_table_newe:

  
  while (src < srclimit) {
    c = *src;
    e = Tbl[c];
    *dst = c;
    src++;
    dst++;
    if (e >= kExitIllegalStructure) {break;}
    Tbl = &Tbl_0[e << eshift];
  }
  

  
  
  
  
  
  
  
  

  if (e >= kExitIllegalStructure) {
    
    int offset = 0;
    switch (e) {
    
    
    case kExitReplace31:    
      dst -= 2;
      if (offsetmap != NULL) {
        offsetmap->Copy(src - copystart - 2);
        offsetmap->Delete(2);
        copystart = src;
      }
      dst[-1] = (unsigned char)Tbl[c + (nEntries * 1)];
      total_changed++;
      goto Do_state_table;
    case kExitReplace32:    
      dst--;
      if (offsetmap != NULL) {
        offsetmap->Copy(src - copystart - 1);
        offsetmap->Delete(1);
        copystart = src;
      }
      dst[-2] = (unsigned char)Tbl[c + (nEntries * 2)];
      dst[-1] = (unsigned char)Tbl[c + (nEntries * 1)];
      total_changed++;
      goto Do_state_table;
    case kExitReplace21:    
      dst--;
      if (offsetmap != NULL) {
        offsetmap->Copy(src - copystart - 1);
        offsetmap->Delete(1);
        copystart = src;
      }
      dst[-1] = (unsigned char)Tbl[c + (nEntries * 1)];
      total_changed++;
      goto Do_state_table;
    case kExitReplace3:    
      dst[-3] = (unsigned char)Tbl[c + (nEntries * 3)];
      
    case kExitReplace2:    
      dst[-2] = (unsigned char)Tbl[c + (nEntries * 2)];
      
    case kExitReplace1:    
      dst[-1] = (unsigned char)Tbl[c + (nEntries * 1)];
      total_changed++;
      goto Do_state_table;
    case kExitReplace1S0:     
      dst[-1] = (unsigned char)Tbl[c + (256 * 1)];
      total_changed++;
      goto Do_state_table;
    
    case kExitReplaceOffset2:
      if ((nEntries != 256) && InStateZero(st, Tbl)) {
        
        
        offset += ((unsigned char)Tbl[c + (256 * 2)] << 8);
      } else {
        offset += ((unsigned char)Tbl[c + (nEntries * 2)] << 8);
      }
      
    case kExitSpecial:      
    case kExitReplaceOffset1:
      if ((nEntries != 256) && InStateZero(st, Tbl)) {
        
        
        offset += (unsigned char)Tbl[c + (256 * 1)];
      } else {
        offset += (unsigned char)Tbl[c + (nEntries * 1)];
      }
      {
        const RemapEntry* re = &st->remap_base[offset];
        int del_len = re->delete_bytes & ~kReplaceAndResumeFlag;
        int add_len = re->add_bytes & ~kHtmlPlaintextFlag;

        
        
        
        
        
        
        if (re->add_bytes & kHtmlPlaintextFlag) {
          
          if (!is_plain_text) {
            
            re = &st->remap_base[offset + 1];
            add_len = re->add_bytes & ~kHtmlPlaintextFlag;
          }
        }

        int string_offset = re->bytes_offset;
        
        uint8* newdst = dst - del_len + add_len;
        if ((dstlimit - newdst) < (srclimit - src)) {
          
          e = kExitDstSpaceFull;
          break;    
        }
        dst -= del_len;
        memcpy(dst, &st->remap_string[string_offset], add_len);
        dst += add_len;
        total_changed++;
        if (offsetmap != NULL) {
          if (add_len > del_len) {
            offsetmap->Copy(src - copystart);
            offsetmap->Insert(add_len - del_len);
            copystart = src;
          } else if (add_len < del_len) {
            offsetmap->Copy(src - copystart + add_len - del_len);
            offsetmap->Delete(del_len - add_len);
            copystart = src;
          }
        }
        if (re->delete_bytes & kReplaceAndResumeFlag) {
          
          
          e = st->remap_string[string_offset + add_len];
          Tbl = &Tbl_0[e << eshift];
          goto Do_state_table_newe;
        }
      }
      if (e == kExitRejectAlt) {break;}
      if (e != kExitSpecial) {goto Do_state_table;}

    
      
      
      
      

      
      
      {
        int srcdel = DoSpecialFixup(c, &src, srclimit, &dst, dstlimit);
        if (offsetmap != NULL) {
          if (srcdel != 0) {
            offsetmap->Copy(src - copystart - srcdel);
            offsetmap->Delete(srcdel);
            copystart = src;
          }
        }
      }
      goto Do_state_table;

    case kExitIllegalStructure:   
    case kExitReject:             
    case kExitRejectAlt:          
    default:                      
      break;
    }   

    
    
    

    
    src--;
    dst--;
    
    if (!InStateZero(st, Tbl)) {
      do {src--;dst--;} while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
    }
  } else if (!InStateZero(st, Tbl)) {
    
    
    e = kExitIllegalStructure;
    do {src--; dst--;} while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
  } else {
    
    
    e = kExitOK;
  }

  if (offsetmap != NULL) {
    if (src > copystart) {
      offsetmap->Copy(src - copystart);
      copystart = src;
    }
  }

  
  
  
  
  
  
  
  
  
  *bytes_consumed = src - isrc;
  *bytes_filled = dst - odst;
  *chars_changed = total_changed;
  return e;
}









static int UTF8GenericReplaceInternalTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed,
                    OffsetMap* offsetmap) {
  int eshift = st->entry_shift;
  int nEntries = (1 << eshift);       
  const uint8* isrc = reinterpret_cast<const uint8*>(istr.data());
  const int ilen = istr.length();
  const uint8* copystart = isrc;
  const uint8* src = isrc;
  const uint8* srclimit = src + ilen;
  *bytes_consumed = 0;
  *bytes_filled = 0;
  *chars_changed = 0;

  const uint8* odst = reinterpret_cast<const uint8*>(ostr.data());
  const int olen = ostr.length();
  uint8* dst = const_cast<uint8*>(odst);
  uint8* dstlimit = dst + olen;

  *chars_changed = 0;

  int total_changed = 0;

  int src_lll = srclimit - src;
  int dst_lll = dstlimit - dst;


  
  
  if ((dstlimit - dst) < (srclimit - src)) {
    if (offsetmap != NULL) {
      offsetmap->Copy(src - copystart);
      copystart = src;
    }
    return kExitDstSpaceFull_2;
  }
  const unsigned short* Tbl_0 = &st->state_table[st->state0];

 Do_state_table_2:
  
  const unsigned short* Tbl = Tbl_0;
  int e = 0;
  uint8 c = 0;

 Do_state_table_newe_2:

  
  while (src < srclimit) {
    c = *src;
    e = Tbl[c];
    *dst = c;
    src++;
    dst++;
    if (e >= kExitIllegalStructure_2) {break;}
    Tbl = &Tbl_0[e << eshift];
  }
  
  src_lll = src - isrc;
  dst_lll = dst - odst;

  
  
  
  
  
  
  
  

  if (e >= kExitIllegalStructure_2) {
    
    int offset = 0;
    switch (e) {
    
    
    case kExitReplace31_2:    
      dst -= 2;
      if (offsetmap != NULL) {
        offsetmap->Copy(src - copystart - 2);
        offsetmap->Delete(2);
        copystart = src;
      }
      dst[-1] = (unsigned char)(Tbl[c + (nEntries * 1)] & 0xff);
      total_changed++;
      goto Do_state_table_2;
    case kExitReplace32_2:    
      dst--;
      if (offsetmap != NULL) {
        offsetmap->Copy(src - copystart - 1);
        offsetmap->Delete(1);
        copystart = src;
      }
      dst[-2] = (unsigned char)(Tbl[c + (nEntries * 1)] >> 8 & 0xff);
      dst[-1] = (unsigned char)(Tbl[c + (nEntries * 1)] & 0xff);
      total_changed++;
      goto Do_state_table_2;
    case kExitReplace21_2:    
      dst--;
      if (offsetmap != NULL) {
        offsetmap->Copy(src - copystart - 1);
        offsetmap->Delete(1);
        copystart = src;
      }
      dst[-1] = (unsigned char)(Tbl[c + (nEntries * 1)] & 0xff);
      total_changed++;
      goto Do_state_table_2;
    case kExitReplace3_2:    
      dst[-3] = (unsigned char)(Tbl[c + (nEntries * 2)] & 0xff);
      
    case kExitReplace2_2:    
      dst[-2] = (unsigned char)(Tbl[c + (nEntries * 1)] >> 8 & 0xff);
      
    case kExitReplace1_2:    
      dst[-1] = (unsigned char)(Tbl[c + (nEntries * 1)] & 0xff);
      total_changed++;
      goto Do_state_table_2;
    case kExitReplace1S0_2:     
      dst[-1] = (unsigned char)(Tbl[c + (256 * 1)] & 0xff);
      total_changed++;
      goto Do_state_table_2;
    
    case kExitReplaceOffset2_2:
      if ((nEntries != 256) && InStateZero_2(st, Tbl)) {
        
        
        offset += ((unsigned char)(Tbl[c + (256 * 1)] >> 8 & 0xff) << 8);
      } else {
        offset += ((unsigned char)(Tbl[c + (nEntries * 1)] >> 8 & 0xff) << 8);
      }
      
    case kExitReplaceOffset1_2:
      if ((nEntries != 256) && InStateZero_2(st, Tbl)) {
        
        
        offset += (unsigned char)(Tbl[c + (256 * 1)] & 0xff);
      } else {
        offset += (unsigned char)(Tbl[c + (nEntries * 1)] & 0xff);
      }
      {
        const RemapEntry* re = &st->remap_base[offset];
        int del_len = re->delete_bytes & ~kReplaceAndResumeFlag;
        int add_len = re->add_bytes & ~kHtmlPlaintextFlag;
        
        
        
        
        
        
        if (re->add_bytes & kHtmlPlaintextFlag) {
          
          if (!is_plain_text) {
            
            re = &st->remap_base[offset + 1];
            add_len = re->add_bytes & ~kHtmlPlaintextFlag;
          }
        }

        
        int string_offset = re->bytes_offset;
        
        uint8* newdst = dst - del_len + add_len;
        if ((dstlimit - newdst) < (srclimit - src)) {
          
          e = kExitDstSpaceFull_2;
          break;    
        }
        dst -= del_len;
        memcpy(dst, &st->remap_string[string_offset], add_len);
        dst += add_len;
        if (offsetmap != NULL) {
          if (add_len > del_len) {
            offsetmap->Copy(src - copystart);
            offsetmap->Insert(add_len - del_len);
            copystart = src;
          } else if (add_len < del_len) {
            offsetmap->Copy(src - copystart + add_len - del_len);
            offsetmap->Delete(del_len - add_len);
            copystart = src;
          }
        }
        if (re->delete_bytes & kReplaceAndResumeFlag) {
          
          
          uint8 c1 = st->remap_string[string_offset + add_len];
          uint8 c2 = st->remap_string[string_offset + add_len + 1];
          e = (c1 << 8) | c2;
          Tbl = &Tbl_0[e << eshift];
          total_changed++;
          goto Do_state_table_newe_2;
        }
      }
      total_changed++;
      if (e == kExitRejectAlt_2) {break;}
      goto Do_state_table_2;

    case kExitSpecial_2:           
    case kExitIllegalStructure_2:  
    case kExitReject_2:            
                                   
    default:
      break;
    }   

    
    
    

    
    src--;
    dst--;
    
    if (!InStateZero_2(st, Tbl)) {
      do {src--;dst--;} while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
    }
  } else if (!InStateZero_2(st, Tbl)) {
    
    
    e = kExitIllegalStructure_2;

    do {src--; dst--;} while ((src > isrc) && ((src[0] & 0xc0) == 0x80));
  } else {
    
    
    e = kExitOK_2;
  }

  if (offsetmap != NULL) {
    if (src > copystart) {
      offsetmap->Copy(src - copystart);
      copystart = src;
    }
  }


  
  
  
  
  
  
  
  
  
  *bytes_consumed = src - isrc;
  *bytes_filled = dst - odst;
  *chars_changed = total_changed;
  return e;
}








int UTF8GenericReplace(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed,
                    OffsetMap* offsetmap) {
  StringPiece local_istr(istr.data(), istr.length());
  StringPiece local_ostr(ostr.data(), ostr.length());
  int total_consumed = 0;
  int total_filled = 0;
  int total_changed = 0;
  int local_bytes_consumed, local_bytes_filled, local_chars_changed;
  int e;
  do {
    e = UTF8GenericReplaceInternal(st,
                    local_istr, local_ostr, is_plain_text,
                    &local_bytes_consumed, &local_bytes_filled,
                    &local_chars_changed,
                    offsetmap);
    local_istr.remove_prefix(local_bytes_consumed);
    local_ostr.remove_prefix(local_bytes_filled);
    total_consumed += local_bytes_consumed;
    total_filled += local_bytes_filled;
    total_changed += local_chars_changed;
  } while ( e == kExitDoAgain );
  *bytes_consumed = total_consumed;
  *bytes_filled = total_filled;
  *chars_changed = total_changed;
  return e;
}


int UTF8GenericReplace(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed) {
  return UTF8GenericReplace(st,
                    istr,
                    ostr,
                    is_plain_text,
                    bytes_consumed,
                    bytes_filled,
                    chars_changed,
                    NULL);
}


int UTF8GenericReplace(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed) {
  bool is_plain_text = false;
  return UTF8GenericReplace(st,
                    istr,
                    ostr,
                    is_plain_text,
                    bytes_consumed,
                    bytes_filled,
                    chars_changed,
                    NULL);
}








int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed,
                    OffsetMap* offsetmap) {
  StringPiece local_istr(istr.data(), istr.length());
  StringPiece local_ostr(ostr.data(), ostr.length());
  int total_consumed = 0;
  int total_filled = 0;
  int total_changed = 0;
  int local_bytes_consumed, local_bytes_filled, local_chars_changed;
  int e;
  do {
    e = UTF8GenericReplaceInternalTwoByte(st,
                    local_istr, local_ostr, is_plain_text,
                    &local_bytes_consumed,
                    &local_bytes_filled,
                    &local_chars_changed,
                    offsetmap);
    local_istr.remove_prefix(local_bytes_consumed);
    local_ostr.remove_prefix(local_bytes_filled);
    total_consumed += local_bytes_consumed;
    total_filled += local_bytes_filled;
    total_changed += local_chars_changed;
  } while ( e == kExitDoAgain_2 );
  *bytes_consumed = total_consumed;
  *bytes_filled = total_filled;
  *chars_changed = total_changed;

  return e - kExitOK_2 + kExitOK;
}


int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed) {
  return UTF8GenericReplaceTwoByte(st,
                    istr,
                    ostr,
                    is_plain_text,
                    bytes_consumed,
                    bytes_filled,
                    chars_changed,
                    NULL);
}


int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed) {
  bool is_plain_text = false;
  return UTF8GenericReplaceTwoByte(st,
                    istr,
                    ostr,
                    is_plain_text,
                    bytes_consumed,
                    bytes_filled,
                    chars_changed,
                    NULL);
}







void UTF8TrimToChars(StringPiece* istr) {
  const char* src = istr->data();
  int len = istr->length();
  
  if (len == 0) {
    return;
  }

  
  if ( ((src[0] & 0xc0) != 0x80) &&
       (static_cast<signed char>(src[len - 1]) >= 0) ) {
    
    return;
  }

  
  const char* srclimit = src + len;
  
  const char* s = srclimit - 1;         
  while ((src <= s) && ((*s & 0xc0) == 0x80)) {
    s--;
  }
  
  if (src <= s) {
    int last_char_len = UTF8OneCharLen(s);
    if (s + last_char_len <= srclimit) {
      
      s += last_char_len;
    }
  }
  if (s != srclimit) {
    
    istr->remove_suffix(srclimit - s);
    
    if (istr->length() == 0) {
      return;
    }
  }

  
  len = istr->length();
  srclimit = src + len;
  s = src;                            
  
  while ((s < srclimit) && ((*s & 0xc0) == 0x80)) {
    s++;
  }
  if (s != src) {
    
    istr->remove_prefix(s - src);
  }
}

}       
