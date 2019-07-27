




































#ifndef _S3_DICT_H_
#define _S3_DICT_H_






#include <sphinxbase/hash_table.h>


#include "s3types.h"
#include "bin_mdef.h"
#include "pocketsphinx_export.h"

#define S3DICT_INC_SZ 4096

#ifdef __cplusplus
extern "C" {
#endif





typedef struct {
    char *word;		
    s3cipid_t *ciphone;	
    int32 pronlen;	
    s3wid_t alt;	
    s3wid_t basewid;	
} dictword_t;






typedef struct {
    int refcnt;
    bin_mdef_t *mdef;	
    dictword_t *word;	
    hash_table_t *ht;	
    int32 max_words;	
    int32 n_word;	
    int32 filler_start;	
    int32 filler_end;	
    s3wid_t startwid;	
    s3wid_t finishwid;	
    s3wid_t silwid;	
    int nocase;
} dict_t;













dict_t *dict_init(cmd_ln_t *config, 
                  bin_mdef_t *mdef  
    );




int dict_write(dict_t *dict, char const *filename, char const *format);


POCKETSPHINX_EXPORT
s3wid_t dict_wordid(dict_t *d, const char *word);





int dict_filler_word(dict_t *d,  
                     s3wid_t w     
    );




POCKETSPHINX_EXPORT
int dict_real_word(dict_t *d,  
                   s3wid_t w     
    );





s3wid_t dict_add_word(dict_t *d,          
                      char const *word,   
                      s3cipid_t const *p, 
                      int32 np            
    );




const char *dict_ciphone_str(dict_t *d,	
                             s3wid_t wid,	
                             int32 pos   	
    );


#define dict_size(d)		((d)->n_word)
#define dict_num_fillers(d)   (dict_filler_end(d) - dict_filler_start(d))





#define dict_num_real_words(d)                                          \
    (dict_size(d) - (dict_filler_end(d) - dict_filler_start(d)) - 2)
#define dict_basewid(d,w)	((d)->word[w].basewid)
#define dict_wordstr(d,w)	((w) < 0 ? NULL : (d)->word[w].word)
#define dict_basestr(d,w)	((d)->word[dict_basewid(d,w)].word)
#define dict_nextalt(d,w)	((d)->word[w].alt)
#define dict_pronlen(d,w)	((d)->word[w].pronlen) 
#define dict_pron(d,w,p)	((d)->word[w].ciphone[p]) /**< The CI phones of the word w at position p */
#define dict_filler_start(d)	((d)->filler_start)
#define dict_filler_end(d)	((d)->filler_end)
#define dict_startwid(d)	((d)->startwid)
#define dict_finishwid(d)	((d)->finishwid)
#define dict_silwid(d)		((d)->silwid)
#define dict_is_single_phone(d,w)	((d)->word[w].pronlen == 1)
#define dict_first_phone(d,w)	((d)->word[w].ciphone[0])
#define dict_second_phone(d,w)	((d)->word[w].ciphone[1])
#define dict_second_last_phone(d,w)	((d)->word[w].ciphone[(d)->word[w].pronlen - 2])
#define dict_last_phone(d,w)	((d)->word[w].ciphone[(d)->word[w].pronlen - 1])


#define S3_START_WORD		"<s>"
#define S3_FINISH_WORD		"</s>"
#define S3_SILENCE_WORD		"<sil>"
#define S3_UNKNOWN_WORD		"<UNK>"








int32 dict_word2basestr(char *word);




dict_t *dict_retain(dict_t *d);




int dict_free(dict_t *d);


void dict_report(dict_t *d 
    );

#ifdef __cplusplus
}
#endif

#endif
