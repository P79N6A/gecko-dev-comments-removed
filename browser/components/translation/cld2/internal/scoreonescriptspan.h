







































































#ifndef I18N_ENCODINGS_CLD2_INTERNAL_SCOREONESCRIPTSPAN_H__
#define I18N_ENCODINGS_CLD2_INTERNAL_SCOREONESCRIPTSPAN_H__

#include <stdio.h>

#include "integral_types.h"           

#include "cld2tablesummary.h"
#include "compact_lang_det_impl.h"    
#include "getonescriptspan.h"
#include "langspan.h"
#include "tote.h"
#include "utf8statetable.h"

namespace CLD2 {

static const int kMaxBoosts = 4;              
                                              
static const int kChunksizeQuads = 20;        
static const int kChunksizeUnis = 50;         
static const int kMaxScoringHits = 1000;
static const int kMaxSummaries = kMaxScoringHits / kChunksizeQuads;





typedef struct {
  const UTF8PropObj* unigram_obj;               
  const CLD2TableSummary* unigram_compat_obj;   
  const CLD2TableSummary* deltabi_obj;
  const CLD2TableSummary* distinctbi_obj;

  const CLD2TableSummary* quadgram_obj;         
  const CLD2TableSummary* quadgram_obj2;        
  const CLD2TableSummary* deltaocta_obj;
  const CLD2TableSummary* distinctocta_obj;

  const short* kExpectedScore;      
                                    
                                    
} ScoringTables;


typedef struct {
   int32 n;
   uint32 langprob[kMaxBoosts];
   int wrap(int32 n) {return n & (kMaxBoosts - 1);}
} LangBoosts;

typedef struct {
   LangBoosts latn;
   LangBoosts othr;
} PerScriptLangBoosts;





typedef struct {
  FILE* debug_file;                   
  bool flags_cld2_score_as_quads;
  bool flags_cld2_html;
  bool flags_cld2_cr;
  bool flags_cld2_verbose;
  ULScript ulscript;        
  Language prior_chunk_lang;          
  
  
  
  
  PerScriptLangBoosts langprior_boost;  
  PerScriptLangBoosts langprior_whack;  
  PerScriptLangBoosts distinct_boost;   
  int oldest_distinct_boost;          
                                      
  const ScoringTables* scoringtables; 
  ScriptScanner* scanner;             

  
  void init() {
    memset(&langprior_boost, 0, sizeof(langprior_boost));
    memset(&langprior_whack, 0, sizeof(langprior_whack));
    memset(&distinct_boost, 0, sizeof(distinct_boost));
  };
} ScoringContext;







typedef struct {
  int offset;         
  int indirect;       
} ScoringHit;

typedef enum {
  UNIHIT                       = 0,
  QUADHIT                      = 1,
  DELTAHIT                     = 2,
  DISTINCTHIT                  = 3
} LinearHitType;


typedef struct {
  uint16 offset;      
  uint16 type;        
  uint32 langprob;    
} LangprobHit;


typedef struct {
  ULScript ulscript;        
  int maxscoringhits;       
  int next_base;            
  int next_delta;           
  int next_distinct;        
  int next_linear;          
  int next_chunk_start;     
  int lowest_offset;        
  
  ScoringHit base[kMaxScoringHits + 1];         
  ScoringHit delta[kMaxScoringHits + 1];        
  ScoringHit distinct[kMaxScoringHits + 1];     
  LangprobHit linear[4 * kMaxScoringHits + 1];  
                                                
  int chunk_start[kMaxSummaries + 1];           
                                                
  int chunk_offset[kMaxSummaries + 1];          
                                                

  void init() {
    ulscript = ULScript_Common;
    maxscoringhits = kMaxScoringHits;
    next_base = 0;
    next_delta = 0;
    next_distinct = 0;
    next_linear = 0;
    next_chunk_start = 0;
    lowest_offset = 0;
    base[0].offset = 0;
    base[0].indirect = 0;
    delta[0].offset = 0;
    delta[0].indirect = 0;
    distinct[0].offset = 0;
    distinct[0].indirect = 0;
    linear[0].offset = 0;
    linear[0].langprob = 0;
    chunk_start[0] = 0;
    chunk_offset[0] = 0;
  };
} ScoringHitBuffer;


typedef struct {
  int chunk_base;       
  int chunk_delta;      
  int chunk_distinct;   
  int base_len;         
  int delta_len;        
  int distinct_len;     
} ChunkSpan;



typedef struct {
  uint16 offset;              
  uint16 chunk_start;         
  uint16 lang1;               
  uint16 lang2;               
  uint16 score1;              
  uint16 score2;              
  uint16 bytes;               
  uint16 grams;               
  uint16 ulscript;            
  uint8 reliability_delta;    
  uint8 reliability_score;    
} ChunkSummary;








typedef struct {
  int n;
  ChunkSummary chunksummary[kMaxSummaries + 1];
} SummaryBuffer;






void ScoreEntireScriptSpan(const LangSpan& scriptspan,
                           ScoringContext* scoringcontext,
                           DocTote* doc_tote,
                           ResultChunkVector* vec);


void ScoreCJKScriptSpan(const LangSpan& scriptspan,
                        ScoringContext* scoringcontext,
                        DocTote* doc_tote,
                        ResultChunkVector* vec);


void ScoreQuadScriptSpan(const LangSpan& scriptspan,
                         ScoringContext* scoringcontext,
                         DocTote* doc_tote,
                         ResultChunkVector* vec);


void ScoreOneScriptSpan(const LangSpan& scriptspan,
                        ScoringContext* scoringcontext,
                        DocTote* doc_tote,
                        ResultChunkVector* vec);

}       

#endif  

