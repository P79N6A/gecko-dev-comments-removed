

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTMODERR_H__
#define __FTMODERR_H__











#undef  FT_NEED_EXTERN_C

#ifndef FT_MODERRDEF

#ifdef FT_CONFIG_OPTION_USE_MODULE_ERRORS
#define FT_MODERRDEF( e, v, s )  FT_Mod_Err_ ## e = v,
#else
#define FT_MODERRDEF( e, v, s )  FT_Mod_Err_ ## e = 0,
#endif

#define FT_MODERR_START_LIST  enum {
#define FT_MODERR_END_LIST    FT_Mod_Err_Max };

#ifdef __cplusplus
#define FT_NEED_EXTERN_C
  extern "C" {
#endif

#endif 


  
  
  
  
  
  
  


#ifdef FT_MODERR_START_LIST
  FT_MODERR_START_LIST
#endif


  FT_MODERRDEF( Base,      0x000, "base module" )
  FT_MODERRDEF( Autofit,   0x100, "autofitter module" )
  FT_MODERRDEF( BDF,       0x200, "BDF module" )
  FT_MODERRDEF( Cache,     0x300, "cache module" )
  FT_MODERRDEF( CFF,       0x400, "CFF module" )
  FT_MODERRDEF( CID,       0x500, "CID module" )
  FT_MODERRDEF( Gzip,      0x600, "Gzip module" )
  FT_MODERRDEF( LZW,       0x700, "LZW module" )
  FT_MODERRDEF( OTvalid,   0x800, "OpenType validation module" )
  FT_MODERRDEF( PCF,       0x900, "PCF module" )
  FT_MODERRDEF( PFR,       0xA00, "PFR module" )
  FT_MODERRDEF( PSaux,     0xB00, "PS auxiliary module" )
  FT_MODERRDEF( PShinter,  0xC00, "PS hinter module" )
  FT_MODERRDEF( PSnames,   0xD00, "PS names module" )
  FT_MODERRDEF( Raster,    0xE00, "raster module" )
  FT_MODERRDEF( SFNT,      0xF00, "SFNT module" )
  FT_MODERRDEF( Smooth,   0x1000, "smooth raster module" )
  FT_MODERRDEF( TrueType, 0x1100, "TrueType module" )
  FT_MODERRDEF( Type1,    0x1200, "Type 1 module" )
  FT_MODERRDEF( Type42,   0x1300, "Type 42 module" )
  FT_MODERRDEF( Winfonts, 0x1400, "Windows FON/FNT module" )


#ifdef FT_MODERR_END_LIST
  FT_MODERR_END_LIST
#endif


  
  
  
  
  
  
  


#ifdef FT_NEED_EXTERN_C
  }
#endif

#undef FT_MODERR_START_LIST
#undef FT_MODERR_END_LIST
#undef FT_MODERRDEF
#undef FT_NEED_EXTERN_C


#endif 



