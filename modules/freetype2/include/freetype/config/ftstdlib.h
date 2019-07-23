


















  
  
  
  
  
  
  
  
  
  


#ifndef __FTSTDLIB_H__
#define __FTSTDLIB_H__


#include <stddef.h>

#define ft_ptrdiff_t  ptrdiff_t


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <limits.h>

#define FT_CHAR_BIT   CHAR_BIT
#define FT_INT_MAX    INT_MAX
#define FT_UINT_MAX   UINT_MAX
#define FT_ULONG_MAX  ULONG_MAX


  
  
  
  
  


#include <string.h>

#define ft_memchr   memchr
#define ft_memcmp   memcmp
#define ft_memcpy   memcpy
#define ft_memmove  memmove
#define ft_memset   memset
#define ft_strcat   strcat
#define ft_strcmp   strcmp
#define ft_strcpy   strcpy
#define ft_strlen   strlen
#define ft_strncmp  strncmp
#define ft_strncpy  strncpy
#define ft_strrchr  strrchr
#define ft_strstr   strstr


  
  
  
  
  


#include <stdio.h>

#define FT_FILE     FILE
#define ft_fclose   fclose
#define ft_fopen    fopen
#define ft_fread    fread
#define ft_fseek    fseek
#define ft_ftell    ftell
#define ft_sprintf  sprintf


  
  
  
  
  


#include <stdlib.h>

#define ft_qsort  qsort

#define ft_exit   exit    /* only used to exit from unhandled exceptions */


  
  
  
  
  


#define ft_scalloc   calloc
#define ft_sfree     free
#define ft_smalloc   malloc
#define ft_srealloc  realloc


  
  
  
  
  


#define ft_atol   atol
#define ft_labs   labs


  
  
  
  
  


#include <setjmp.h>

#define ft_jmp_buf     jmp_buf  /* note: this cannot be a typedef since */
                                
                                

#define ft_longjmp     longjmp
#define ft_setjmp( b ) setjmp( *(jmp_buf*) &(b) )    /* same thing here */


  
  

#include <stdarg.h>


#endif 



