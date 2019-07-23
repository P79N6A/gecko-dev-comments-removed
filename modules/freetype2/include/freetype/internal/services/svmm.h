

















#ifndef __SVMM_H__
#define __SVMM_H__

#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER


  






#define FT_SERVICE_ID_MULTI_MASTERS  "multi-masters"


  typedef FT_Error
  (*FT_Get_MM_Func)( FT_Face           face,
                     FT_Multi_Master*  master );

  typedef FT_Error
  (*FT_Get_MM_Var_Func)( FT_Face      face,
                         FT_MM_Var*  *master );

  typedef FT_Error
  (*FT_Set_MM_Design_Func)( FT_Face   face,
                            FT_UInt   num_coords,
                            FT_Long*  coords );

  typedef FT_Error
  (*FT_Set_Var_Design_Func)( FT_Face    face,
                             FT_UInt    num_coords,
                             FT_Fixed*  coords );

  typedef FT_Error
  (*FT_Set_MM_Blend_Func)( FT_Face   face,
                           FT_UInt   num_coords,
                           FT_Long*  coords );


  FT_DEFINE_SERVICE( MultiMasters )
  {
    FT_Get_MM_Func          get_mm;
    FT_Set_MM_Design_Func   set_mm_design;
    FT_Set_MM_Blend_Func    set_mm_blend;
    FT_Get_MM_Var_Func      get_mm_var;
    FT_Set_Var_Design_Func  set_var_design;
  };

  


FT_END_HEADER

#endif 



