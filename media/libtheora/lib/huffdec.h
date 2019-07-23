
















#if !defined(_huffdec_H)
# define _huffdec_H (1)
# include "huffman.h"
# include "bitpack.h"



typedef struct oc_huff_node oc_huff_node;


































struct oc_huff_node{
  



  unsigned char  nbits;
  

  unsigned char  token;
  




  unsigned char  depth;
  



  oc_huff_node  *nodes[2];
};



int oc_huff_trees_unpack(oc_pack_buf *_opb,
 oc_huff_node *_nodes[TH_NHUFFMAN_TABLES]);
int oc_huff_trees_copy(oc_huff_node *_dst[TH_NHUFFMAN_TABLES],
 const oc_huff_node *const _src[TH_NHUFFMAN_TABLES]);
void oc_huff_trees_clear(oc_huff_node *_nodes[TH_NHUFFMAN_TABLES]);
int oc_huff_token_decode(oc_pack_buf *_opb,const oc_huff_node *_node);


#endif
