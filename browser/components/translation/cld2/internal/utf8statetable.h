




















#ifndef UTIL_UTF8_UTF8STATETABLE_H_
#define UTIL_UTF8_UTF8STATETABLE_H_

#include <string>
#include "integral_types.h"             
#include "stringpiece.h"


namespace CLD2 {

class OffsetMap;





struct RemapEntry {
  uint8 delete_bytes;
  uint8 add_bytes;
  uint16 bytes_offset;
};





typedef enum {
  kExitDstSpaceFull = 239,
  kExitIllegalStructure,  
  kExitOK,                
  kExitReject,            
  kExitReplace1,
  kExitReplace2,
  kExitReplace3,
  kExitReplace21,
  kExitReplace31,
  kExitReplace32,
  kExitReplaceOffset1,
  kExitReplaceOffset2,
  kExitReplace1S0,
  kExitSpecial,
  kExitDoAgain,
  kExitRejectAlt,
  kExitNone               
} ExitReason;

typedef enum {
  kExitDstSpaceFull_2 = 32767,       
  kExitIllegalStructure_2,  
  kExitOK_2,                
  kExitReject_2,            
  kExitReplace1_2,
  kExitReplace2_2,
  kExitReplace3_2,
  kExitReplace21_2,
  kExitReplace31_2,
  kExitReplace32_2,
  kExitReplaceOffset1_2,
  kExitReplaceOffset2_2,
  kExitReplace1S0_2,
  kExitSpecial_2,
  kExitDoAgain_2,
  kExitRejectAlt_2,
  kExitNone_2               
} ExitReason_2;










typedef struct {
  const uint32 state0;
  const uint32 state0_size;
  const uint32 total_size;
  const int max_expand;
  const int entry_shift;
  const int bytes_per_entry;
  const uint32 losub;
  const uint32 hiadd;
  const uint8* state_table;
  const RemapEntry* remap_base;
  const uint8* remap_string;
  const uint8* fast_state;
} UTF8StateMachineObj;


typedef struct {
  const uint32 state0;
  const uint32 state0_size;
  const uint32 total_size;
  const int max_expand;
  const int entry_shift;
  const int bytes_per_entry;
  const uint32 losub;
  const uint32 hiadd;
  const unsigned short* state_table;
  const RemapEntry* remap_base;
  const uint8* remap_string;
  const uint8* fast_state;
} UTF8StateMachineObj_2;


typedef UTF8StateMachineObj UTF8PropObj;
typedef UTF8StateMachineObj UTF8ScanObj;
typedef UTF8StateMachineObj UTF8ReplaceObj;
typedef UTF8StateMachineObj_2 UTF8PropObj_2;
typedef UTF8StateMachineObj_2 UTF8ReplaceObj_2;






uint8 UTF8GenericProperty(const UTF8PropObj* st,
                          const uint8** src,
                          int* srclen);



bool UTF8HasGenericProperty(const UTF8PropObj& st, const char* src);








uint8 UTF8GenericPropertyBigOneByte(const UTF8PropObj* st,
                          const uint8** src,
                          int* srclen);







bool UTF8HasGenericPropertyBigOneByte(const UTF8PropObj& st, const char* src);




uint8 UTF8GenericPropertyTwoByte(const UTF8PropObj_2* st,
                          const uint8** src,
                          int* srclen);



bool UTF8HasGenericPropertyTwoByte(const UTF8PropObj_2& st, const char* src);




int UTF8GenericScan(const UTF8ScanObj* st,
                    const StringPiece& str,
                    int* bytes_consumed);









int UTF8GenericReplace(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed,
                    OffsetMap* offsetmap);


int UTF8GenericReplace(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed);


int UTF8GenericReplace(const UTF8ReplaceObj* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed);












int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed,
                    OffsetMap* offsetmap);


int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    bool is_plain_text,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed);


int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                    const StringPiece& istr,
                    StringPiece& ostr,
                    int* bytes_consumed,
                    int* bytes_filled,
                    int* chars_changed);


static const unsigned char kUTF8LenTbl[256] = {
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4
};

inline int UTF8OneCharLen(const char* in) {
  return kUTF8LenTbl[*reinterpret_cast<const uint8*>(in)];
}







void UTF8TrimToChars(StringPiece* istr);

}       

#endif  
