

















#ifndef __FTTYPES_H__
#define __FTTYPES_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_SYSTEM_H
#include FT_IMAGE_H

#include <stddef.h>


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  typedef unsigned char  FT_Bool;


  
  
  
  
  
  
  
  
  
  typedef signed short  FT_FWord;   


  
  
  
  
  
  
  
  
  
  typedef unsigned short  FT_UFWord;  


  
  
  
  
  
  
  
  
  typedef signed char  FT_Char;


  
  
  
  
  
  
  
  
  typedef unsigned char  FT_Byte;


  
  
  
  
  
  
  
  
  typedef const FT_Byte*  FT_Bytes;


  
  
  
  
  
  
  
  
  typedef FT_UInt32  FT_Tag;


  
  
  
  
  
  
  
  
  typedef char  FT_String;


  
  
  
  
  
  
  
  
  typedef signed short  FT_Short;


  
  
  
  
  
  
  
  
  typedef unsigned short  FT_UShort;


  
  
  
  
  
  
  
  
  typedef signed int  FT_Int;


  
  
  
  
  
  
  
  
  typedef unsigned int  FT_UInt;


  
  
  
  
  
  
  
  
  typedef signed long  FT_Long;


  
  
  
  
  
  
  
  
  typedef unsigned long  FT_ULong;


  
  
  
  
  
  
  
  
  typedef signed short  FT_F2Dot14;


  
  
  
  
  
  
  
  
  
  typedef signed long  FT_F26Dot6;


  
  
  
  
  
  
  
  
  
  typedef signed long  FT_Fixed;


  
  
  
  
  
  
  
  
  
  typedef int  FT_Error;


  
  
  
  
  
  
  
  
  typedef void*  FT_Pointer;


  
  
  
  
  
  
  
  
  
  
  typedef size_t  FT_Offset;


  
  
  
  
  
  
  
  
  
  
  typedef ft_ptrdiff_t  FT_PtrDist;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_UnitVector_
  {
    FT_F2Dot14  x;
    FT_F2Dot14  y;

  } FT_UnitVector;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_Matrix_
  {
    FT_Fixed  xx, xy;
    FT_Fixed  yx, yy;

  } FT_Matrix;


  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_Data_
  {
    const FT_Byte*  pointer;
    FT_Int          length;

  } FT_Data;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef void  (*FT_Generic_Finalizer)(void*  object);


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_Generic_
  {
    void*                 data;
    FT_Generic_Finalizer  finalizer;

  } FT_Generic;


  
  
  
  
  
  
  
  
  
  
  
  
  
#define FT_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (FT_ULong)_x1 << 24 ) |     \
            ( (FT_ULong)_x2 << 16 ) |     \
            ( (FT_ULong)_x3 <<  8 ) |     \
              (FT_ULong)_x4         )


  
  
  
  
  
  
  


  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  typedef struct FT_ListNodeRec_*  FT_ListNode;


  
  
  
  
  
  
  
  
  typedef struct FT_ListRec_*  FT_List;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_ListNodeRec_
  {
    FT_ListNode  prev;
    FT_ListNode  next;
    void*        data;

  } FT_ListNodeRec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  FT_ListRec_
  {
    FT_ListNode  head;
    FT_ListNode  tail;

  } FT_ListRec;


  

#define FT_IS_EMPTY( list )  ( (list).head == 0 )

  
#define FT_ERROR_BASE( x )    ( (x) & 0xFF )

  
#define FT_ERROR_MODULE( x )  ( (x) & 0xFF00U )

#define FT_BOOL( x )  ( (FT_Bool)( x ) )

FT_END_HEADER

#endif 



