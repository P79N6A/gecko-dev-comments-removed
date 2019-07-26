





































#ifndef __CF2READ_H__
#define __CF2READ_H__


FT_BEGIN_HEADER


  typedef struct  CF2_BufferRec_
  {
    FT_Error*       error;
    const FT_Byte*  start;
    const FT_Byte*  end;
    const FT_Byte*  ptr;

  } CF2_BufferRec, *CF2_Buffer;


  FT_LOCAL( CF2_Int )
  cf2_buf_readByte( CF2_Buffer  buf );
  FT_LOCAL( FT_Bool )
  cf2_buf_isEnd( CF2_Buffer  buf );


FT_END_HEADER


#endif 



