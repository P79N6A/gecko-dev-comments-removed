












#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/utypes.h"
#include "unicode/ustdio.h"



int c_main(UFILE *out)
{
  UChar32 ch;
  UErrorCode errorCode = U_ZERO_ERROR;
  static const UChar upper[] = {0x61, 0x42, 0x49, 0}; 
  static const UChar lower[] = {0x61, 0x42, 0x69, 0}; 
  
  static const UChar unfold[] = {0x61, 0x42, 0x131, 0} ;
  UChar buffer[32];
  const UChar char_k = 0x006b;   
  const UChar char_K = 0x004b; 
  
  int length;

  printf("** C Case Mapping Sample\n");

  
  ch = u_toupper(char_k); 
  u_fprintf(out, "toupper(%C) = %C\n", char_k, ch);
  ch = u_tolower(ch); 
  u_fprintf(out, "tolower() = %C\n", ch);
  ch = u_totitle(char_k); 
  u_fprintf(out, "totitle(%C) = %C\n", char_k, ch);
  ch = u_foldCase(char_K, U_FOLD_CASE_DEFAULT); 
  u_fprintf(out, "u_foldCase(%C, U_FOLD_CASE_DEFAULT) = %C\n", char_K, (UChar) ch);

  
  

  length = u_strToLower(buffer, sizeof(buffer)/sizeof(buffer[0]), upper, 
                        sizeof(upper)/sizeof(upper[0]), "tr", &errorCode);
  if(U_FAILURE(errorCode) || buffer[length]!=0) {
    u_fprintf(out, "error in u_strToLower(Turkish locale)=%ld error=%s\n", length, 
           u_errorName(errorCode));
  }
  
  u_fprintf(out, "u_strToLower(%S, turkish) -> %S\n", upper, buffer);


  
  

  length = u_strToUpper(buffer, sizeof(buffer)/sizeof(buffer[0]), upper, 
                        sizeof(upper)/sizeof(upper[0]), "en", &errorCode);
  if(U_FAILURE(errorCode) || buffer[length]!=0) {
    u_fprintf(out, "error in u_strToLower(English locale)=%ld error=%s\n", length, 
              u_errorName(errorCode));
  }
  u_fprintf(out, "u_strToUpper(%S, english) -> %S\n", lower, buffer);


  
  
  length = u_strFoldCase(buffer, sizeof(buffer)/sizeof(buffer[0]), unfold, 
                         sizeof(unfold)/sizeof(unfold[0]), U_FOLD_CASE_DEFAULT,
                         &errorCode);
  if(U_FAILURE(errorCode) || buffer[length]!=0) {
    u_fprintf(out, "error in u_strFoldCase()=%ld error=%s\n", length, 
           u_errorName(errorCode));
  }
  u_fprintf(out, "u_strFoldCase(%S, U_FOLD_CASE_DEFAULT) -> %S\n", unfold, buffer);
  u_fprintf(out, "\n** end of C sample\n");

  return 0;
}
