

















#ifndef __TTINTERP_H__
#define __TTINTERP_H__

#include <ft2build.h>
#include "ttobjs.h"


FT_BEGIN_HEADER


#ifndef TT_CONFIG_OPTION_STATIC_INTERPRETER 

#define EXEC_OP_   TT_ExecContext  exc,
#define EXEC_OP    TT_ExecContext  exc
#define EXEC_ARG_  exc,
#define EXEC_ARG   exc

#else                                       

#define EXEC_OP_
#define EXEC_OP
#define EXEC_ARG_
#define EXEC_ARG

#endif 


  
  
  
  
#define TT_Round_Off             5
#define TT_Round_To_Half_Grid    0
#define TT_Round_To_Grid         1
#define TT_Round_To_Double_Grid  2
#define TT_Round_Up_To_Grid      4
#define TT_Round_Down_To_Grid    3
#define TT_Round_Super           6
#define TT_Round_Super_45        7


  
  
  
  
  
  
  

  
  typedef FT_F26Dot6
  (*TT_Round_Func)( EXEC_OP_ FT_F26Dot6  distance,
                             FT_F26Dot6  compensation );

  
  typedef void
  (*TT_Move_Func)( EXEC_OP_ TT_GlyphZone  zone,
                            FT_UShort     point,
                            FT_F26Dot6    distance );

  
  typedef FT_F26Dot6
  (*TT_Project_Func)( EXEC_OP_ FT_Pos   dx,
                               FT_Pos   dy );

  
  typedef FT_F26Dot6
  (*TT_Get_CVT_Func)( EXEC_OP_ FT_ULong  idx );

  
  
  typedef void
  (*TT_Set_CVT_Func)( EXEC_OP_ FT_ULong    idx,
                               FT_F26Dot6  value );


  
  
  
  
  typedef struct  TT_CallRec_
  {
    FT_Int   Caller_Range;
    FT_Long  Caller_IP;
    FT_Long  Cur_Count;
    FT_Long  Cur_Restart;
    FT_Long  Cur_End;

  } TT_CallRec, *TT_CallStack;


#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING

  
  
  
  
  

#define SPH_MAX_NAME_SIZE      32
#define SPH_MAX_CLASS_MEMBERS  100

  typedef struct  SPH_TweakRule_
  {
    const char      family[SPH_MAX_NAME_SIZE];
    const FT_UInt   ppem;
    const char      style[SPH_MAX_NAME_SIZE];
    const FT_ULong  glyph;

  } SPH_TweakRule;


  typedef struct  SPH_ScaleRule_
  {
    const char      family[SPH_MAX_NAME_SIZE];
    const FT_UInt   ppem;
    const char      style[SPH_MAX_NAME_SIZE];
    const FT_ULong  glyph;
    const FT_ULong  scale;

  } SPH_ScaleRule;


  typedef struct  SPH_Font_Class_
  {
    const char  name[SPH_MAX_NAME_SIZE];
    const char  member[SPH_MAX_CLASS_MEMBERS][SPH_MAX_NAME_SIZE];

  } SPH_Font_Class;

#endif 


  
  
  
  
  
  typedef struct  TT_ExecContextRec_
  {
    TT_Face            face;
    TT_Size            size;
    FT_Memory          memory;

    

    FT_Error           error;      

    FT_Long            top;        

    FT_UInt            stackSize;  
    FT_Long*           stack;      

    FT_Long            args;
    FT_UInt            new_top;    

    TT_GlyphZoneRec    zp0,        
                       zp1,
                       zp2,
                       pts,
                       twilight;

    FT_Size_Metrics    metrics;
    TT_Size_Metrics    tt_metrics; 

    TT_GraphicsState   GS;         

    FT_Int             curRange;  
    FT_Byte*           code;      
    FT_Long            IP;        
    FT_Long            codeSize;  

    FT_Byte            opcode;    
    FT_Int             length;    

    FT_Bool            step_ins;  
                                  
    FT_ULong           cvtSize;
    FT_Long*           cvt;

    FT_UInt            glyphSize; 
    FT_Byte*           glyphIns;  

    FT_UInt            numFDefs;  
    FT_UInt            maxFDefs;  
    TT_DefArray        FDefs;     

    FT_UInt            numIDefs;  
    FT_UInt            maxIDefs;  
    TT_DefArray        IDefs;     

    FT_UInt            maxFunc;   
    FT_UInt            maxIns;    

    FT_Int             callTop,    
                       callSize;   
    TT_CallStack       callStack;  

    FT_UShort          maxPoints;    
    FT_Short           maxContours;  
                                     

    TT_CodeRangeTable  codeRangeTable;  
                                        

    FT_UShort          storeSize;  
    FT_Long*           storage;    

    FT_F26Dot6         period;     
    FT_F26Dot6         phase;      
    FT_F26Dot6         threshold;

#if 0
    
    FT_Int             cur_ppem;   
#endif

    FT_Bool            instruction_trap; 
                                         

    TT_GraphicsState   default_GS;       
                                         
    FT_Bool            is_composite;     
    FT_Bool            pedantic_hinting; 

    

    FT_Long            F_dot_P;    
                                   
    TT_Round_Func      func_round; 

    TT_Project_Func    func_project,   
                       func_dualproj,  
                       func_freeProj;  

    TT_Move_Func       func_move;      
    TT_Move_Func       func_move_orig; 

    TT_Get_CVT_Func    func_read_cvt;  
    TT_Set_CVT_Func    func_write_cvt; 
    TT_Set_CVT_Func    func_move_cvt;  

    FT_Bool            grayscale;      

#ifdef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    TT_Round_Func      func_round_sphn;   

    FT_Bool            grayscale_hinting; 
    FT_Bool            subpixel_hinting;  
    FT_Bool            native_hinting;    
    FT_Bool            ignore_x_mode;     
                                          
                                          

    
    
    FT_Bool            compatible_widths;     
    FT_Bool            symmetrical_smoothing; 
    FT_Bool            bgr;                   
    FT_Bool            subpixel_positioned;   
                                              

    FT_Int             rasterizer_version;    

    FT_Bool            iup_called;            

    FT_ULong           sph_tweak_flags;       
                                              

    FT_ULong           sph_in_func_flags;     
                                              

#endif 

  } TT_ExecContextRec;


  extern const TT_GraphicsState  tt_default_graphics_state;


#ifdef TT_USE_BYTECODE_INTERPRETER
  FT_LOCAL( FT_Error )
  TT_Goto_CodeRange( TT_ExecContext  exec,
                     FT_Int          range,
                     FT_Long         IP );

  FT_LOCAL( FT_Error )
  TT_Set_CodeRange( TT_ExecContext  exec,
                    FT_Int          range,
                    void*           base,
                    FT_Long         length );

  FT_LOCAL( FT_Error )
  TT_Clear_CodeRange( TT_ExecContext  exec,
                      FT_Int          range );


  FT_LOCAL( FT_Error )
  Update_Max( FT_Memory  memory,
              FT_ULong*  size,
              FT_Long    multiplier,
              void*      _pbuff,
              FT_ULong   new_max );
#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( TT_ExecContext )
  TT_New_Context( TT_Driver  driver );


#ifdef TT_USE_BYTECODE_INTERPRETER
  FT_LOCAL( FT_Error )
  TT_Done_Context( TT_ExecContext  exec );

  FT_LOCAL( FT_Error )
  TT_Load_Context( TT_ExecContext  exec,
                   TT_Face         face,
                   TT_Size         size );

  FT_LOCAL( FT_Error )
  TT_Save_Context( TT_ExecContext  exec,
                   TT_Size         ins );

  FT_LOCAL( FT_Error )
  TT_Run_Context( TT_ExecContext  exec,
                  FT_Bool         debug );
#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_EXPORT( FT_Error )
  TT_RunIns( TT_ExecContext  exec );


FT_END_HEADER

#endif 



