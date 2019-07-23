




































#include "nsID.h"
#include "prprf.h"
#include "prmem.h"

static const char gIDFormat[] = 
  "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";

static const char gIDFormat2[] = 
  "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";








#define ADD_HEX_CHAR_TO_INT_OR_RETURN_FALSE(the_char, the_int_var) \
    the_int_var = (the_int_var << 4) + the_char; \
    if(the_char >= '0' && the_char <= '9') the_int_var -= '0'; \
    else if(the_char >= 'a' && the_char <= 'f') the_int_var -= 'a'-10; \
    else if(the_char >= 'A' && the_char <= 'F') the_int_var -= 'A'-10; \
    else return PR_FALSE









#define PARSE_CHARS_TO_NUM(char_pointer, dest_variable, number_of_chars) \
  do { PRInt32 _i=number_of_chars; \
  dest_variable = 0; \
  while(_i) { \
    ADD_HEX_CHAR_TO_INT_OR_RETURN_FALSE(*char_pointer, dest_variable); \
    char_pointer++; \
    _i--; \
  } } while(0)








 #define PARSE_HYPHEN(char_pointer)   if(*(char_pointer++) != '-') return PR_FALSE
    





PRBool nsID::Parse(const char *aIDStr)
{
  
  if(!aIDStr) {
    return PR_FALSE;
  }

  PRBool expectFormat1 = (aIDStr[0] == '{');
  if(expectFormat1) aIDStr++;

  PARSE_CHARS_TO_NUM(aIDStr, m0, 8);
  PARSE_HYPHEN(aIDStr);
  PARSE_CHARS_TO_NUM(aIDStr, m1, 4);
  PARSE_HYPHEN(aIDStr);
  PARSE_CHARS_TO_NUM(aIDStr, m2, 4);
  PARSE_HYPHEN(aIDStr);
  int i;
  for(i=0; i<2; i++)
    PARSE_CHARS_TO_NUM(aIDStr, m3[i], 2);
  PARSE_HYPHEN(aIDStr);
  while(i < 8) {
    PARSE_CHARS_TO_NUM(aIDStr, m3[i], 2);
    i++;
  }
  
  return expectFormat1 ? *aIDStr == '}' : PR_TRUE;
}







char *nsID::ToString() const 
{
  char *res = (char*)PR_Malloc(39);    

  if (res != NULL) {
    PR_snprintf(res, 39, gIDFormat,
                m0, (PRUint32) m1, (PRUint32) m2,
                (PRUint32) m3[0], (PRUint32) m3[1], (PRUint32) m3[2],
                (PRUint32) m3[3], (PRUint32) m3[4], (PRUint32) m3[5],
                (PRUint32) m3[6], (PRUint32) m3[7]);
  }
  return res;
}

