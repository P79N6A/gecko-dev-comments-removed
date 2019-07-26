


















#include "debug.h"
#include <string>

using namespace std;

namespace CLD2 {

string GetPlainEscapedText(const string& txt) {return string("");}

string GetHtmlEscapedText(const string& txt) {return string("");}

string GetColorHtmlEscapedText(Language lang, const string& txt) {
  return string("");
}

string GetLangColorHtmlEscapedText(Language lang, const string& txt) {
  return string("");
}






void CLD2_Debug(const char* text,
                int lo_offset,
                int hi_offset,
                bool more_to_come, bool score_cjk,
                const ScoringHitBuffer* hitbuffer,
                const ScoringContext* scoringcontext,
                const ChunkSpan* cspan,
                const ChunkSummary* chunksummary) {}


void CLD2_Debug2(const char* text,
                 bool more_to_come, bool score_cjk,
                 const ScoringHitBuffer* hitbuffer,
                 const ScoringContext* scoringcontext,
                 const SummaryBuffer* summarybuffer) {}

void DumpResultChunkVector(FILE* f, const char* src,
                           ResultChunkVector* resultchunkvector) {}

}       

