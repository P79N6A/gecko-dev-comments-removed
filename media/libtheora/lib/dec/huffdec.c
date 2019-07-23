
















#include <stdlib.h>
#include <ogg/ogg.h>
#include "huffdec.h"
#include "decint.h"



#define _ogg_offsetof(_type,_field)\
 ((size_t)((char *)&((_type *)0)->_field-(char *)0))







static int theorapackB_look(oggpack_buffer *_b,int _bits,long *_ret){
  long ret;
  long m;
  long d;
  m=32-_bits;
  _bits+=_b->endbit;
  d=_b->storage-_b->endbyte;
  if(d<=4){
    
    if(d<=0){
      *_ret=0L;
      return -(_bits>d*8);
    }
    
    if(d*8<_bits)_bits=d*8;
  }
  ret=_b->ptr[0]<<24+_b->endbit;
  if(_bits>8){
    ret|=_b->ptr[1]<<16+_b->endbit;
    if(_bits>16){
      ret|=_b->ptr[2]<<8+_b->endbit;
      if(_bits>24){
        ret|=_b->ptr[3]<<_b->endbit;
        if(_bits>32)ret|=_b->ptr[4]>>8-_b->endbit;
      }
    }
  }
  *_ret=((ret&0xFFFFFFFF)>>(m>>1))>>(m+1>>1);
  return 0;
}


static void theorapackB_adv(oggpack_buffer *_b,int _bits){
  _bits+=_b->endbit;
  _b->ptr+=_bits>>3;
  _b->endbyte+=_bits>>3;
  _b->endbit=_bits&7;
}


















#define OC_HUFF_SLUSH (1)







static oc_huff_node *oc_huff_node_alloc(int _nbits){
  oc_huff_node *ret;
  size_t        size;
  size=_ogg_offsetof(oc_huff_node,nodes);
  if(_nbits>0)size+=sizeof(oc_huff_node *)*(1<<_nbits);
  ret=_ogg_calloc(1,size);
  ret->nbits=(unsigned char)_nbits;
  return ret;
}




static void oc_huff_node_free(oc_huff_node *_node){
  _ogg_free(_node);
}




static void oc_huff_tree_free(oc_huff_node *_node){
  if(_node==NULL)return;
  if(_node->nbits){
    int nchildren;
    int i;
    int inext;
    nchildren=1<<_node->nbits;
    for(i=0;i<nchildren;i=inext){
      inext=i+(_node->nodes[i]!=NULL?1<<_node->nbits-_node->nodes[i]->depth:1);
      oc_huff_tree_free(_node->nodes[i]);
    }
  }
  oc_huff_node_free(_node);
}







static int oc_huff_tree_unpack(oggpack_buffer *_opb,
 oc_huff_node **_binode,int _depth){
  oc_huff_node *binode;
  long          bits;
  
  if(++_depth>32)return TH_EBADHEADER;
  if(theorapackB_read1(_opb,&bits)<0)return TH_EBADHEADER;
  
  if(!bits){
    int ret;
    binode=oc_huff_node_alloc(1);
    binode->depth=(unsigned char)(_depth>1);
    ret=oc_huff_tree_unpack(_opb,binode->nodes,_depth);
    if(ret>=0)ret=oc_huff_tree_unpack(_opb,binode->nodes+1,_depth);
    if(ret<0){
      oc_huff_tree_free(binode);
      *_binode=NULL;
      return ret;
    }
  }
  
  else{
    if(theorapackB_read(_opb,OC_NDCT_TOKEN_BITS,&bits)<0)return TH_EBADHEADER;
    binode=oc_huff_node_alloc(0);
    binode->depth=(unsigned char)(_depth>1);
    binode->token=(unsigned char)bits;
  }
  *_binode=binode;
  return 0;
}







static int oc_huff_tree_mindepth(oc_huff_node *_binode){
  int depth0;
  int depth1;
  if(_binode->nbits==0)return 0;
  depth0=oc_huff_tree_mindepth(_binode->nodes[0]);
  depth1=oc_huff_tree_mindepth(_binode->nodes[1]);
  return OC_MINI(depth0,depth1)+1;
}








static int oc_huff_tree_occupancy(oc_huff_node *_binode,int _depth){
  if(_binode->nbits==0||_depth<=0)return 1;
  else{
    return oc_huff_tree_occupancy(_binode->nodes[0],_depth-1)+
     oc_huff_tree_occupancy(_binode->nodes[1],_depth-1);
  }
}

static oc_huff_node *oc_huff_tree_collapse(oc_huff_node *_binode);














static void oc_huff_node_fill(oc_huff_node **_nodes,
 oc_huff_node *_binode,int _level,int _depth){
  if(_level<=0||_binode->nbits==0){
    int i;
    _binode->depth=(unsigned char)(_depth-_level);
    _nodes[0]=oc_huff_tree_collapse(_binode);
    for(i=1;i<1<<_level;i++)_nodes[i]=_nodes[0];
  }
  else{
    _level--;
    oc_huff_node_fill(_nodes,_binode->nodes[0],_level,_depth);
    oc_huff_node_fill(_nodes+(1<<_level),_binode->nodes[1],_level,_depth);
    oc_huff_node_free(_binode);
  }
}







static oc_huff_node *oc_huff_tree_collapse(oc_huff_node *_binode){
  oc_huff_node *root;
  int           mindepth;
  int           depth;
  int           loccupancy;
  int           occupancy;
  depth=mindepth=oc_huff_tree_mindepth(_binode);
  occupancy=1<<mindepth;
  do{
    loccupancy=occupancy;
    occupancy=oc_huff_tree_occupancy(_binode,++depth);
  }
  while(occupancy>loccupancy&&occupancy>=1<<OC_MAXI(depth-OC_HUFF_SLUSH,0));
  depth--;
  if(depth<=1)return _binode;
  root=oc_huff_node_alloc(depth);
  root->depth=_binode->depth;
  oc_huff_node_fill(root->nodes,_binode,depth,depth);
  return root;
}




static oc_huff_node *oc_huff_tree_copy(const oc_huff_node *_node){
  oc_huff_node *ret;
  ret=oc_huff_node_alloc(_node->nbits);
  ret->depth=_node->depth;
  if(_node->nbits){
    int nchildren;
    int i;
    int inext;
    nchildren=1<<_node->nbits;
    for(i=0;i<nchildren;){
      ret->nodes[i]=oc_huff_tree_copy(_node->nodes[i]);
      inext=i+(1<<_node->nbits-ret->nodes[i]->depth);
      while(++i<inext)ret->nodes[i]=ret->nodes[i-1];
    }
  }
  else ret->token=_node->token;
  return ret;
}






int oc_huff_trees_unpack(oggpack_buffer *_opb,
 oc_huff_node *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    int ret;
    ret=oc_huff_tree_unpack(_opb,_nodes+i,0);
    if(ret<0)return ret;
    _nodes[i]=oc_huff_tree_collapse(_nodes[i]);
  }
  return 0;
}




void oc_huff_trees_copy(oc_huff_node *_dst[TH_NHUFFMAN_TABLES],
 const oc_huff_node *const _src[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++)_dst[i]=oc_huff_tree_copy(_src[i]);
}



void oc_huff_trees_clear(oc_huff_node *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++)oc_huff_tree_free(_nodes[i]);
}





int oc_huff_token_decode(oggpack_buffer *_opb,const oc_huff_node *_node){
  long bits;
  while(_node->nbits!=0){
    theorapackB_look(_opb,_node->nbits,&bits);
    _node=_node->nodes[bits];
    theorapackB_adv(_opb,_node->depth);
  }
  return _node->token;
}
