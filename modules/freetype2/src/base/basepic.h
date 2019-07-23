

















#ifndef __BASEPIC_H__
#define __BASEPIC_H__

  
FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC
#define FT_OUTLINE_GLYPH_CLASS_GET &ft_outline_glyph_class
#define FT_BITMAP_GLYPH_CLASS_GET  &ft_bitmap_glyph_class
#define FT_DEFAULT_MODULES_GET     ft_default_modules

#else 

#include FT_GLYPH_H

  typedef struct BasePIC_
  {
    FT_Module_Class** default_module_classes;
    FT_Glyph_Class ft_outline_glyph_class;
    FT_Glyph_Class ft_bitmap_glyph_class;
  } BasePIC;

#define GET_PIC(lib)                  ((BasePIC*)((lib)->pic_container.base))
#define FT_OUTLINE_GLYPH_CLASS_GET    (&GET_PIC(library)->ft_outline_glyph_class)
#define FT_BITMAP_GLYPH_CLASS_GET     (&GET_PIC(library)->ft_bitmap_glyph_class)
#define FT_DEFAULT_MODULES_GET        (GET_PIC(library)->default_module_classes)

  void
  ft_base_pic_free( FT_Library library );

  FT_Error
  ft_base_pic_init( FT_Library library );

#endif 
 

FT_END_HEADER

#endif 



