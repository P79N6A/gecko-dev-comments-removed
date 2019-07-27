








































#ifndef __NGRAM_SEARCH_FWDFLAT_H__
#define __NGRAM_SEARCH_FWDFLAT_H__




#include "ngram_search.h"




void ngram_fwdflat_init(ngram_search_t *ngs);




void ngram_fwdflat_deinit(ngram_search_t *ngs);




int ngram_fwdflat_reinit(ngram_search_t *ngs);




void ngram_fwdflat_start(ngram_search_t *ngs);




int ngram_fwdflat_search(ngram_search_t *ngs, int frame_idx);




void ngram_fwdflat_finish(ngram_search_t *ngs);


#endif 
