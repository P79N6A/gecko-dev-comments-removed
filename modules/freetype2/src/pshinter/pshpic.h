

















#ifndef __PSHPIC_H__
#define __PSHPIC_H__


FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC

#define FTPSHINTER_INTERFACE_GET        pshinter_interface

#else 

#include FT_INTERNAL_POSTSCRIPT_HINTS_H

  typedef struct PSHinterPIC_
  {
    PSHinter_Interface pshinter_interface;
  } PSHinterPIC;

#define GET_PIC(lib)                    ((PSHinterPIC*)((lib)->pic_container.autofit))
#define FTPSHINTER_INTERFACE_GET        (GET_PIC(library)->pshinter_interface)

  
  void
  pshinter_module_class_pic_free( FT_Library  library );

  FT_Error
  pshinter_module_class_pic_init( FT_Library  library );

#endif 

 

FT_END_HEADER

#endif 



