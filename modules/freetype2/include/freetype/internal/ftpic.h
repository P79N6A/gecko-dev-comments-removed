
















  
  
  
  
  
  


#ifndef __FTPIC_H__
#define __FTPIC_H__

  
FT_BEGIN_HEADER

#ifdef FT_CONFIG_OPTION_PIC

  typedef struct FT_PIC_Container_
  {
    
    void* base;
    
    void* autofit;   
    void* cff;    
    void* pshinter;    
    void* psnames;    
    void* raster;     
    void* sfnt;     
    void* smooth;     
    void* truetype;     
  } FT_PIC_Container;

  
  FT_BASE( FT_Error )
  ft_pic_container_init( FT_Library library );


  
  FT_BASE( void )
  ft_pic_container_destroy( FT_Library library );

#endif 

 

FT_END_HEADER

#endif 



