


















#include "getonescriptspan.h"
#include <string.h>

#include "fixunicodevalue.h"
#include "lang_script.h"
#include "port.h"
#include "utf8statetable.h"

#include "utf8prop_lettermarkscriptnum.h"
#include "utf8repl_lettermarklower.h"
#include "utf8scannot_lettermarkspecial.h"


namespace CLD2 {



extern const int kNameToEntitySize;
extern const CharIntPair kNameToEntity[];

static const int kMaxUpToWordBoundary = 50;       
                                                  
static const int kMaxAdvanceToWordBoundary = 10;  
                                                  
                                                  

static const char kSpecialSymbol[256] = {       
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,1,0,1,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
};



#define LT 0      // <
#define GT 1      // >
#define EX 2      // !
#define HY 3      // -
#define QU 4      // "
#define AP 5      // '
#define SL 6      // /
#define S_ 7
#define C_ 8
#define R_ 9
#define I_ 10
#define P_ 11
#define T_ 12
#define Y_ 13
#define L_ 14
#define E_ 15
#define CR 16     // <cr> or <lf>
#define NL 17     // non-letter: ASCII whitespace, digit, punctuation
#define PL 18     // possible letter, incl. &
#define xx 19     // <unused>


static const uint8 kCharToSub[256] = {
  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,CR,NL, NL,CR,NL,NL,
  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL,
  NL,EX,QU,NL, NL,NL,PL,AP, NL,NL,NL,NL, NL,HY,NL,SL,
  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL, LT,NL,GT,NL,

  PL,PL,PL,C_, PL,E_,PL,PL, PL,I_,PL,PL, L_,PL,PL,PL,
  P_,PL,R_,S_, T_,PL,PL,PL, PL,Y_,PL,NL, NL,NL,NL,NL,
  PL,PL,PL,C_, PL,E_,PL,PL, PL,I_,PL,PL, L_,PL,PL,PL,
  P_,PL,R_,S_, T_,PL,PL,PL, PL,Y_,PL,NL, NL,NL,NL,NL,

  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL,
  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL,
  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL,
  NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL, NL,NL,NL,NL,

  PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL,
  PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL,
  PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL,
  PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL, PL,PL,PL,PL,
};

#undef LT
#undef GT
#undef EX
#undef HY
#undef QU
#undef AP
#undef SL
#undef S_
#undef C_
#undef R_
#undef I_
#undef P_
#undef T_
#undef Y_
#undef L_
#undef E_
#undef CR
#undef NL
#undef PL
#undef xx


#define OK 0
#define X_ 1


static const int kMaxExitStateLettersMarksOnly = 1;
static const int kMaxExitStateAllText = 2;


















static const uint8 kTagParseTbl_0[] = {

   3, 2, 2, 2,  2, 2, 2,OK, OK,OK,OK,OK, OK,OK,OK,OK,  2, 2,OK,X_, 
  X_,X_,X_,X_, X_,X_,X_,X_, X_,X_,X_,X_, X_,X_,X_,X_, X_,X_,X_,X_, 
   3, 2, 2, 2,  2, 2, 2,OK, OK,OK,OK,OK, OK,OK,OK,OK,  2, 2,OK,X_, 
  X_, 2, 4, 9, 10,11, 9,13,  9, 9, 9, 9,  9, 9, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 5, 10,11, 9, 9,  9, 9, 9, 9,  9, 9, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 6, 10,11, 9, 9,  9, 9, 9, 9,  9, 9, 9, 9,  9, 9, 9,X_, 
   6, 6, 6, 7,  6, 6, 6, 6,  6, 6, 6, 6,  6, 6, 6, 6,  6, 6, 6,X_, 
   6, 6, 6, 8,  6, 6, 6, 6,  6, 6, 6, 6,  6, 6, 6, 6,  6, 6, 6,X_, 
   6, 2, 6, 8,  6, 6, 6, 6,  6, 6, 6, 6,  6, 6, 6, 6,  6, 6, 6,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9, 9, 9, 9,  9, 9, 9, 9,  9, 9, 9,X_, 
  10,10,10,10,  9,10,10,10, 10,10,10,10, 10,10,10,10, 12,10,10,X_, 
  11,11,11,11, 11, 9,11,11, 11,11,11,11, 11,11,11,11, 12,11,11,X_, 
  X_, 2,12,12, 12,12,12,12, 12,12,12,12, 12,12,12,12, 12,12,12,X_, 


  X_, 2, 9, 9, 10,11, 9, 9, 14, 9, 9, 9, 28, 9, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9,15, 9, 9,  9, 9, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9, 9,16, 9,  9, 9, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9, 9, 9,17,  9, 9, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9, 9, 9, 9, 18, 9, 9, 9,  9, 9, 9,X_, 
  X_,19, 9, 9, 10,11, 9, 9,  9, 9, 9, 9,  9, 9, 9, 9, 19,19, 9,X_, 
  20,19,19,19, 19,19,19,19, 19,19,19,19, 19,19,19,19, 19,19,19,X_, 
  19,19,19,19, 19,19,21,19, 19,19,19,19, 19,19,19,19, 19,19,19,X_, 
  19,19,19,19, 19,19,19,22, 19,19,19,19, 19,19,19,19, 21,21,19,X_, 
  19,19,19,19, 19,19,19,19, 23,19,19,19, 19,19,19,19, 19,19,19,X_, 
  19,19,19,19, 19,19,19,19, 19,24,19,19, 19,19,19,19, 19,19,19,X_, 
  19,19,19,19, 19,19,19,19, 19,19,25,19, 19,19,19,19, 19,19,19,X_, 
  19,19,19,19, 19,19,19,19, 19,19,19,26, 19,19,19,19, 19,19,19,X_, 
  19,19,19,19, 19,19,19,19, 19,19,19,19, 27,19,19,19, 19,19,19,X_, 
  19, 2,19,19, 19,19,19,19, 19,19,19,19, 19,19,19,19, 19,19,19,X_, 


  X_, 2, 9, 9, 10,11, 9, 9,  9, 9, 9, 9,  9,29, 9, 9,  9, 9, 9,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9, 9, 9, 9,  9, 9,30, 9,  9, 9, 9,X_, 
  X_, 2, 9, 9, 10,11, 9, 9,  9, 9, 9, 9,  9, 9, 9,31,  9, 9, 9,X_, 
  X_,32, 9, 9, 10,11, 9, 9,  9, 9, 9, 9,  9, 9, 9, 9, 32,32, 9,X_, 
  33,32,32,32, 32,32,32,32, 32,32,32,32, 32,32,32,32, 32,32,32,X_, 
  32,32,32,32, 32,32,34,32, 32,32,32,32, 32,32,32,32, 32,32,32,X_, 
  32,32,32,32, 32,32,32,35, 32,32,32,32, 32,32,32,32, 34,34,32,X_, 
  32,32,32,32, 32,32,32,32, 32,32,32,32, 36,32,32,32, 32,32,32,X_, 
  32,32,32,32, 32,32,32,32, 32,32,32,32, 32,37,32,32, 32,32,32,X_, 
  32,32,32,32, 32,32,32,32, 32,32,32,32, 32,32,38,32, 32,32,32,X_, 
  32,32,32,32, 32,32,32,32, 32,32,32,32, 32,32,32,39, 32,32,32,X_, 
  32, 2,32,32, 32,32,32,32, 32,32,32,32, 32,32,32,32, 32,32,32,X_, 
};

#undef OK
#undef X_

enum
{
  UTFmax        = 4,            
  Runesync      = 0x80,         
  Runeself      = 0x80,         
  Runeerror     = 0xFFFD,       
  Runemax       = 0x10FFFF,     
};


static char gDisplayPiece[32];
const uint8 gCharlen[16] = {1,1,1,1, 1,1,1,1, 1,1,1,1, 2,2,3,4};
char* DisplayPiece(const char* next_byte_, int byte_length_) {
  
  int k = 0;    
  int n = 0;    
  for (int i = 0; i < byte_length_; ++i) {
    char c = next_byte_[i];
    if ((c & 0xc0) != 0x80) {
      
      int charlen = gCharlen[static_cast<uint8>(c) >> 4];
      if (i + charlen > byte_length_) {break;} 
      if (k >= (32 - 7)) {break;}   
      if (n >= 8) {break;}          
      ++n;
    }
    if (c == '<') {
      memcpy(&gDisplayPiece[k], "&lt;", 4); k += 4;
    } else if (c == '>') {
      memcpy(&gDisplayPiece[k], "&gt;", 4); k += 4;
    } else if (c == '&') {
      memcpy(&gDisplayPiece[k], "&amp;", 5); k += 5;
    } else if (c == '\'') {
      memcpy(&gDisplayPiece[k], "&apos;", 6); k += 6;
    } else if (c == '"') {
      memcpy(&gDisplayPiece[k], "&quot;", 6); k += 6;
    } else {
      gDisplayPiece[k++] = c;
    }
  }
  gDisplayPiece[k++] = '\0';
  return gDisplayPiece;
}





int runetochar(char *str, const char32 *rune) {
  
  unsigned long c;

  
  c = *rune;
  if(c <= 0x7F) {
    str[0] = c;
    return 1;
  }

  
  if(c <= 0x07FF) {
    str[0] = 0xC0 | (c >> 1*6);
    str[1] = 0x80 | (c & 0x3F);
    return 2;
  }

  
  if (c > Runemax) {
    c = Runeerror;
  }

  
  if (c <= 0xFFFF) {
    str[0] = 0xE0 |  (c >> 2*6);
    str[1] = 0x80 | ((c >> 1*6) & 0x3F);
    str[2] = 0x80 |  (c & 0x3F);
    return 3;
  }

  
  str[0] = 0xF0 | (c >> 3*6);
  str[1] = 0x80 | ((c >> 2*6) & 0x3F);
  str[2] = 0x80 | ((c >> 1*6) & 0x3F);
  str[3] = 0x80 | (c & 0x3F);
  return 4;
}





int LookupEntity(const char* entity_name, int entity_len) {
  
  if (entity_len >= 16) {return -1;}    
  char temp[16];
  memcpy(temp, entity_name, entity_len);
  temp[entity_len] = '\0';
  int match = BinarySearch(temp, 0, kNameToEntitySize, kNameToEntity);
  if (match >= 0) {return kNameToEntity[match].i;}
  return -1;
}

bool ascii_isdigit(char c) {
  return ('0' <= c) && (c <= '9');
}
bool ascii_isxdigit(char c) {
  if (('0' <= c) && (c <= '9')) {return true;}
  if (('a' <= c) && (c <= 'f')) {return true;}
  if (('A' <= c) && (c <= 'F')) {return true;}
  return false;
}
bool ascii_isalnum(char c) {
  if (('0' <= c) && (c <= '9')) {return true;}
  if (('a' <= c) && (c <= 'z')) {return true;}
  if (('A' <= c) && (c <= 'Z')) {return true;}
  return false;
}
int hex_digit_to_int(char c) {
  if (('0' <= c) && (c <= '9')) {return c - '0';}
  if (('a' <= c) && (c <= 'f')) {return c - 'a' + 10;}
  if (('A' <= c) && (c <= 'F')) {return c - 'A' + 10;}
  return 0;
}

static int32 strto32_base10(const char* nptr, const char* limit,
                            const char **endptr) {
  *endptr = nptr;
  while (nptr < limit && *nptr == '0') {
    ++nptr;
  }
  if (nptr == limit || !ascii_isdigit(*nptr))
    return -1;
  const char* end_digits_run = nptr;
  while (end_digits_run < limit && ascii_isdigit(*end_digits_run)) {
    ++end_digits_run;
  }
  *endptr = end_digits_run;
  const int num_digits = end_digits_run - nptr;
  
  if (num_digits < 9 ||
      (num_digits == 10 && memcmp(nptr, "2147483647", 10) <= 0)) {
    int value = 0;
    for (; nptr < end_digits_run; ++nptr) {
      value *= 10;
      value += *nptr - '0';
    }
    
    
    return FixUnicodeValue(value);
  } else {
    
    
    return 0xFFFD;
  }
}

static int32 strto32_base16(const char* nptr, const char* limit,
                            const char **endptr) {
  *endptr = nptr;
  while (nptr < limit && *nptr == '0') {
    ++nptr;
  }
  if (nptr == limit || !ascii_isxdigit(*nptr)) {
    return -1;
  }
  const char* end_xdigits_run = nptr;
  while (end_xdigits_run < limit && ascii_isxdigit(*end_xdigits_run)) {
    ++end_xdigits_run;
  }
  *endptr = end_xdigits_run;
  const int num_xdigits = end_xdigits_run - nptr;
  
  if (num_xdigits < 8 || (num_xdigits == 8 && nptr[0] < '8')) {
    int value = 0;
    for (; nptr < end_xdigits_run; ++nptr) {
      value <<= 4;
      value += hex_digit_to_int(*nptr);
    }
    
    
    return FixUnicodeValue(value);
  } else {
    
    
    return 0xFFFD;
  }
}





int ReadEntity(const char* src, int srcn, int* src_consumed) {
  const char* const srcend = src + srcn;

  if (srcn == 0 || *src != '&') {      
    *src_consumed = 0;
    return -1;
  }
  *src_consumed = 1;                   

  
  
  
  
  const char* entstart, *entend;  
  entstart = src + 1;             
  int entval;                     
  if ( *entstart == '#' ) {       
    if ( entstart + 2 >= srcend ) {
      return -1;                  
    } else if ( entstart[1] == 'x' || entstart[1] == 'X' ) {   
      entval = strto32_base16(entstart + 2, srcend, &entend);
    } else {                                  
      entval = strto32_base10(entstart+1, srcend, &entend);
    }
    if (entval == -1 || entend > srcend) {
      return -1;                 
    }
  } else {                       
    for (entend = entstart;
         entend < srcend && ascii_isalnum(*entend);
         ++entend ) {
      
    }
    entval = LookupEntity(entstart, entend - entstart);
    if (entval < 0) {
      return -1;  
    }
    
    
    
    
    
    
    
    
    if ( entval >= 256 && !(entend < srcend && *entend == ';') ) {
      return -1;                 
    }
  }

  
  if ( entend < srcend && *entend == ';' ) {
    entend++;                    
  }
  *src_consumed = entend - src;
  return entval;
}




void EntityToBuffer(const char* src, int len, char* dst,
                    int* tlen, int* plen) {
  char32 entval = ReadEntity(src, len, tlen);

  

  
  if (entval > 0) {
    *plen = runetochar(dst, &entval);
  } else {
    
    *tlen = 1;
    *plen = 0;
  }
}


bool inline IsSpecial(char c) {
  if ((c & 0xe0) == 0x20) {
    return kSpecialSymbol[static_cast<uint8>(c)];
  }
  return false;
}



int ScanToLetterOrSpecial(const char* src, int len) {
  int bytes_consumed;
  StringPiece str(src, len);
  UTF8GenericScan(&utf8scannot_lettermarkspecial_obj, str, &bytes_consumed);
  return bytes_consumed;
}

















int ScanToPossibleLetter(const char* isrc, int len, int max_exit_state) {
  const uint8* src = reinterpret_cast<const uint8*>(isrc);
  const uint8* srclimit = src + len;
  const uint8* tagParseTbl = kTagParseTbl_0;
  int e = 0;
  while (src < srclimit) {
    e = tagParseTbl[kCharToSub[*src++]];
    if (e <= max_exit_state) {
      
      --src;
      break;
    }
    tagParseTbl = &kTagParseTbl_0[e * 20];
  }

  if (src >= srclimit) {
    
    
    
    return len;
  }

  
  if ((e != 0) && (e != 2)) {
    
    
    int offset = src - reinterpret_cast<const uint8*>(isrc);

    
    --offset;   
    while ((0 < offset) && (isrc[offset] != '<')) {
      
      --offset;
    }
    
    return offset + 1;
  }

  return src - reinterpret_cast<const uint8*>(isrc);
}


ScriptScanner::ScriptScanner(const char* buffer,
                             int buffer_length,
                             bool is_plain_text)
  : start_byte_(buffer),
  next_byte_(buffer),
  next_byte_limit_(buffer + buffer_length),
  byte_length_(buffer_length),
  is_plain_text_(is_plain_text),
  letters_marks_only_(true),
  one_script_only_(true),
  exit_state_(kMaxExitStateLettersMarksOnly) {
    script_buffer_ = new char[kMaxScriptBuffer];
    script_buffer_lower_ = new char[kMaxScriptLowerBuffer];
    map2original_.Clear();    
    map2uplow_.Clear();       
}


ScriptScanner::ScriptScanner(const char* buffer,
                             int buffer_length,
                             bool is_plain_text,
                             bool any_text,
                             bool any_script)
  : start_byte_(buffer),
  next_byte_(buffer),
  next_byte_limit_(buffer + buffer_length),
  byte_length_(buffer_length),
  is_plain_text_(is_plain_text),
  letters_marks_only_(!any_text),
  one_script_only_(!any_script),
  exit_state_(any_text ? kMaxExitStateAllText : kMaxExitStateLettersMarksOnly) {
    script_buffer_ = new char[kMaxScriptBuffer];
    script_buffer_lower_ = new char[kMaxScriptLowerBuffer];
    map2original_.Clear();    
    map2uplow_.Clear();       
}


ScriptScanner::~ScriptScanner() {
  delete[] script_buffer_;
  delete[] script_buffer_lower_;
}







int ScriptScanner::SkipToFrontOfSpan(const char* src, int len, int* script) {
  int sc = UNKNOWN_ULSCRIPT;
  int skip = 0;
  int tlen, plen;

  
  tlen = 0;
  while (skip < len) {
    
    
    skip += ScanToLetterOrSpecial(src + skip, len - skip);

    
    if (skip >= len) {
      
      *script = sc;
      return len;
    }

    
    if (IsSpecial(src[skip]) && !is_plain_text_) {
      if (src[skip] == '<') {
        
        tlen = ScanToPossibleLetter(src + skip, len - skip,
                                    exit_state_);
        sc = 0;
      } else if (src[skip] == '>') {
        
        tlen = 1;         
        sc = 0;
      } else if (src[skip] == '&') {
        
        char temp[4];
        EntityToBuffer(src + skip, len - skip,
                       temp, &tlen, &plen);
        sc = GetUTF8LetterScriptNum(temp);
      }
    } else {
      
      tlen = UTF8OneCharLen(src + skip);
      sc = GetUTF8LetterScriptNum(src + skip);
    }
    if (sc != 0) {break;}           
    skip += tlen;                   
  }

  *script = sc;
  return skip;
}




inline bool EqCase(char uplow, char c) {
  return (uplow | 0x20) == c;
}



inline bool NeqLetter(char c) {
  return c < 0x40;
}



inline bool WS(char c) {
  return (c == ' ') || (c == '\n');
}


static const char LF = '\n';














bool ScriptScanner::GetOneTextSpan(LangSpan* span) {
  span->text = script_buffer_;
  span->text_bytes = 0;
  span->offset = next_byte_ - start_byte_;
  span->ulscript = UNKNOWN_ULSCRIPT;
  span->lang = UNKNOWN_LANGUAGE;
  span->truncated = false;

  int put_soft_limit = kMaxScriptBytes - kWithinScriptTail;
  if ((kMaxScriptBytes <= byte_length_) &&
      (byte_length_ < (2 * kMaxScriptBytes))) {
    
    put_soft_limit = byte_length_ / 2;
  }

  script_buffer_[0] = ' ';  
  script_buffer_[1] = '\0';
  int take = 0;
  int put = 1;              
  int tlen, plen;

  if (byte_length_ <= 0) {
    return false;          
  }

  
  
  bool last_byte_was_space = false;
  while (take < byte_length_) {
    char c = next_byte_[take];
    if (c == '\r') {c = LF;}      
    if (c == '\n') {c = LF;}      

    if (IsSpecial(c) && !is_plain_text_) {
      if (c == '<') {
        
        c = ' ';                      
        
        if (take < (byte_length_ - 3)) {
          if (EqCase(next_byte_[take + 1], 'p') &&
              NeqLetter(next_byte_[take + 2])) {
            c = LF;
          }
          if (EqCase(next_byte_[take + 1], 'b') &&
              EqCase(next_byte_[take + 2], 'r') &&
              NeqLetter(next_byte_[take + 3])) {
            c = LF;
          }
          if (EqCase(next_byte_[take + 1], 't') &&
              EqCase(next_byte_[take + 2], 'r') &&
              NeqLetter(next_byte_[take + 3])) {
            c = LF;
          }
        }
        
        tlen = 1 + ScanToPossibleLetter(next_byte_ + take, byte_length_ - take,
                                    exit_state_);
        
        if (!last_byte_was_space || !WS(c)) {
          script_buffer_[put++] = c;      
          last_byte_was_space = WS(c);
        }
      } else if (c == '>') {
        
        tlen = 1;         
        script_buffer_[put++] = c;    
      } else if (c == '&') {
        
        EntityToBuffer(next_byte_ + take, byte_length_ - take,
                       script_buffer_ + put, &tlen, &plen);
        put += plen;                  
      }
      take += tlen;                   
    } else {
      
      if (!last_byte_was_space || !WS(c)) {
        script_buffer_[put++] = c;      
        last_byte_was_space = WS(c);
      }
      ++take;                         
    }

    if (WS(c) &&
        (put >= put_soft_limit)) {
      
      span->truncated = true;
      break;
    }
    if (put >= kMaxScriptBytes) {
      
      span->truncated = true;
      break;
    }
  }

  
  while ((0 < take) && ((next_byte_[take] & 0xc0) == 0x80)) {
    
    --take;
    --put;
  }

  
  next_byte_ += take;
  byte_length_ -= take;

  
  
  script_buffer_[put + 0] = ' ';
  script_buffer_[put + 1] = ' ';
  script_buffer_[put + 2] = ' ';
  script_buffer_[put + 3] = '\0';

  span->text_bytes = put;       
  return true;
}




bool ScriptScanner::GetOneScriptSpan(LangSpan* span) {
  if (!letters_marks_only_) {
    
    return GetOneTextSpan(span);
  }

  span->text = script_buffer_;
  span->text_bytes = 0;
  span->offset = next_byte_ - start_byte_;
  span->ulscript = UNKNOWN_ULSCRIPT;
  span->lang = UNKNOWN_LANGUAGE;
  span->truncated = false;

  

  int put_soft_limit = kMaxScriptBytes - kWithinScriptTail;
  if ((kMaxScriptBytes <= byte_length_) &&
      (byte_length_ < (2 * kMaxScriptBytes))) {
    
    put_soft_limit = byte_length_ / 2;
  }


  int spanscript;           
  int sc = UNKNOWN_ULSCRIPT;  
  int tlen = 0;
  int plen = 0;

  script_buffer_[0] = ' ';  
  script_buffer_[1] = '\0';
  int take = 0;
  int put = 1;              

  
  
  
  map2original_.Clear();
  map2original_.Delete(span->offset);   

  
  int skip = SkipToFrontOfSpan(next_byte_, byte_length_, &spanscript);
  next_byte_ += skip;
  byte_length_ -= skip;

  if (skip != 1) {
    map2original_.Delete(skip);
    map2original_.Insert(1);
  } else {
    map2original_.Copy(1);
  }
  if (byte_length_ <= 0) {
    map2original_.Reset();
    return false;               
  }

  
  span->ulscript = (ULScript)spanscript;


  
  
  while (take < byte_length_) {
    
    int letter_count = 0;              
    bool need_break = false;

    while (take < byte_length_) {
      
      if (IsSpecial(next_byte_[take]) && !is_plain_text_) {
        if (next_byte_[take] == '<') {
          
          sc = 0;
          break;
        } else if (next_byte_[take] == '>') {
          
          sc = 0;
          break;
        } else if (next_byte_[take] == '&') {
          
          EntityToBuffer(next_byte_ + take, byte_length_ - take,
                         script_buffer_ + put, &tlen, &plen);
          sc = GetUTF8LetterScriptNum(script_buffer_ + put);
        }
      } else {
        
        
        tlen = plen = UTF8OneCharLen(next_byte_ + take);
        if (take < (byte_length_ - 3)) {
          
          UNALIGNED_STORE32(script_buffer_ + put,
                            UNALIGNED_LOAD32(next_byte_ + take));

        } else {
          
          memcpy(script_buffer_ + put, next_byte_ + take, plen);
        }
        sc = GetUTF8LetterScriptNum(next_byte_ + take);
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      if ((sc != spanscript) && (sc != ULScript_Inherited)) {
        
        if (sc == ULScript_Common) {
          need_break = true;
        } else {
          
          int sc2 = GetUTF8LetterScriptNum(next_byte_ + take + tlen);
          if ((sc2 != ULScript_Common) && (sc2 != spanscript)) {
            
            if (one_script_only_) {
              need_break = true;
            }
          }
        }
      }
      if (need_break) {break;}  

      take += tlen;                   
      put += plen;                    

      
      if (tlen == plen) {
        map2original_.Copy(tlen);
      } else if (tlen < plen) {
        map2original_.Copy(tlen);
        map2original_.Insert(plen - tlen);
      } else {    
        map2original_.Copy(plen);
        map2original_.Delete(tlen - plen);
      }

      ++letter_count;
      if (put >= kMaxScriptBytes) {
        
        span->truncated = true;
        break;
      }
    }     

    
    while (take < byte_length_) {
      
      tlen = ScanToLetterOrSpecial(next_byte_ + take, byte_length_ - take);
      take += tlen;
      map2original_.Delete(tlen);
      if (take >= byte_length_) {break;}    

      
      if (IsSpecial(next_byte_[take]) && !is_plain_text_) {
        if (next_byte_[take] == '<') {
          
          tlen = ScanToPossibleLetter(next_byte_ + take, byte_length_ - take,
                                      exit_state_);
          sc = 0;
        } else if (next_byte_[take] == '>') {
          
          tlen = 1;         
          sc = 0;
        } else if (next_byte_[take] == '&') {
          
          EntityToBuffer(next_byte_ + take, byte_length_ - take,
                         script_buffer_ + put, &tlen, &plen);
          sc = GetUTF8LetterScriptNum(script_buffer_ + put);
        }
      } else {
        
        tlen = UTF8OneCharLen(next_byte_ + take);
        sc = GetUTF8LetterScriptNum(next_byte_ + take);
      }
      if (sc != 0) {break;}           
      take += tlen;                   
      map2original_.Delete(tlen);
    }     

    script_buffer_[put++] = ' ';
    map2original_.Insert(1);

    
    if ((sc != spanscript) && (sc != ULScript_Inherited)) {break;}
    if (put >= put_soft_limit) {
      
      span->truncated = true;
      break;
    }
  }

  
  while ((0 < take) && (take < byte_length_) &&
         ((next_byte_[take] & 0xc0) == 0x80)) {
    
    --take;
    --put;
  }

  
  next_byte_ += take;
  byte_length_ -= take;

  
  
  script_buffer_[put + 0] = ' ';
  script_buffer_[put + 1] = ' ';
  script_buffer_[put + 2] = ' ';
  script_buffer_[put + 3] = '\0';
  map2original_.Insert(4);
  map2original_.Reset();

  span->text_bytes = put;       
  return true;
}





void ScriptScanner::LowerScriptSpan(LangSpan* span) {
  
  
  
  map2uplow_.Clear();
  
  
  
  
  
  int consumed, filled, changed;
  StringPiece istr(span->text, span->text_bytes + 3);
  StringPiece ostr(script_buffer_lower_, kMaxScriptLowerBuffer);

  UTF8GenericReplace(&utf8repl_lettermarklower_obj,
                            istr, ostr, is_plain_text_,
                            &consumed, &filled, &changed, &map2uplow_);
  script_buffer_lower_[filled] = '\0';
  span->text = script_buffer_lower_;
  span->text_bytes = filled - 3;
  map2uplow_.Reset();
}




bool ScriptScanner::GetOneScriptSpanLower(LangSpan* span) {
  bool ok = GetOneScriptSpan(span);
  LowerScriptSpan(span);
  return ok;
}










int ScriptScanner::MapBack(int text_offset) {
  return map2original_.MapBack(map2uplow_.MapBack(text_offset));
}




int GetUTF8LetterScriptNum(const char* src) {
  int srclen = UTF8OneCharLen(src);
  const uint8* usrc = reinterpret_cast<const uint8*>(src);
  return UTF8GenericPropertyTwoByte(&utf8prop_lettermarkscriptnum_obj,
                                    &usrc, &srclen);
}

}  


