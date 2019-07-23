

















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


#endif 

 

FT_END_HEADER

#endif 



