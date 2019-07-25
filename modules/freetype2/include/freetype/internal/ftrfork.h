






















#ifndef __FTRFORK_H__
#define __FTRFORK_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  
  
#define FT_RACCESS_N_RULES  9


  
  
  

  typedef struct  FT_RFork_Ref_
  {
    FT_UShort  res_id;
    FT_ULong   offset;

  } FT_RFork_Ref;

#ifdef FT_CONFIG_OPTION_GUESSING_EMBEDDED_RFORK
  typedef FT_Error
  (*ft_raccess_guess_func)( FT_Library  library,
                            FT_Stream   stream,
                            char       *base_file_name,
                            char      **result_file_name,
                            FT_Long    *result_offset );

  typedef enum  FT_RFork_Rule_ {
    FT_RFork_Rule_invalid = -2,
    FT_RFork_Rule_uknown, 
    FT_RFork_Rule_apple_double,
    FT_RFork_Rule_apple_single,
    FT_RFork_Rule_darwin_ufs_export,
    FT_RFork_Rule_darwin_newvfs,
    FT_RFork_Rule_darwin_hfsplus,
    FT_RFork_Rule_vfat,
    FT_RFork_Rule_linux_cap,
    FT_RFork_Rule_linux_double,
    FT_RFork_Rule_linux_netatalk
  } FT_RFork_Rule;

  



  typedef struct ft_raccess_guess_rec_ {
    ft_raccess_guess_func  func;
    FT_RFork_Rule          type;
  } ft_raccess_guess_rec;

#ifndef FT_CONFIG_OPTION_PIC
  
#define CONST_FT_RFORK_RULE_ARRAY_BEGIN( name, type ) \
        const type name[] = {
#define CONST_FT_RFORK_RULE_ARRAY_ENTRY( func_suffix, type_suffix ) \
        { raccess_guess_##func_suffix, FT_RFork_Rule_##type_suffix },
#define CONST_FT_RFORK_RULE_ARRAY_END };
#else 
  
#define CONST_FT_RFORK_RULE_ARRAY_BEGIN( name, type ) \
        void FT_Init_##name ( type* storage ) {       \
          type *local = storage;                      \
          int i = 0;
#define CONST_FT_RFORK_RULE_ARRAY_ENTRY( func_suffix, type_suffix ) \
        local[i].func = raccess_guess_##func_suffix;                \
        local[i].type = FT_RFork_Rule_##type_suffix;                \
        i++;
#define CONST_FT_RFORK_RULE_ARRAY_END }
#endif 
#endif 

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( void )
  FT_Raccess_Guess( FT_Library  library,
                    FT_Stream   stream,
                    char*       base_name,
                    char**      new_names,
                    FT_Long*    offsets,
                    FT_Error*   errors );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Error )
  FT_Raccess_Get_HeaderInfo( FT_Library  library,
                             FT_Stream   stream,
                             FT_Long     rfork_offset,
                             FT_Long    *map_offset,
                             FT_Long    *rdata_pos );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Error )
  FT_Raccess_Get_DataOffsets( FT_Library  library,
                              FT_Stream   stream,
                              FT_Long     map_offset,
                              FT_Long     rdata_pos,
                              FT_Long     tag,
                              FT_Long   **offsets,
                              FT_Long    *count );


FT_END_HEADER

#endif 



