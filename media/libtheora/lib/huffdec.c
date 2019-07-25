
















#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "huffdec.h"
#include "decint.h"

































































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





static const unsigned char OC_DCT_TOKEN_MAP_LOG_NENTRIES[TH_NDCT_TOKENS]={
  0,0,0,2,3,0,0,3,0,0,0,0,0,1,1,1,1,2,3,1,1,1,2,1,1,1,1,1,3,1,2,3
};


















#define OC_HUFF_SLUSH (2)






#define OC_ROOT_HUFF_SLUSH (7)












int oc_huff_tree_unpack(oc_pack_buf *_opb,unsigned char _tokens[256][2]){
  ogg_uint32_t code;
  int          len;
  int          ntokens;
  int          nleaves;
  code=0;
  len=ntokens=nleaves=0;
  for(;;){
    long bits;
    bits=oc_pack_read1(_opb);
    
    if(oc_pack_bytes_left(_opb)<0)return TH_EBADHEADER;
    
    if(!bits){
      len++;
      
      if(len>32)return TH_EBADHEADER;
    }
    
    else{
      ogg_uint32_t code_bit;
      int          neb;
      int          nentries;
      int          token;
      
      if(++nleaves>32)return TH_EBADHEADER;
      bits=oc_pack_read(_opb,OC_NDCT_TOKEN_BITS);
      neb=OC_DCT_TOKEN_MAP_LOG_NENTRIES[bits];
      token=OC_DCT_TOKEN_MAP[bits];
      nentries=1<<neb;
      while(nentries-->0){
        _tokens[ntokens][0]=(unsigned char)token++;
        _tokens[ntokens][1]=(unsigned char)(len+neb);
        ntokens++;
      }
      code_bit=0x80000000U>>len-1;
      while(len>0&&(code&code_bit)){
        code^=code_bit;
        code_bit<<=1;
        len--;
      }
      if(len<=0)break;
      code|=code_bit;
    }
  }
  return ntokens;
}






static int oc_huff_subtree_tokens(unsigned char _tokens[][2],int _depth){
  ogg_uint32_t code;
  int          ti;
  code=0;
  ti=0;
  do{
    if(_tokens[ti][1]-_depth<32)code+=0x80000000U>>_tokens[ti++][1]-_depth;
    else{
      


      code++;
      ti+=oc_huff_subtree_tokens(_tokens+ti,_depth+31);
    }
  }
  while(code<0x80000000U);
  return ti;
}









static int oc_huff_tree_collapse_depth(unsigned char _tokens[][2],
 int _ntokens,int _depth){
  int got_leaves;
  int loccupancy;
  int occupancy;
  int slush;
  int nbits;
  int best_nbits;
  slush=_depth>0?OC_HUFF_SLUSH:OC_ROOT_HUFF_SLUSH;
  






  nbits=1;
  occupancy=2;
  got_leaves=1;
  do{
    int ti;
    if(got_leaves)best_nbits=nbits;
    nbits++;
    got_leaves=0;
    loccupancy=occupancy;
    for(occupancy=ti=0;ti<_ntokens;occupancy++){
      if(_tokens[ti][1]<_depth+nbits)ti++;
      else if(_tokens[ti][1]==_depth+nbits){
        got_leaves=1;
        ti++;
      }
      else ti+=oc_huff_subtree_tokens(_tokens+ti,_depth+nbits);
    }
  }
  while(occupancy>loccupancy&&occupancy*slush>=1<<nbits);
  return best_nbits;
}






static size_t oc_huff_node_size(int _nbits){
  return 1+(1<<_nbits);
}









static size_t oc_huff_tree_collapse(ogg_int16_t *_tree,
 unsigned char _tokens[][2],int _ntokens){
  ogg_int16_t   node[34];
  unsigned char depth[34];
  unsigned char last[34];
  size_t        ntree;
  int           ti;
  int           l;
  depth[0]=0;
  last[0]=(unsigned char)(_ntokens-1);
  ntree=0;
  ti=0;
  l=0;
  do{
    int nbits;
    nbits=oc_huff_tree_collapse_depth(_tokens+ti,last[l]+1-ti,depth[l]);
    node[l]=(ogg_int16_t)ntree;
    ntree+=oc_huff_node_size(nbits);
    if(_tree!=NULL)_tree[node[l]++]=(ogg_int16_t)nbits;
    do{
      while(ti<=last[l]&&_tokens[ti][1]<=depth[l]+nbits){
        if(_tree!=NULL){
          ogg_int16_t leaf;
          int         nentries;
          nentries=1<<depth[l]+nbits-_tokens[ti][1];
          leaf=(ogg_int16_t)-(_tokens[ti][1]-depth[l]<<8|_tokens[ti][0]);
          while(nentries-->0)_tree[node[l]++]=leaf;
        }
        ti++;
      }
      if(ti<=last[l]){
        
        depth[l+1]=(unsigned char)(depth[l]+nbits);
        if(_tree!=NULL)_tree[node[l]++]=(ogg_int16_t)ntree;
        l++;
        last[l]=
         (unsigned char)(ti+oc_huff_subtree_tokens(_tokens+ti,depth[l])-1);
        break;
      }
      
      else if(l-->0)nbits=depth[l+1]-depth[l];
    }
    while(l>=0);
  }
  while(l>=0);
  return ntree;
}








int oc_huff_trees_unpack(oc_pack_buf *_opb,
 ogg_int16_t *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    unsigned char  tokens[256][2];
    int            ntokens;
    ogg_int16_t   *tree;
    size_t         size;
    
    ntokens=oc_huff_tree_unpack(_opb,tokens);
    if(ntokens<0)return ntokens;
    
    size=oc_huff_tree_collapse(NULL,tokens,ntokens);
    

    if(size>32767)return TH_EIMPL;
    tree=(ogg_int16_t *)_ogg_malloc(size*sizeof(*tree));
    if(tree==NULL)return TH_EFAULT;
    
    oc_huff_tree_collapse(tree,tokens,ntokens);
    _nodes[i]=tree;
  }
  return 0;
}





static size_t oc_huff_tree_size(const ogg_int16_t *_tree,int _node){
  size_t size;
  int    nchildren;
  int    n;
  int    i;
  n=_tree[_node];
  size=oc_huff_node_size(n);
  nchildren=1<<n;
  i=0;
  do{
    int child;
    child=_tree[_node+i+1];
    if(child<=0)i+=1<<n-(-child>>8);
    else{
      size+=oc_huff_tree_size(_tree,child);
      i++;
    }
  }
  while(i<nchildren);
  return size;
}




int oc_huff_trees_copy(ogg_int16_t *_dst[TH_NHUFFMAN_TABLES],
 const ogg_int16_t *const _src[TH_NHUFFMAN_TABLES]){
  int total;
  int i;
  total=0;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    size_t size;
    size=oc_huff_tree_size(_src[i],0);
    total+=size;
    _dst[i]=(ogg_int16_t *)_ogg_malloc(size*sizeof(*_dst[i]));
    if(_dst[i]==NULL){
      while(i-->0)_ogg_free(_dst[i]);
      return TH_EFAULT;
    }
    memcpy(_dst[i],_src[i],size*sizeof(*_dst[i]));
  }
  return 0;
}



void oc_huff_trees_clear(ogg_int16_t *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++)_ogg_free(_nodes[i]);
}






int oc_huff_token_decode_c(oc_pack_buf *_opb,const ogg_int16_t *_tree){
  const unsigned char *ptr;
  const unsigned char *stop;
  oc_pb_window         window;
  int                  available;
  long                 bits;
  int                  node;
  int                  n;
  ptr=_opb->ptr;
  window=_opb->window;
  stop=_opb->stop;
  available=_opb->bits;
  node=0;
  for(;;){
    n=_tree[node];
    if(n>available){
      unsigned shift;
      shift=OC_PB_WINDOW_SIZE-available;
      do{
        

        if(ptr>=stop){
          shift=(unsigned)-OC_LOTS_OF_BITS;
          break;
        }
        shift-=8;
        window|=(oc_pb_window)*ptr++<<shift;
      }
      while(shift>=8);
      

      available=OC_PB_WINDOW_SIZE-shift;
    }
    bits=window>>OC_PB_WINDOW_SIZE-n;
    node=_tree[node+1+bits];
    if(node<=0)break;
    window<<=n;
    available-=n;
  }
  node=-node;
  n=node>>8;
  window<<=n;
  available-=n;
  _opb->ptr=ptr;
  _opb->window=window;
  _opb->bits=available;
  return node&255;
}
