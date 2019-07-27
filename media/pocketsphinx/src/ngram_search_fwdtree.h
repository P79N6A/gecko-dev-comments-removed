








































#ifndef __NGRAM_SEARCH_FWDTREE_H__
#define __NGRAM_SEARCH_FWDTREE_H__




#include "ngram_search.h"




void ngram_fwdtree_init(ngram_search_t *ngs);




void ngram_fwdtree_deinit(ngram_search_t *ngs);




int ngram_fwdtree_reinit(ngram_search_t *ngs);




void ngram_fwdtree_start(ngram_search_t *ngs);






int ngram_fwdtree_search(ngram_search_t *ngs, int frame_idx);




void ngram_fwdtree_finish(ngram_search_t *ngs);


#endif 
