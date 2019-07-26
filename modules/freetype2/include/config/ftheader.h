
















#ifndef __FT_HEADER_H__
#define __FT_HEADER_H__













#ifdef __cplusplus
#define FT_BEGIN_HEADER  extern "C" {
#else
#define FT_BEGIN_HEADER
#endif


  
  
  
  
  
  
  
  
  
  
  
#ifdef __cplusplus
#define FT_END_HEADER  }
#else
#define FT_END_HEADER
#endif


  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  

  









#ifndef FT_CONFIG_CONFIG_H
#define FT_CONFIG_CONFIG_H  <config/ftconfig.h>
#endif


  









#ifndef FT_CONFIG_STANDARD_LIBRARY_H
#define FT_CONFIG_STANDARD_LIBRARY_H  <config/ftstdlib.h>
#endif


  









#ifndef FT_CONFIG_OPTIONS_H
#define FT_CONFIG_OPTIONS_H  <config/ftoption.h>
#endif


  










#ifndef FT_CONFIG_MODULES_H
#define FT_CONFIG_MODULES_H  <config/ftmodule.h>
#endif

  

  

  









#define FT_FREETYPE_H  <freetype.h>


  











#define FT_ERRORS_H  <fterrors.h>


  









#define FT_MODULE_ERRORS_H  <ftmoderr.h>


  












#define FT_SYSTEM_H  <ftsystem.h>


  












#define FT_IMAGE_H  <ftimage.h>


  











#define FT_TYPES_H  <fttypes.h>


  











#define FT_LIST_H  <ftlist.h>


  









#define FT_OUTLINE_H  <ftoutln.h>


  









#define FT_SIZES_H  <ftsizes.h>


  









#define FT_MODULE_H  <ftmodapi.h>


  









#define FT_RENDER_H  <ftrender.h>


  









#define FT_AUTOHINTER_H  <ftautoh.h>


  









#define FT_CFF_DRIVER_H  <ftcffdrv.h>


  









#define FT_TRUETYPE_DRIVER_H  <ftttdrv.h>


  









#define FT_TYPE1_TABLES_H  <t1tables.h>


  











#define FT_TRUETYPE_IDS_H  <ttnameid.h>


  









#define FT_TRUETYPE_TABLES_H  <tttables.h>


  










#define FT_TRUETYPE_TAGS_H  <tttags.h>


  










#define FT_BDF_H  <ftbdf.h>


  










#define FT_CID_H  <ftcid.h>


  









#define FT_GZIP_H  <ftgzip.h>


  









#define FT_LZW_H  <ftlzw.h>


  









#define FT_BZIP2_H  <ftbzip2.h>


  









#define FT_WINFONTS_H   <ftwinfnt.h>


  









#define FT_GLYPH_H  <ftglyph.h>


  









#define FT_BITMAP_H  <ftbitmap.h>


  









#define FT_BBOX_H  <ftbbox.h>


  









#define FT_CACHE_H  <ftcache.h>


  
















#define FT_CACHE_IMAGE_H  FT_CACHE_H


  

















#define FT_CACHE_SMALL_BITMAPS_H  FT_CACHE_H


  












#define FT_CACHE_CHARMAP_H  FT_CACHE_H


  













#define FT_MAC_H  <ftmac.h>


  









#define FT_MULTIPLE_MASTERS_H  <ftmm.h>


  










#define FT_SFNT_NAMES_H  <ftsnames.h>


  










#define FT_OPENTYPE_VALIDATE_H  <ftotval.h>


  










#define FT_GX_VALIDATE_H  <ftgxval.h>


  









#define FT_PFR_H  <ftpfr.h>


  








#define FT_STROKER_H  <ftstroke.h>


  








#define FT_SYNTHESIS_H  <ftsynth.h>


  









#define FT_XFREE86_H  <ftxf86.h>


  









#define FT_TRIGONOMETRY_H  <fttrigon.h>


  








#define FT_LCD_FILTER_H  <ftlcdfil.h>


  








#define FT_UNPATENTED_HINTING_H  <ttunpat.h>


  








#define FT_INCREMENTAL_H  <ftincrem.h>


  








#define FT_GASP_H  <ftgasp.h>


  








#define FT_ADVANCES_H  <ftadvanc.h>


  

#define FT_ERROR_DEFINITIONS_H  <fterrdef.h>


  
  
  
  
#define FT_CACHE_MANAGER_H           <ftcache.h>
#define FT_CACHE_INTERNAL_MRU_H      <ftcache.h>
#define FT_CACHE_INTERNAL_MANAGER_H  <ftcache.h>
#define FT_CACHE_INTERNAL_CACHE_H    <ftcache.h>
#define FT_CACHE_INTERNAL_GLYPH_H    <ftcache.h>
#define FT_CACHE_INTERNAL_IMAGE_H    <ftcache.h>
#define FT_CACHE_INTERNAL_SBITS_H    <ftcache.h>


#define FT_INCREMENTAL_H          <ftincrem.h>

#define FT_TRUETYPE_UNPATENTED_H  <ttunpat.h>


  



#ifdef FT2_BUILD_LIBRARY
#define  FT_INTERNAL_INTERNAL_H  <internal/internal.h>
#include FT_INTERNAL_INTERNAL_H
#endif 


#endif 



