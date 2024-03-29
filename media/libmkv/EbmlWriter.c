








#include "EbmlWriter.h"
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <limits.h>
#include "EbmlBufferWriter.h"
#if defined(_MSC_VER)
#define LITERALU64(n) n
#else
#define LITERALU64(n) n##LLU
#endif

void Ebml_WriteLen(EbmlGlobal *glob, int64_t val) {
  
  unsigned char size = 8; 

  
  int64_t minVal = 0xff;

  for (size = 1; size < 8; size ++) {
    if (val < minVal)
      break;

    minVal = (minVal << 7);
  }

  val |= (((uint64_t)0x80) << ((size - 1) * 7));

  Ebml_Serialize(glob, (void *) &val, sizeof(val), size);
}

void Ebml_WriteString(EbmlGlobal *glob, const char *str) {
  const size_t size_ = strlen(str);
  const uint64_t  size = size_;
  Ebml_WriteLen(glob, size);
  


  Ebml_Write(glob, str, (unsigned long)size);
}

void Ebml_WriteUTF8(EbmlGlobal *glob, const wchar_t *wstr) {
  const size_t strlen = wcslen(wstr);

  


  const uint64_t  size = strlen;

  Ebml_WriteLen(glob, size);
  Ebml_Write(glob, wstr, (unsigned long)size);
}

void Ebml_WriteID(EbmlGlobal *glob, unsigned long class_id) {
  int len;

  if (class_id >= 0x01000000)
    len = 4;
  else if (class_id >= 0x00010000)
    len = 3;
  else if (class_id >= 0x00000100)
    len = 2;
  else
    len = 1;

  Ebml_Serialize(glob, (void *)&class_id, sizeof(class_id), len);
}

void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned long class_id, uint32_t ui) {
  unsigned char sizeSerialized = 8 | 0x80;
  Ebml_WriteID(glob, class_id);
  Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
  Ebml_Serialize(glob, &ui, sizeof(ui), 4);
}

void Ebml_SerializeUnsigned64(EbmlGlobal *glob, unsigned long class_id, uint64_t ui) {
  unsigned char sizeSerialized = 8 | 0x80;
  Ebml_WriteID(glob, class_id);
  Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
  Ebml_Serialize(glob, &ui, sizeof(ui), 8);
}

void Ebml_SerializeUnsigned(EbmlGlobal *glob, unsigned long class_id, unsigned long ui) {
  unsigned char size = 8; 
  unsigned char sizeSerialized = 0;
  unsigned long minVal;

  Ebml_WriteID(glob, class_id);
  minVal = 0x7fLU; 

  for (size = 1; size < 4; size ++) {
    if (ui < minVal) {
      break;
    }

    minVal <<= 7;
  }

  sizeSerialized = 0x80 | size;
  Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
  Ebml_Serialize(glob, &ui, sizeof(ui), size);
}

void Ebml_SerializeBinary(EbmlGlobal *glob, unsigned long class_id, unsigned long bin) {
  int size;
  for (size = 4; size > 1; size--) {
    if (bin & (unsigned int)0x000000ff << ((size - 1) * 8))
      break;
  }
  Ebml_WriteID(glob, class_id);
  Ebml_WriteLen(glob, size);
  Ebml_WriteID(glob, bin);
}

void Ebml_SerializeFloat(EbmlGlobal *glob, unsigned long class_id, double d) {
  unsigned char len = 0x88;

  Ebml_WriteID(glob, class_id);
  Ebml_Serialize(glob, &len, sizeof(len), 1);
  Ebml_Serialize(glob,  &d, sizeof(d), 8);
}

void Ebml_WriteSigned16(EbmlGlobal *glob, short val) {
  signed long out = ((val & 0x003FFFFF) | 0x00200000) << 8;
  Ebml_Serialize(glob, &out, sizeof(out), 3);
}

void Ebml_SerializeString(EbmlGlobal *glob, unsigned long class_id, const char *s) {
  Ebml_WriteID(glob, class_id);
  Ebml_WriteString(glob, s);
}

void Ebml_SerializeUTF8(EbmlGlobal *glob, unsigned long class_id, wchar_t *s) {
  Ebml_WriteID(glob,  class_id);
  Ebml_WriteUTF8(glob,  s);
}

void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length) {
  Ebml_WriteID(glob, class_id);
  Ebml_WriteLen(glob, data_length);
  Ebml_Write(glob,  data, data_length);
}

void Ebml_WriteVoid(EbmlGlobal *glob, unsigned long vSize) {
  unsigned char tmp = 0;
  unsigned long i = 0;

  Ebml_WriteID(glob, 0xEC);
  Ebml_WriteLen(glob, vSize);

  for (i = 0; i < vSize; i++) {
    Ebml_Write(glob, &tmp, 1);
  }
}


