


























#ifndef __PCFTYPES_H__
#define __PCFTYPES_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  typedef struct  PCF_Public_FaceRec_
  {
    FT_FaceRec    root;
    FT_StreamRec  gzip_stream;
    FT_Stream     gzip_source;

    char*         charset_encoding;
    char*         charset_registry;

  } PCF_Public_FaceRec, *PCF_Public_Face;


FT_END_HEADER

#endif  



