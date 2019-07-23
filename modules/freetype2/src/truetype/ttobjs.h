

















#ifndef __TTOBJS_H__
#define __TTOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  
  
  
  
  
  
  
  
  typedef struct TT_DriverRec_*  TT_Driver;


  
  
  
  
  
  
  
  
  typedef struct TT_SizeRec_*  TT_Size;


  
  
  
  
  
  
  
  
  
  
  
  
  typedef FT_GlyphSlot  TT_GlyphSlot;


  
  
  
  
  
  
  
  
  typedef struct  TT_GraphicsState_
  {
    FT_UShort      rp0;
    FT_UShort      rp1;
    FT_UShort      rp2;

    FT_UnitVector  dualVector;
    FT_UnitVector  projVector;
    FT_UnitVector  freeVector;

#ifdef TT_CONFIG_OPTION_UNPATENTED_HINTING
    FT_Bool        both_x_axis;
#endif

    FT_Long        loop;
    FT_F26Dot6     minimum_distance;
    FT_Int         round_state;

    FT_Bool        auto_flip;
    FT_F26Dot6     control_value_cutin;
    FT_F26Dot6     single_width_cutin;
    FT_F26Dot6     single_width_value;
    FT_Short       delta_base;
    FT_Short       delta_shift;

    FT_Byte        instruct_control;
    FT_Bool        scan_control;
    FT_Int         scan_type;

    FT_UShort      gep0;
    FT_UShort      gep1;
    FT_UShort      gep2;

  } TT_GraphicsState;


#ifdef TT_USE_BYTECODE_INTERPRETER

  FT_LOCAL( void )
  tt_glyphzone_done( TT_GlyphZone  zone );

  FT_LOCAL( FT_Error )
  tt_glyphzone_new( FT_Memory     memory,
                    FT_UShort     maxPoints,
                    FT_Short      maxContours,
                    TT_GlyphZone  zone );

#endif 



  
  
  
  
  
  
  


#define TT_MAX_CODE_RANGES  3


  
  
  
  
  
  
  
  typedef enum  TT_CodeRange_Tag_
  {
    tt_coderange_none = 0,
    tt_coderange_font,
    tt_coderange_cvt,
    tt_coderange_glyph

  } TT_CodeRange_Tag;


  typedef struct  TT_CodeRange_
  {
    FT_Byte*  base;
    FT_ULong  size;

  } TT_CodeRange;

  typedef TT_CodeRange  TT_CodeRangeTable[TT_MAX_CODE_RANGES];


  
  
  
  
  typedef struct  TT_DefRecord_
  {
    FT_Int   range;      
    FT_Long  start;      
    FT_UInt  opc;        
    FT_Bool  active;     

  } TT_DefRecord, *TT_DefArray;


  
  
  
  
  typedef struct  TT_Transform_
  {
    FT_Fixed    xx, xy;     
    FT_Fixed    yx, yy;
    FT_F26Dot6  ox, oy;     

  } TT_Transform;


  
  
  
  
  typedef struct  TT_SubglyphRec_
  {
    FT_Long          index;        
    FT_Bool          is_scaled;    
    FT_Bool          is_hinted;    
    FT_Bool          preserve_pps; 

    FT_Long          file_offset;

    FT_BBox          bbox;
    FT_Pos           left_bearing;
    FT_Pos           advance;

    TT_GlyphZoneRec  zone;

    FT_Long          arg1;         
    FT_Long          arg2;         

    FT_UShort        element_flag; 

    TT_Transform     transform;    

    FT_Vector        pp1, pp2;     
    FT_Vector        pp3, pp4;     

  } TT_SubGlyphRec, *TT_SubGlyph_Stack;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  typedef struct  TT_Size_Metrics_
  {
    
    FT_Long     x_ratio;
    FT_Long     y_ratio;

    FT_UShort   ppem;               
    FT_Long     ratio;              
    FT_Fixed    scale;

    FT_F26Dot6  compensations[4];   

    FT_Bool     valid;

    FT_Bool     rotated;            
    FT_Bool     stretched;          

  } TT_Size_Metrics;


  
  
  
  
  typedef struct  TT_SizeRec_
  {
    FT_SizeRec         root;

    
    
    FT_Size_Metrics    metrics;

    TT_Size_Metrics    ttmetrics;

    FT_ULong           strike_index;      

#ifdef TT_USE_BYTECODE_INTERPRETER

    FT_UInt            num_function_defs; 
    FT_UInt            max_function_defs;
    TT_DefArray        function_defs;     

    FT_UInt            num_instruction_defs;  
    FT_UInt            max_instruction_defs;
    TT_DefArray        instruction_defs;      

    FT_UInt            max_func;
    FT_UInt            max_ins;

    TT_CodeRangeTable  codeRangeTable;

    TT_GraphicsState   GS;

    FT_ULong           cvt_size;      
    FT_Long*           cvt;

    FT_UShort          storage_size; 
    FT_Long*           storage;      

    TT_GlyphZoneRec    twilight;     

    

    
    
    

    FT_Bool            debug;
    TT_ExecContext     context;

    FT_Bool            bytecode_ready;
    FT_Bool            cvt_ready;

#endif 

  } TT_SizeRec;


  
  
  
  
  typedef struct  TT_DriverRec_
  {
    FT_DriverRec     root;
    TT_ExecContext   context;  
    TT_GlyphZoneRec  zone;     

    void*            extension_component;

  } TT_DriverRec;


  
  
  
  
  
  
  


  
  
  
  
  FT_LOCAL( FT_Error )
  tt_face_init( FT_Stream      stream,
                FT_Face        ttface,      
                FT_Int         face_index,
                FT_Int         num_params,
                FT_Parameter*  params );

  FT_LOCAL( void )
  tt_face_done( FT_Face  ttface );          


  
  
  
  
  FT_LOCAL( FT_Error )
  tt_size_init( FT_Size  ttsize );          

  FT_LOCAL( void )
  tt_size_done( FT_Size  ttsize );          

#ifdef TT_USE_BYTECODE_INTERPRETER

  FT_LOCAL( FT_Error )
  tt_size_run_fpgm( TT_Size  size );

  FT_LOCAL( FT_Error )
  tt_size_run_prep( TT_Size  size );

  FT_LOCAL( FT_Error )
  tt_size_ready_bytecode( TT_Size  size );

#endif 

  FT_LOCAL( FT_Error )
  tt_size_reset( TT_Size  size );


  
  
  
  
  FT_LOCAL( FT_Error )
  tt_driver_init( FT_Module  ttdriver );    

  FT_LOCAL( void )
  tt_driver_done( FT_Module  ttdriver );    


  
  
  
  
  FT_LOCAL( FT_Error )
  tt_slot_init( FT_GlyphSlot  slot );


FT_END_HEADER

#endif 



