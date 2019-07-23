

















#ifndef __SVWINFNT_H__
#define __SVWINFNT_H__

#include FT_INTERNAL_SERVICE_H
#include FT_WINFONTS_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_WINFNT  "winfonts"

  typedef FT_Error
  (*FT_WinFnt_GetHeaderFunc)( FT_Face               face,
                              FT_WinFNT_HeaderRec  *aheader );


  FT_DEFINE_SERVICE( WinFnt )
  {
    FT_WinFnt_GetHeaderFunc  get_header;
  };

  


FT_END_HEADER


#endif 



