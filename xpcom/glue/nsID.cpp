





#include "nsID.h"
#include "prprf.h"
#include "nsMemory.h"

void nsID::Clear()
{
  m0 = 0;
  m1 = 0;
  m2 = 0;
  for (int i = 0; i < 8; ++i) {
    m3[i] = 0;
  }
}







#define ADD_HEX_CHAR_TO_INT_OR_RETURN_FALSE(the_char, the_int_var) \
    the_int_var = (the_int_var << 4) + the_char; \
    if(the_char >= '0' && the_char <= '9') the_int_var -= '0'; \
    else if(the_char >= 'a' && the_char <= 'f') the_int_var -= 'a'-10; \
    else if(the_char >= 'A' && the_char <= 'F') the_int_var -= 'A'-10; \
    else return false









#define PARSE_CHARS_TO_NUM(char_pointer, dest_variable, number_of_chars) \
  do { int32_t _i=number_of_chars; \
  dest_variable = 0; \
  while(_i) { \
    ADD_HEX_CHAR_TO_INT_OR_RETURN_FALSE(*char_pointer, dest_variable); \
    char_pointer++; \
    _i--; \
  } } while(0)








#define PARSE_HYPHEN(char_pointer) if (*(char_pointer++) != '-') return false






bool
nsID::Parse(const char* aIDStr)
{
  
  if (!aIDStr) {
    return false;
  }

  bool expectFormat1 = (aIDStr[0] == '{');
  if (expectFormat1) {
    ++aIDStr;
  }

  PARSE_CHARS_TO_NUM(aIDStr, m0, 8);
  PARSE_HYPHEN(aIDStr);
  PARSE_CHARS_TO_NUM(aIDStr, m1, 4);
  PARSE_HYPHEN(aIDStr);
  PARSE_CHARS_TO_NUM(aIDStr, m2, 4);
  PARSE_HYPHEN(aIDStr);
  int i;
  for (i = 0; i < 2; ++i) {
    PARSE_CHARS_TO_NUM(aIDStr, m3[i], 2);
  }
  PARSE_HYPHEN(aIDStr);
  while (i < 8) {
    PARSE_CHARS_TO_NUM(aIDStr, m3[i], 2);
    i++;
  }

  return expectFormat1 ? *aIDStr == '}' : true;
}

#ifndef XPCOM_GLUE_AVOID_NSPR

static const char gIDFormat[] =
  "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";







char*
nsID::ToString() const
{
  char* res = (char*)NS_Alloc(NSID_LENGTH);

  if (res) {
    PR_snprintf(res, NSID_LENGTH, gIDFormat,
                m0, (uint32_t)m1, (uint32_t)m2,
                (uint32_t)m3[0], (uint32_t)m3[1], (uint32_t)m3[2],
                (uint32_t)m3[3], (uint32_t)m3[4], (uint32_t)m3[5],
                (uint32_t)m3[6], (uint32_t)m3[7]);
  }
  return res;
}

void
nsID::ToProvidedString(char (&aDest)[NSID_LENGTH]) const
{
  PR_snprintf(aDest, NSID_LENGTH, gIDFormat,
              m0, (uint32_t)m1, (uint32_t)m2,
              (uint32_t)m3[0], (uint32_t)m3[1], (uint32_t)m3[2],
              (uint32_t)m3[3], (uint32_t)m3[4], (uint32_t)m3[5],
              (uint32_t)m3[6], (uint32_t)m3[7]);
}

#endif 
