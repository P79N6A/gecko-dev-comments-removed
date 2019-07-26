



















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_DEBUG_H_
#define I18N_ENCODINGS_CLD2_INTERNAL_DEBUG_H_

#include <string>
#include "scoreonescriptspan.h"

namespace CLD2 {


void CLD2_Debug(const char* text,
                int lo_offset,
                int hi_offset,
                bool more_to_come, bool score_cjk,
                const ScoringHitBuffer* hitbuffer,
                const ScoringContext* scoringcontext,
                const ChunkSpan* cspan,
                const ChunkSummary* chunksummary);


void CLD2_Debug2(const char* text,
                 bool more_to_come, bool score_cjk,
                 const ScoringHitBuffer* hitbuffer,
                 const ScoringContext* scoringcontext,
                 const SummaryBuffer* summarybuffer);

std::string GetPlainEscapedText(const std::string& txt);
std::string GetHtmlEscapedText(const std::string& txt);
std::string GetColorHtmlEscapedText(Language lang, const std::string& txt);
std::string GetLangColorHtmlEscapedText(Language lang, const std::string& txt);

void DumpResultChunkVector(FILE* f, const char* src,
                           ResultChunkVector* resultchunkvector);


}     

#endif  

