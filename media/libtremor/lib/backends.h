




















#ifndef _vorbis_backend_h_
#define _vorbis_backend_h_

#include "codec_internal.h"








typedef struct{
  vorbis_info_floor     *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_floor     *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				   vorbis_info_floor *);
  void (*free_info) (vorbis_info_floor *);
  void (*free_look) (vorbis_look_floor *);
  void *(*inverse1)  (struct vorbis_block *,vorbis_look_floor *);
  int   (*inverse2)  (struct vorbis_block *,vorbis_look_floor *,
		     void *buffer,ogg_int32_t *);
} vorbis_func_floor;

typedef struct{
  int   order;
  long  rate;
  long  barkmap;

  int   ampbits;
  int   ampdB;

  int   numbooks; 
  int   books[16];

} vorbis_info_floor0;

#define VIF_POSIT 63
#define VIF_CLASS 16
#define VIF_PARTS 31
typedef struct{
  int   partitions;                
  int   partitionclass[VIF_PARTS]; 

  int   class_dim[VIF_CLASS];        
  int   class_subs[VIF_CLASS];       
  int   class_book[VIF_CLASS];       
  int   class_subbook[VIF_CLASS][8]; 


  int   mult;                       
  int   postlist[VIF_POSIT+2];     

} vorbis_info_floor1;


typedef struct{
  vorbis_info_residue *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_residue *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				 vorbis_info_residue *);
  void (*free_info)    (vorbis_info_residue *);
  void (*free_look)    (vorbis_look_residue *);
  int  (*inverse)      (struct vorbis_block *,vorbis_look_residue *,
			ogg_int32_t **,int *,int);
} vorbis_func_residue;

typedef struct vorbis_info_residue0{

  long  begin;
  long  end;

  
  int    grouping;         
  int    partitions;       
  int    partvals;         
  int    groupbook;        
  int    secondstages[64]; 
  int    booklist[512];    
} vorbis_info_residue0;


typedef struct{
  vorbis_info_mapping *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_mapping *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				 vorbis_info_mapping *);
  void (*free_info)    (vorbis_info_mapping *);
  void (*free_look)    (vorbis_look_mapping *);
  int  (*inverse)      (struct vorbis_block *vb,vorbis_look_mapping *);
} vorbis_func_mapping;

typedef struct vorbis_info_mapping0{
  int   submaps;  
  int   chmuxlist[256];   
  
  int   floorsubmap[16];   
  int   residuesubmap[16]; 

  int   psy[2]; 


  int   coupling_steps;
  int   coupling_mag[256];
  int   coupling_ang[256];
} vorbis_info_mapping0;

#endif





