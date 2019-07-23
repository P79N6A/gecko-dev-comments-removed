
















#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "huffdec.h"
#include "decint.h"



#define _ogg_offsetof(_type,_field)\
 ((size_t)((char *)&((_type *)0)->_field-(char *)0))


static const unsigned char OC_DCT_TOKEN_MAP_ENTRIES[TH_NDCT_TOKENS]={
  1,1,1,4,8,1,1,8,1,1,1,1,1,2,2,2,2,4,8,2,2,2,4,2,2,2,2,2,8,2,4,8
};









static const unsigned char OC_DCT_TOKEN_MAP[TH_NDCT_TOKENS]={
  
  15,
  
  16,
  
  17,
  
  88,
  
  80,
  
   1,
  
   0,
  
  48,
  
  14,
  
  56,
  
  57,
  
  58,
  
  59,
  
  60,
  62,
  64,
  66,
  
  68,
  
  72,
  
   2,
  
   4,
  
   6,
  
   8,
  
  18,
  20,
  22,
  24,
  26,
  
  32,
  
  12,
  
  28,
  
  40
};






static oc_pb_window oc_pack_refill(oc_pack_buf *_b,int _bits){
  const unsigned char *ptr;
  const unsigned char *stop;
  oc_pb_window         window;
  int                  available;
  window=_b->window;
  available=_b->bits;
  ptr=_b->ptr;
  stop=_b->stop;
  

  if(ptr>=stop)available=OC_LOTS_OF_BITS;
  while(available<=OC_PB_WINDOW_SIZE-8){
    available+=8;
    window|=(oc_pb_window)*ptr++<<OC_PB_WINDOW_SIZE-available;
    if(ptr>=stop)available=OC_LOTS_OF_BITS;
  }
  _b->ptr=ptr;
  if(_bits>available)window|=*ptr>>(available&7);
  _b->bits=available;
  return window;
}




static long oc_pack_look(oc_pack_buf *_b,int _bits){
  oc_pb_window window;
  int          available;
  long         result;
  window=_b->window;
  available=_b->bits;
  if(_bits==0)return 0;
  if(_bits>available)_b->window=window=oc_pack_refill(_b,_bits);
  result=window>>OC_PB_WINDOW_SIZE-_bits;
  return result;
}


static void oc_pack_adv(oc_pack_buf *_b,int _bits){
  




  _b->window<<=_bits;
  _b->bits-=_bits;
}


















#define OC_HUFF_SLUSH (1)








static size_t oc_huff_node_size(int _nbits){
  size_t size;
  size=_ogg_offsetof(oc_huff_node,nodes);
  if(_nbits>0)size+=sizeof(oc_huff_node *)*(1<<_nbits);
  return size;
}

static oc_huff_node *oc_huff_node_init(char **_storage,size_t _size,int _nbits){
  oc_huff_node *ret;
  ret=(oc_huff_node *)*_storage;
  ret->nbits=(unsigned char)_nbits;
  (*_storage)+=_size;
  return ret;
}







static size_t oc_huff_tree_size(const oc_huff_node *_node){
  size_t size;
  size=oc_huff_node_size(_node->nbits);
  if(_node->nbits){
    int nchildren;
    int i;
    nchildren=1<<_node->nbits;
    for(i=0;i<nchildren;i+=1<<_node->nbits-_node->nodes[i]->depth){
      size+=oc_huff_tree_size(_node->nodes[i]);
    }
  }
  return size;
}







static int oc_huff_tree_unpack(oc_pack_buf *_opb,
 oc_huff_node *_binodes,int _nbinodes){
  oc_huff_node *binode;
  long          bits;
  int           nused;
  if(_nbinodes<1)return TH_EBADHEADER;
  binode=_binodes;
  nused=0;
  bits=oc_pack_read1(_opb);
  if(oc_pack_bytes_left(_opb)<0)return TH_EBADHEADER;
  
  if(!bits){
    int ret;
    nused++;
    binode->nbits=1;
    binode->depth=1;
    binode->nodes[0]=_binodes+nused;
    ret=oc_huff_tree_unpack(_opb,_binodes+nused,_nbinodes-nused);
    if(ret>=0){
      nused+=ret;
      binode->nodes[1]=_binodes+nused;
      ret=oc_huff_tree_unpack(_opb,_binodes+nused,_nbinodes-nused);
    }
    if(ret<0)return ret;
    nused+=ret;
  }
  
  else{
    int ntokens;
    int token;
    int i;
    bits=oc_pack_read(_opb,OC_NDCT_TOKEN_BITS);
    if(oc_pack_bytes_left(_opb)<0)return TH_EBADHEADER;
    
    ntokens=OC_DCT_TOKEN_MAP_ENTRIES[bits];
    if(_nbinodes<2*ntokens-1)return TH_EBADHEADER;
    
    for(i=1;i<ntokens;i<<=1){
      int j;
      binode=_binodes+nused;
      nused+=i;
      for(j=0;j<i;j++){
        binode[j].nbits=1;
        binode[j].depth=1;
        binode[j].nodes[0]=_binodes+nused+2*j;
        binode[j].nodes[1]=_binodes+nused+2*j+1;
      }
    }
    
    token=OC_DCT_TOKEN_MAP[bits];
    for(i=0;i<ntokens;i++){
      binode=_binodes+nused++;
      binode->nbits=0;
      binode->depth=1;
      binode->token=token+i;
    }
  }
  return nused;
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




static oc_huff_node *oc_huff_tree_copy(const oc_huff_node *_node,
 char **_storage){
  oc_huff_node *ret;
  ret=oc_huff_node_init(_storage,oc_huff_node_size(_node->nbits),_node->nbits);
  ret->depth=_node->depth;
  if(_node->nbits){
    int nchildren;
    int i;
    int inext;
    nchildren=1<<_node->nbits;
    for(i=0;i<nchildren;){
      ret->nodes[i]=oc_huff_tree_copy(_node->nodes[i],_storage);
      inext=i+(1<<_node->nbits-ret->nodes[i]->depth);
      while(++i<inext)ret->nodes[i]=ret->nodes[i-1];
    }
  }
  else ret->token=_node->token;
  return ret;
}

static size_t oc_huff_tree_collapse_size(oc_huff_node *_binode,int _depth){
  size_t size;
  int    mindepth;
  int    depth;
  int    loccupancy;
  int    occupancy;
  if(_binode->nbits!=0&&_depth>0){
    return oc_huff_tree_collapse_size(_binode->nodes[0],_depth-1)+
     oc_huff_tree_collapse_size(_binode->nodes[1],_depth-1);
  }
  depth=mindepth=oc_huff_tree_mindepth(_binode);
  occupancy=1<<mindepth;
  do{
    loccupancy=occupancy;
    occupancy=oc_huff_tree_occupancy(_binode,++depth);
  }
  while(occupancy>loccupancy&&occupancy>=1<<OC_MAXI(depth-OC_HUFF_SLUSH,0));
  depth--;
  size=oc_huff_node_size(depth);
  if(depth>0){
    size+=oc_huff_tree_collapse_size(_binode->nodes[0],depth-1);
    size+=oc_huff_tree_collapse_size(_binode->nodes[1],depth-1);
  }
  return size;
}

static oc_huff_node *oc_huff_tree_collapse(oc_huff_node *_binode,
 char **_storage);














static void oc_huff_node_fill(oc_huff_node **_nodes,
 oc_huff_node *_binode,int _level,int _depth,char **_storage){
  if(_level<=0||_binode->nbits==0){
    int i;
    _binode->depth=(unsigned char)(_depth-_level);
    _nodes[0]=oc_huff_tree_collapse(_binode,_storage);
    for(i=1;i<1<<_level;i++)_nodes[i]=_nodes[0];
  }
  else{
    _level--;
    oc_huff_node_fill(_nodes,_binode->nodes[0],_level,_depth,_storage);
    _nodes+=1<<_level;
    oc_huff_node_fill(_nodes,_binode->nodes[1],_level,_depth,_storage);
  }
}







static oc_huff_node *oc_huff_tree_collapse(oc_huff_node *_binode,
 char **_storage){
  oc_huff_node *root;
  size_t        size;
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
  if(depth<=1)return oc_huff_tree_copy(_binode,_storage);
  size=oc_huff_node_size(depth);
  root=oc_huff_node_init(_storage,size,depth);
  root->depth=_binode->depth;
  oc_huff_node_fill(root->nodes,_binode,depth,depth,_storage);
  return root;
}






int oc_huff_trees_unpack(oc_pack_buf *_opb,
 oc_huff_node *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    oc_huff_node  nodes[511];
    char         *storage;
    size_t        size;
    int           ret;
    
    ret=oc_huff_tree_unpack(_opb,nodes,sizeof(nodes)/sizeof(*nodes));
    if(ret<0)return ret;
    
    size=oc_huff_tree_collapse_size(nodes,0);
    storage=(char *)_ogg_calloc(1,size);
    if(storage==NULL)return TH_EFAULT;
    
    _nodes[i]=oc_huff_tree_collapse(nodes,&storage);
  }
  return 0;
}




int oc_huff_trees_copy(oc_huff_node *_dst[TH_NHUFFMAN_TABLES],
 const oc_huff_node *const _src[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    size_t  size;
    char   *storage;
    size=oc_huff_tree_size(_src[i]);
    storage=(char *)_ogg_calloc(1,size);
    if(storage==NULL){
      while(i-->0)_ogg_free(_dst[i]);
      return TH_EFAULT;
    }
    _dst[i]=oc_huff_tree_copy(_src[i],&storage);
  }
  return 0;
}



void oc_huff_trees_clear(oc_huff_node *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++)_ogg_free(_nodes[i]);
}





int oc_huff_token_decode(oc_pack_buf *_opb,const oc_huff_node *_node){
  long bits;
  while(_node->nbits!=0){
    bits=oc_pack_look(_opb,_node->nbits);
    _node=_node->nodes[bits];
    oc_pack_adv(_opb,_node->depth);
  }
  return _node->token;
}
