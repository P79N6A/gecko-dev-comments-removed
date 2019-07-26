



















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_NEW_CLDUTIL_H__
#define I18N_ENCODINGS_CLD2_INTERNAL_NEW_CLDUTIL_H__

#include "cldutil_shared.h"
#include "scoreonescriptspan.h"
#include "tote.h"

namespace CLD2 {




int GetUniHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer);



void GetBiHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer);




int GetQuadHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer);



void GetOctaHits(const char* text,
                     int letter_offset, int letter_limit,
                     ScoringContext* scoringcontext,
                     ScoringHitBuffer* hitbuffer);


int ReliabilityDelta(int value1, int value2, int gramcount);
int ReliabilityExpected(int actual_score_1kb, int expected_score_1kb);


uint32 MakeLangProb(Language lang, int qprob);


void ProcessProbV2Tote(uint32 probs, Tote* tote);


int GetLangScore(uint32 probs, uint8 pslang);

static inline int minint(int a, int b) {return (a < b) ? a: b;}
static inline int maxint(int a, int b) {return (a > b) ? a: b;}

}       

#endif  


