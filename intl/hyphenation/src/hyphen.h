

















































#ifndef __HYPHEN_H__
#define __HYPHEN_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HyphenDict HyphenDict;
typedef struct _HyphenState HyphenState;
typedef struct _HyphenTrans HyphenTrans;
#define MAX_CHARS 100
#define MAX_NAME 20

struct _HyphenDict {
  
  char lhmin;    
  char rhmin;    
  char clhmin;   
  char crhmin;   
  char * nohyphen; 

  int nohyphenl; 
  
  int num_states;
  char cset[MAX_NAME];
  int utf8;
  HyphenState *states;
  HyphenDict *nextlevel;
};

struct _HyphenState {
  char *match;
  char *repl;
  signed char replindex;
  signed char replcut;
  int fallback_state;
  int num_trans;
  HyphenTrans *trans;
};

struct _HyphenTrans {
  char ch;
  int new_state;
};

HyphenDict *hnj_hyphen_load (const char *fn);
void hnj_hyphen_free (HyphenDict *dict);


int hnj_hyphen_hyphenate (HyphenDict *dict,
			   const char *word, int word_size,
			   char *hyphens);

















































int hnj_hyphen_hyphenate2 (HyphenDict *dict,
        const char *word, int word_size, char * hyphens,
        char *hyphenated_word, char *** rep, int ** pos, int ** cut);








int hnj_hyphen_hyphenate3 (HyphenDict *dict,
	const char *word, int word_size, char * hyphens,
	char *hyphword, char *** rep, int ** pos, int ** cut,
	int lhmin, int rhmin, int clhmin, int crhmin);

#ifdef __cplusplus
}
#endif 

#endif
