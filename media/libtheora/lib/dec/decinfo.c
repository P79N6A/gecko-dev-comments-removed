
















#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "decint.h"








static void oc_unpack_octets(oggpack_buffer *_opb,char *_buf,size_t _len){
  while(_len-->0){
    long val;
    theorapackB_read(_opb,8,&val);
    *_buf++=(char)val;
  }
}


static long oc_unpack_length(oggpack_buffer *_opb){
  long ret[4];
  int  i;
  for(i=0;i<4;i++)theorapackB_read(_opb,8,ret+i);
  return ret[0]|ret[1]<<8|ret[2]<<16|ret[3]<<24;
}

static int oc_info_unpack(oggpack_buffer *_opb,th_info *_info){
  long val;
  
  theorapackB_read(_opb,8,&val);
  _info->version_major=(unsigned char)val;
  theorapackB_read(_opb,8,&val);
  _info->version_minor=(unsigned char)val;
  theorapackB_read(_opb,8,&val);
  _info->version_subminor=(unsigned char)val;
  

  if(_info->version_major>TH_VERSION_MAJOR||
   _info->version_major==TH_VERSION_MAJOR&&
   _info->version_minor>TH_VERSION_MINOR){
    return TH_EVERSION;
  }
  
  theorapackB_read(_opb,16,&val);
  _info->frame_width=(ogg_uint32_t)val<<4;
  theorapackB_read(_opb,16,&val);
  _info->frame_height=(ogg_uint32_t)val<<4;
  theorapackB_read(_opb,24,&val);
  _info->pic_width=(ogg_uint32_t)val;
  theorapackB_read(_opb,24,&val);
  _info->pic_height=(ogg_uint32_t)val;
  theorapackB_read(_opb,8,&val);
  _info->pic_x=(ogg_uint32_t)val;
  theorapackB_read(_opb,8,&val);
  _info->pic_y=(ogg_uint32_t)val;
  theorapackB_read(_opb,32,&val);
  _info->fps_numerator=(ogg_uint32_t)val;
  theorapackB_read(_opb,32,&val);
  _info->fps_denominator=(ogg_uint32_t)val;
  if(_info->frame_width==0||_info->frame_height==0||
   _info->pic_width+_info->pic_x>_info->frame_width||
   _info->pic_height+_info->pic_y>_info->frame_height||
   _info->fps_numerator==0||_info->fps_denominator==0){
    return TH_EBADHEADER;
  }
  



  _info->pic_y=_info->frame_height-_info->pic_height-_info->pic_y;
  theorapackB_read(_opb,24,&val);
  _info->aspect_numerator=(ogg_uint32_t)val;
  theorapackB_read(_opb,24,&val);
  _info->aspect_denominator=(ogg_uint32_t)val;
  theorapackB_read(_opb,8,&val);
  _info->colorspace=(th_colorspace)val;
  theorapackB_read(_opb,24,&val);
  _info->target_bitrate=(int)val;
  theorapackB_read(_opb,6,&val);
  _info->quality=(int)val;
  theorapackB_read(_opb,5,&val);
  _info->keyframe_granule_shift=(int)val;
  theorapackB_read(_opb,2,&val);
  _info->pixel_fmt=(th_pixel_fmt)val;
  if(_info->pixel_fmt==TH_PF_RSVD)return TH_EBADHEADER;
  if(theorapackB_read(_opb,3,&val)<0||val!=0)return TH_EBADHEADER;
  return 0;
}

static int oc_comment_unpack(oggpack_buffer *_opb,th_comment *_tc){
  long len;
  int  i;
  
  len=oc_unpack_length(_opb);
  if(len<0||len>_opb->storage-theorapackB_bytes(_opb))return TH_EBADHEADER;
  _tc->vendor=_ogg_malloc((size_t)len+1);
  oc_unpack_octets(_opb,_tc->vendor,len);
  _tc->vendor[len]='\0';
  
  _tc->comments=(int)oc_unpack_length(_opb);
  len=_tc->comments;
  if(len<0||len>(LONG_MAX>>2)||len<<2>_opb->storage-theorapackB_bytes(_opb)){
    _tc->comments=0;
    return TH_EBADHEADER;
  }
  _tc->comment_lengths=(int *)_ogg_malloc(
   _tc->comments*sizeof(_tc->comment_lengths[0]));
  _tc->user_comments=(char **)_ogg_malloc(
   _tc->comments*sizeof(_tc->user_comments[0]));
  for(i=0;i<_tc->comments;i++){
    len=oc_unpack_length(_opb);
    if(len<0||len>_opb->storage-theorapackB_bytes(_opb)){
      _tc->comments=i;
      return TH_EBADHEADER;
    }
    _tc->comment_lengths[i]=len;
    _tc->user_comments[i]=_ogg_malloc((size_t)len+1);
    oc_unpack_octets(_opb,_tc->user_comments[i],len);
    _tc->user_comments[i][len]='\0';
  }
  return theorapackB_read(_opb,0,&len)<0?TH_EBADHEADER:0;
}

static int oc_setup_unpack(oggpack_buffer *_opb,th_setup_info *_setup){
  int ret;
  
  ret=oc_quant_params_unpack(_opb,&_setup->qinfo);
  if(ret<0)return ret;
  
  return oc_huff_trees_unpack(_opb,_setup->huff_tables);
}

static void oc_setup_clear(th_setup_info *_setup){
  oc_quant_params_clear(&_setup->qinfo);
  oc_huff_trees_clear(_setup->huff_tables);
}

static int oc_dec_headerin(oggpack_buffer *_opb,th_info *_info,
 th_comment *_tc,th_setup_info **_setup,ogg_packet *_op){
  char buffer[6];
  long val;
  int  packtype;
  int  ret;
  theorapackB_read(_opb,8,&val);
  packtype=(int)val;
  

  if(!(packtype&0x80)&&_info->frame_width>0&&_tc->vendor!=NULL&&*_setup!=NULL){
    return 0;
  }
  
  oc_unpack_octets(_opb,buffer,6);
  if(memcmp(buffer,"theora",6)!=0)return TH_ENOTFORMAT;
  switch(packtype){
    
    case 0x80:{
      

      if(!_op->b_o_s||_info->frame_width>0)return TH_EBADHEADER;
      ret=oc_info_unpack(_opb,_info);
      if(ret<0)th_info_clear(_info);
      else ret=3;
    }break;
    
    case 0x81:{
      if(_tc==NULL)return TH_EFAULT;
      

      if(_info->frame_width==0||_tc->vendor!=NULL)return TH_EBADHEADER;
      ret=oc_comment_unpack(_opb,_tc);
      if(ret<0)th_comment_clear(_tc);
      else ret=2;
    }break;
    
    case 0x82:{
      oc_setup_info *setup;
      if(_tc==NULL||_setup==NULL)return TH_EFAULT;
      

      if(_info->frame_width==0||_tc->vendor==NULL||*_setup!=NULL){
        return TH_EBADHEADER;
      }
      setup=(oc_setup_info *)_ogg_calloc(1,sizeof(*setup));
      ret=oc_setup_unpack(_opb,setup);
      if(ret<0){
        oc_setup_clear(setup);
        _ogg_free(setup);
      }
      else{
        *_setup=setup;
        ret=1;
      }
    }break;
    default:{
      
      return TH_EBADHEADER;
    }break;
  }
  return ret;
}





int th_decode_headerin(th_info *_info,th_comment *_tc,
 th_setup_info **_setup,ogg_packet *_op){
  oggpack_buffer opb;
  if(_op==NULL)return TH_EBADHEADER;
  if(_info==NULL)return TH_EFAULT;
  theorapackB_readinit(&opb,_op->packet,_op->bytes);
  return oc_dec_headerin(&opb,_info,_tc,_setup,_op);
}

void th_setup_free(th_setup_info *_setup){
  if(_setup!=NULL){
    oc_setup_clear(_setup);
    _ogg_free(_setup);
  }
}
