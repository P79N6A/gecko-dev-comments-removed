


















#include "unicode/putil.h"
#include "rbutil.h"
#include "cmemory.h"
#include "cstring.h"



void
get_dirname(char *dirname,
            const char *filename)
{
  const char *lastSlash = uprv_strrchr(filename, U_FILE_SEP_CHAR) + 1;

  if(lastSlash>filename) {
    uprv_strncpy(dirname, filename, (lastSlash - filename));
    *(dirname + (lastSlash - filename)) = '\0';
  } else {
    *dirname = '\0';
  }
}


void
get_basename(char *basename,
             const char *filename)
{
  
  const char *lastSlash = uprv_strrchr(filename, U_FILE_SEP_CHAR) + 1;
  char *lastDot;

  if(lastSlash>filename) {
    uprv_strcpy(basename, lastSlash);
  } else {
    uprv_strcpy(basename, filename);
  }

  
  lastDot = uprv_strrchr(basename, '.');

  if(lastDot != NULL) {
    *lastDot = '\0';
  }
}

#define MAX_DIGITS 10
int32_t 
itostr(char * buffer, int32_t i, uint32_t radix, int32_t pad)
{
    const char digits[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int32_t length = 0;
    int32_t num = 0;
    int32_t save = i;
    int digit;
    int32_t j;
    char temp;
    
    
    if(i<0){
        i=-i;
    }
    
    do{
        digit = (int)(i % radix);
        buffer[length++]= digits[digit];
        i=i/radix;
    } while(i);

    while (length < pad){
        buffer[length++] = '0';
    }
    
    
    if(save < 0){
        buffer[length++]='-';
    }

    
    if(length<MAX_DIGITS){
        buffer[length] =  0x0000;
    }

    num= (pad>=length) ? pad :length;
 

    
    for (j = 0; j < (num / 2); j++){
        temp = buffer[(length-1) - j];
        buffer[(length-1) - j] = buffer[j];
        buffer[j] = temp;
    }
    return length;
}
