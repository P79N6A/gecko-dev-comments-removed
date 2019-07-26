


















#include "scoreonescriptspan.h"

#include "cldutil.h"
#include "debug.h"
#include "lang_script.h"

#include <stdio.h>

using namespace std;

namespace CLD2 {

static const int kUnreliablePercentThreshold = 75;

void AddLangProb(uint32 langprob, Tote* chunk_tote) {
  ProcessProbV2Tote(langprob, chunk_tote);
}

void ZeroPSLang(uint32 langprob, Tote* chunk_tote) {
  uint8 top1 = (langprob >> 8) & 0xff;
  chunk_tote->SetScore(top1, 0);
}

bool SameCloseSet(uint16 lang1, uint16 lang2) {
  int lang1_close_set = LanguageCloseSet(static_cast<Language>(lang1));
  if (lang1_close_set == 0) {return false;}
  int lang2_close_set = LanguageCloseSet(static_cast<Language>(lang2));
  return (lang1_close_set == lang2_close_set);
}

bool SameCloseSet(Language lang1, Language lang2) {
  int lang1_close_set = LanguageCloseSet(lang1);
  if (lang1_close_set == 0) {return false;}
  int lang2_close_set = LanguageCloseSet(lang2);
  return (lang1_close_set == lang2_close_set);
}



void SetChunkSummary(ULScript ulscript, int first_linear_in_chunk,
                     int offset, int len,
                     const ScoringContext* scoringcontext,
                     const Tote* chunk_tote,
                     ChunkSummary* chunksummary) {
  int key3[3];
  chunk_tote->CurrentTopThreeKeys(key3);
  Language lang1 = FromPerScriptNumber(ulscript, key3[0]);
  Language lang2 = FromPerScriptNumber(ulscript, key3[1]);

  int actual_score_per_kb = 0;
  if (len > 0) {
    actual_score_per_kb = (chunk_tote->GetScore(key3[0]) << 10) / len;
  }
  int expected_subscr = lang1 * 4 + LScript4(ulscript);
  int expected_score_per_kb =
     scoringcontext->scoringtables->kExpectedScore[expected_subscr];

  chunksummary->offset = offset;
  chunksummary->chunk_start = first_linear_in_chunk;
  chunksummary->lang1 = lang1;
  chunksummary->lang2 = lang2;
  chunksummary->score1 = chunk_tote->GetScore(key3[0]);
  chunksummary->score2 = chunk_tote->GetScore(key3[1]);
  chunksummary->bytes = len;
  chunksummary->grams = chunk_tote->GetScoreCount();
  chunksummary->ulscript = ulscript;
  chunksummary->reliability_delta = ReliabilityDelta(chunksummary->score1,
                                                     chunksummary->score2,
                                                     chunksummary->grams);
  
  if (SameCloseSet(lang1, lang2)) {
    chunksummary->reliability_delta = 100;
  }
  chunksummary->reliability_score =
     ReliabilityExpected(actual_score_per_kb, expected_score_per_kb);
}


bool IsSingleLang(uint32 langprob) {
  
  return ((langprob & 0x00ffff00) == 0);
}


void AddDistinctBoost1(uint32 langprob, ScoringContext* scoringcontext) {
  
}




void AddDistinctBoost2(uint32 langprob, ScoringContext* scoringcontext) {

  LangBoosts* distinct_boost = &scoringcontext->distinct_boost.latn;
  if (scoringcontext->ulscript != ULScript_Latin) {
    distinct_boost = &scoringcontext->distinct_boost.othr;
  }
  int n = distinct_boost->n;
  distinct_boost->langprob[n] = langprob;
  distinct_boost->n = distinct_boost->wrap(n + 1);
}



void ScoreBoosts(const ScoringContext* scoringcontext, Tote* chunk_tote) {
  
  const LangBoosts* langprior_boost = &scoringcontext->langprior_boost.latn;
  const LangBoosts* langprior_whack = &scoringcontext->langprior_whack.latn;
  const LangBoosts* distinct_boost = &scoringcontext->distinct_boost.latn;
  if (scoringcontext->ulscript != ULScript_Latin) {
    langprior_boost = &scoringcontext->langprior_boost.othr;
    langprior_whack = &scoringcontext->langprior_whack.othr;
    distinct_boost = &scoringcontext->distinct_boost.othr;
  }

  for (int k = 0; k < kMaxBoosts; ++k) {
    uint32 langprob = langprior_boost->langprob[k];
    if (langprob > 0) {AddLangProb(langprob, chunk_tote);}
  }
  for (int k = 0; k < kMaxBoosts; ++k) {
    uint32 langprob = distinct_boost->langprob[k];
    if (langprob > 0) {AddLangProb(langprob, chunk_tote);}
  }
  
  
  
  
  for (int k = 0; k < kMaxBoosts; ++k) {
    uint32 langprob = langprior_whack->langprob[k];
    if (langprob > 0) {ZeroPSLang(langprob, chunk_tote);}
  }
}











void GetTextSpanOffsets(const ScoringHitBuffer* hitbuffer,
                        const ChunkSpan* cspan, int* lo, int* hi) {
  
  int lo_base = hitbuffer->base[cspan->chunk_base].offset;
  int lo_delta = hitbuffer->delta[cspan->chunk_delta].offset;
  int lo_distinct = hitbuffer->distinct[cspan->chunk_distinct].offset;
  
  int hi_base = hitbuffer->base[cspan->chunk_base +
    cspan->base_len].offset;
  int hi_delta = hitbuffer->delta[cspan->chunk_delta +
    cspan->delta_len].offset;
  int hi_distinct = hitbuffer->distinct[cspan->chunk_distinct +
    cspan->distinct_len].offset;

  *lo = 0;



  *lo = minint(minint(lo_base, lo_delta), lo_distinct);
  *hi = minint(minint(hi_base, hi_delta), hi_distinct);
}


int DiffScore(const CLD2TableSummary* obj, int indirect,
              uint16 lang1, uint16 lang2) {
  if (indirect < static_cast<int>(obj->kCLDTableSizeOne)) {
    
    uint32 langprob = obj->kCLDTableInd[indirect];
    return GetLangScore(langprob, lang1) - GetLangScore(langprob, lang2);
  } else {
    
    indirect += (indirect - obj->kCLDTableSizeOne);
    uint32 langprob = obj->kCLDTableInd[indirect];
    uint32 langprob2 = obj->kCLDTableInd[indirect + 1];
    return (GetLangScore(langprob, lang1) + GetLangScore(langprob2, lang1)) -
      (GetLangScore(langprob, lang2) + GetLangScore(langprob2, lang2));
  }

}





void ScoreOneChunk(const char* text, ULScript ulscript,
                   const ScoringHitBuffer* hitbuffer,
                   int chunk_i,
                   ScoringContext* scoringcontext,
                   ChunkSpan* cspan, Tote* chunk_tote,
                   ChunkSummary* chunksummary) {
  int first_linear_in_chunk = hitbuffer->chunk_start[chunk_i];
  int first_linear_in_next_chunk = hitbuffer->chunk_start[chunk_i + 1];

  chunk_tote->Reinit();
  cspan->delta_len = 0;
  cspan->distinct_len = 0;
  if (scoringcontext->flags_cld2_verbose) {
    fprintf(scoringcontext->debug_file, "<br>ScoreOneChunk[%d..%d) ",
            first_linear_in_chunk, first_linear_in_next_chunk);
  }

  
  cspan->chunk_base = first_linear_in_chunk;
  cspan->base_len = first_linear_in_next_chunk - first_linear_in_chunk;
  for (int i = first_linear_in_chunk; i < first_linear_in_next_chunk; ++i) {
    uint32 langprob = hitbuffer->linear[i].langprob;
    AddLangProb(langprob, chunk_tote);
    if (hitbuffer->linear[i].type <= QUADHIT) {
      chunk_tote->AddScoreCount();      
    }
    if (hitbuffer->linear[i].type == DISTINCTHIT) {
      AddDistinctBoost2(langprob, scoringcontext);
    }
  }

  
  
  ScoreBoosts(scoringcontext, chunk_tote);

  int lo = hitbuffer->linear[first_linear_in_chunk].offset;
  int hi = hitbuffer->linear[first_linear_in_next_chunk].offset;

  
  SetChunkSummary(ulscript, first_linear_in_chunk, lo, hi - lo,
                  scoringcontext, chunk_tote, chunksummary);

  bool more_to_come = false;
  bool score_cjk = false;
  if (scoringcontext->flags_cld2_html) {
    
    CLD2_Debug(text, lo, hi, more_to_come, score_cjk, hitbuffer,
               scoringcontext, cspan, chunksummary);
  }

  scoringcontext->prior_chunk_lang = static_cast<Language>(chunksummary->lang1);
}





void ScoreAllHits(const char* text,  ULScript ulscript,
                  bool more_to_come, bool score_cjk,
                  const ScoringHitBuffer* hitbuffer,
                  ScoringContext* scoringcontext,
                  SummaryBuffer* summarybuffer, ChunkSpan* last_cspan) {
  ChunkSpan prior_cspan = {0, 0, 0, 0, 0, 0};
  ChunkSpan cspan = {0, 0, 0, 0, 0, 0};

  for (int i = 0; i < hitbuffer->next_chunk_start; ++i) {
    
    
    Tote chunk_tote;
    ChunkSummary chunksummary;
    ScoreOneChunk(text, ulscript,
                  hitbuffer, i,
                  scoringcontext, &cspan, &chunk_tote, &chunksummary);

    
    if (summarybuffer->n < kMaxSummaries) {
      summarybuffer->chunksummary[summarybuffer->n] = chunksummary;
      summarybuffer->n += 1;
    }

    prior_cspan = cspan;
    cspan.chunk_base += cspan.base_len;
    cspan.chunk_delta += cspan.delta_len;
    cspan.chunk_distinct += cspan.distinct_len;
  }

  
  int linear_off_end = hitbuffer->next_linear;
  int offset_off_end = hitbuffer->linear[linear_off_end].offset;
  ChunkSummary* cs = &summarybuffer->chunksummary[summarybuffer->n];
  memset(cs, 0, sizeof(ChunkSummary));
  cs->offset = offset_off_end;
  cs->chunk_start = linear_off_end;
  *last_cspan = prior_cspan;
}


void SummaryBufferToDocTote(const SummaryBuffer* summarybuffer,
                            bool more_to_come, DocTote* doc_tote) {
  int cs_bytes_sum = 0;
  for (int i = 0; i < summarybuffer->n; ++i) {
    const ChunkSummary* cs = &summarybuffer->chunksummary[i];
    int reliability = minint(cs->reliability_delta, cs->reliability_score);
    
    doc_tote->Add(cs->lang1, cs->bytes, cs->score1, reliability);
    cs_bytes_sum += cs->bytes;
  }
}


static const bool kShowLettersOriginal = false;




void ItemToVector(ScriptScanner* scanner,
                  ResultChunkVector* vec, Language new_lang,
                  int mapped_offset, int mapped_len) {
  uint16 last_vec_lang = static_cast<uint16>(UNKNOWN_LANGUAGE);
  int last_vec_subscr = vec->size() - 1;
  if (last_vec_subscr >= 0) {
    ResultChunk* priorrc = &(*vec)[last_vec_subscr];
    last_vec_lang = priorrc->lang1;
    if (new_lang == last_vec_lang) {
      
      
      priorrc->bytes = minint((mapped_offset + mapped_len) - priorrc->offset,
                              kMaxResultChunkBytes);
      if (kShowLettersOriginal) {
        
        string temp2(&scanner->GetBufferStart()[priorrc->offset],
                     priorrc->bytes);
        fprintf(stderr, "Item[%d..%d) '%s'<br>\n",
                priorrc->offset, priorrc->offset + priorrc->bytes,
                GetHtmlEscapedText(temp2).c_str());
      }
      return;
    }
  }
  
  ResultChunk rc;
  rc.offset = mapped_offset;
  rc.bytes = minint(mapped_len, kMaxResultChunkBytes);
  rc.lang1 = static_cast<uint16>(new_lang);
  vec->push_back(rc);
  if (kShowLettersOriginal) {
    
    string temp2(&scanner->GetBufferStart()[rc.offset], rc.bytes);
    fprintf(stderr, "Item[%d..%d) '%s'<br>\n",
            rc.offset, rc.offset + rc.bytes,
            GetHtmlEscapedText(temp2).c_str());
  }
}

uint16 PriorVecLang(const ResultChunkVector* vec) {
  if (vec->empty()) {return static_cast<uint16>(UNKNOWN_LANGUAGE);}
  return (*vec)[vec->size() - 1].lang1;
}

uint16 NextChunkLang(const SummaryBuffer* summarybuffer, int i) {
  if ((i + 1) >= summarybuffer->n) {
    return static_cast<uint16>(UNKNOWN_LANGUAGE);
  }
  return summarybuffer->chunksummary[i + 1].lang1;
}
















void SummaryBufferToVector(ScriptScanner* scanner, const char* text,
                           const SummaryBuffer* summarybuffer,
                           bool more_to_come, ResultChunkVector* vec) {
  if (vec == NULL) {return;}

  if (kShowLettersOriginal) {
    fprintf(stderr, "map2original_ ");
    scanner->map2original_.DumpWindow();
    fprintf(stderr, "<br>\n");
    fprintf(stderr, "map2uplow_ ");
    scanner->map2uplow_.DumpWindow();
    fprintf(stderr, "<br>\n");
  }

  for (int i = 0; i < summarybuffer->n; ++i) {
    const ChunkSummary* cs = &summarybuffer->chunksummary[i];
    int unmapped_offset = cs->offset;
    int unmapped_len = cs->bytes;

    if (kShowLettersOriginal) {
      
      string temp(&text[unmapped_offset], unmapped_len);
      fprintf(stderr, "Letters [%d..%d) '%s'<br>\n",
              unmapped_offset, unmapped_offset + unmapped_len,
              GetHtmlEscapedText(temp).c_str());
    }

    int mapped_offset = scanner->MapBack(unmapped_offset);

    
    if (mapped_offset > 0) {
      
      int prior_size = 0;
      if (!vec->empty()) {
        ResultChunk* rc = &(*vec)[vec->size() - 1];
        prior_size = rc->bytes;
      }
      
      
      int n_limit = minint(prior_size - 3, mapped_offset);
      n_limit = minint(n_limit, 12);

      
      
      const char* s = &scanner->GetBufferStart()[mapped_offset];
      const unsigned char* us = reinterpret_cast<const unsigned char*>(s);
      int n = 0;
      while ((n < n_limit) && (us[-n - 1] >= 0x41)) {++n;}
      if (n >= n_limit) {n = 0;} 

      
      if (n < n_limit) {
        unsigned char c = us[-n - 1];
        if ((c == '\'') || (c == '"') || (c == '#') || (c == '@')) {++n;}
      }
      
      if (n > 0) {
        ResultChunk* rc = &(*vec)[vec->size() - 1];
        rc->bytes -= n;
        mapped_offset -= n;
        if (kShowLettersOriginal) {
          fprintf(stderr, "Back up %d bytes<br>\n", n);
          
          string temp2(&scanner->GetBufferStart()[rc->offset], rc->bytes);
          fprintf(stderr, "Prior   [%d..%d) '%s'<br>\n",
                  rc->offset, rc->offset + rc->bytes,
                  GetHtmlEscapedText(temp2).c_str());
        }
      }
    }

    int mapped_len =
      scanner->MapBack(unmapped_offset + unmapped_len) - mapped_offset;

    if (kShowLettersOriginal) {
      
      string temp2(&scanner->GetBufferStart()[mapped_offset], mapped_len);
      fprintf(stderr, "Original[%d..%d) '%s'<br>\n",
              mapped_offset, mapped_offset + mapped_len,
              GetHtmlEscapedText(temp2).c_str());
    }

    Language new_lang = static_cast<Language>(cs->lang1);
    bool reliability_delta_bad =
      (cs->reliability_delta < kUnreliablePercentThreshold);
    bool reliability_score_bad =
      (cs->reliability_score < kUnreliablePercentThreshold);

    
    uint16 prior_lang = PriorVecLang(vec);
    if (prior_lang == cs->lang1) {
      reliability_delta_bad = false;
    }
    
    if (SameCloseSet(cs->lang1, prior_lang)) {
      new_lang = static_cast<Language>(prior_lang);
      reliability_delta_bad = false;
    }
    
    
    if (SameCloseSet(cs->lang1, cs->lang2) &&
        (prior_lang == cs->lang2)) {
      new_lang = static_cast<Language>(prior_lang);
      reliability_delta_bad = false;
    }
    
    
    uint16 next_lang = NextChunkLang(summarybuffer, i);
    if (reliability_delta_bad &&
        (prior_lang == cs->lang2) && (next_lang == cs->lang2)) {
      new_lang = static_cast<Language>(prior_lang);
      reliability_delta_bad = false;
    }

    if (reliability_delta_bad || reliability_score_bad) {
      new_lang = UNKNOWN_LANGUAGE;
    }
    ItemToVector(scanner, vec, new_lang, mapped_offset, mapped_len);
  }
}



void JustOneItemToVector(ScriptScanner* scanner, const char* text,
                         Language lang1, int unmapped_offset, int unmapped_len,
                         ResultChunkVector* vec) {
  if (vec == NULL) {return;}

  if (kShowLettersOriginal) {
    fprintf(stderr, "map2original_ ");
    scanner->map2original_.DumpWindow();
    fprintf(stderr, "<br>\n");
    fprintf(stderr, "map2uplow_ ");
    scanner->map2uplow_.DumpWindow();
    fprintf(stderr, "<br>\n");
  }

  if (kShowLettersOriginal) {
   
   string temp(&text[unmapped_offset], unmapped_len);
   fprintf(stderr, "Letters1 [%d..%d) '%s'<br>\n",
           unmapped_offset, unmapped_offset + unmapped_len,
           GetHtmlEscapedText(temp).c_str());
  }

  int mapped_offset = scanner->MapBack(unmapped_offset);
  int mapped_len =
    scanner->MapBack(unmapped_offset + unmapped_len) - mapped_offset;

  if (kShowLettersOriginal) {
    
    string temp2(&scanner->GetBufferStart()[mapped_offset], mapped_len);
    fprintf(stderr, "Original1[%d..%d) '%s'<br>\n",
            mapped_offset, mapped_offset + mapped_len,
            GetHtmlEscapedText(temp2).c_str());
  }

  ItemToVector(scanner, vec, lang1, mapped_offset, mapped_len);
}



char* DisplayPiece(const char* next_byte_, int byte_length_);


inline int PrintableIndirect(int x) {
  if ((x & 0x80000000u) != 0) {
    return (x & ~0x80000000u) + 2000000000;
  }
  return x;
}
void DumpHitBuffer(FILE* df, const char* text,
                   const ScoringHitBuffer* hitbuffer) {
  fprintf(df,
          "<br>DumpHitBuffer[%s, next_base/delta/distinct %d, %d, %d)<br>\n",
          ULScriptCode(hitbuffer->ulscript),
          hitbuffer->next_base, hitbuffer->next_delta,
          hitbuffer->next_distinct);
  for (int i = 0; i < hitbuffer->maxscoringhits; ++i) {
    if (i < hitbuffer->next_base) {
      fprintf(df, "Q[%d]%d,%d,%s ",
              i, hitbuffer->base[i].offset,
              PrintableIndirect(hitbuffer->base[i].indirect),
              DisplayPiece(&text[hitbuffer->base[i].offset], 6));
    }
    if (i < hitbuffer->next_delta) {
      fprintf(df, "DL[%d]%d,%d,%s ",
              i, hitbuffer->delta[i].offset, hitbuffer->delta[i].indirect,
              DisplayPiece(&text[hitbuffer->delta[i].offset], 12));
    }
    if (i < hitbuffer->next_distinct) {
      fprintf(df, "D[%d]%d,%d,%s ",
              i, hitbuffer->distinct[i].offset, hitbuffer->distinct[i].indirect,
              DisplayPiece(&text[hitbuffer->distinct[i].offset], 12));
    }
    if (i < hitbuffer->next_base) {
      fprintf(df, "<br>\n");
    }
    if (i > 50) {break;}
  }
  if (hitbuffer->next_base > 50) {
    int i = hitbuffer->next_base;
    fprintf(df, "Q[%d]%d,%d,%s ",
            i, hitbuffer->base[i].offset,
            PrintableIndirect(hitbuffer->base[i].indirect),
            DisplayPiece(&text[hitbuffer->base[i].offset], 6));
  }
  if (hitbuffer->next_delta > 50) {
    int i = hitbuffer->next_delta;
    fprintf(df, "DL[%d]%d,%d,%s ",
            i, hitbuffer->delta[i].offset, hitbuffer->delta[i].indirect,
            DisplayPiece(&text[hitbuffer->delta[i].offset], 12));
  }
  if (hitbuffer->next_distinct > 50) {
    int i = hitbuffer->next_distinct;
    fprintf(df, "D[%d]%d,%d,%s ",
            i, hitbuffer->distinct[i].offset, hitbuffer->distinct[i].indirect,
            DisplayPiece(&text[hitbuffer->distinct[i].offset], 12));
  }
  fprintf(df, "<br>\n");
}


void DumpLinearBuffer(FILE* df, const char* text,
                      const ScoringHitBuffer* hitbuffer) {
  fprintf(df, "<br>DumpLinearBuffer[%d)<br>\n",
          hitbuffer->next_linear);
  
  for (int i = 0; i < hitbuffer->next_linear + 1; ++i) {
    if ((50 < i) && (i < (hitbuffer->next_linear - 1))) {continue;}
    fprintf(df, "[%d]%d,%c=%08x,%s<br>\n",
            i, hitbuffer->linear[i].offset,
            "UQLD"[hitbuffer->linear[i].type],
            hitbuffer->linear[i].langprob,
            DisplayPiece(&text[hitbuffer->linear[i].offset], 6));
  }
  fprintf(df, "<br>\n");

  fprintf(df, "DumpChunkStart[%d]<br>\n", hitbuffer->next_chunk_start);
  for (int i = 0; i < hitbuffer->next_chunk_start + 1; ++i) {
    fprintf(df, "[%d]%d\n", i, hitbuffer->chunk_start[i]);
  }
  fprintf(df, "<br>\n");
}


void DumpChunkSummary(FILE* df, const ChunkSummary* cs) {
  
  fprintf(df, "%d lin[%d] %s.%d %s.%d %dB %d# %s %dRd %dRs<br>\n",
          cs->offset,
          cs->chunk_start,
          LanguageCode(static_cast<Language>(cs->lang1)),
          cs->score1,
          LanguageCode(static_cast<Language>(cs->lang2)),
          cs->score2,
          cs->bytes,
          cs->grams,
          ULScriptCode(static_cast<ULScript>(cs->ulscript)),
          cs->reliability_delta,
          cs->reliability_score);
}

void DumpSummaryBuffer(FILE* df, const SummaryBuffer* summarybuffer) {
  fprintf(df, "<br>DumpSummaryBuffer[%d]<br>\n", summarybuffer->n);
  fprintf(df, "[i] offset linear[chunk_start] lang.score1 lang.score2 "
              "bytesB ngrams# script rel_delta rel_score<br>\n");
  for (int i = 0; i <= summarybuffer->n; ++i) {
    fprintf(df, "[%d] ", i);
    DumpChunkSummary(df, &summarybuffer->chunksummary[i]);
  }
  fprintf(df, "<br>\n");
}









int BetterBoundary(const char* text,
                   ScoringHitBuffer* hitbuffer,
                   ScoringContext* scoringcontext,
                   uint16 pslang0, uint16 pslang1,
                   int linear0, int linear1, int linear2) {
  
  if ((linear2 - linear0) <= 8) {return linear1;}

  
  
  
  
  int running_diff = 0;
  int diff[8];    
  
  for (int i = linear0; i < linear0 + 8; ++i) {
    int j = i & 7;
    uint32 langprob = hitbuffer->linear[i].langprob;
    diff[j] = GetLangScore(langprob, pslang0) -
       GetLangScore(langprob, pslang1);
    if (i < linear0 + 4) {
      
      running_diff += diff[j];
    } else {
      
      running_diff -= diff[j];
    }
  }

  
  
  int better_boundary_value = 0;
  int better_boundary = linear1;
  for (int i = linear0; i < linear2 - 8; ++i) {
    int j = i & 7;
    if (better_boundary_value < running_diff) {
      bool has_plus = false;
      bool has_minus = false;
      for (int kk = 0; kk < 8; ++kk) {
        if (diff[kk] > 0) {has_plus = true;}
        if (diff[kk] < 0) {has_minus = true;}
      }
      if (has_plus && has_minus) {
        better_boundary_value = running_diff;
        better_boundary = i + 4;
      }
    }
    
    uint32 langprob = hitbuffer->linear[i + 8].langprob;
    int newdiff = GetLangScore(langprob, pslang0) -
       GetLangScore(langprob, pslang1);
    int middiff = diff[(i + 4) & 7];
    int olddiff = diff[j];
    diff[j] = newdiff;
    running_diff -= olddiff;                  
    running_diff += 2 * middiff;              
    running_diff -= newdiff;                  
  }

  if (scoringcontext->flags_cld2_verbose && (linear1 != better_boundary)) {
    Language lang0 = FromPerScriptNumber(scoringcontext->ulscript, pslang0);
    Language lang1 = FromPerScriptNumber(scoringcontext->ulscript, pslang1);
    fprintf(scoringcontext->debug_file, " Better lin[%d=>%d] %s^^%s <br>\n",
            linear1, better_boundary,
            LanguageCode(lang0), LanguageCode(lang1));
    int lin0_off = hitbuffer->linear[linear0].offset;
    int lin1_off = hitbuffer->linear[linear1].offset;
    int lin2_off = hitbuffer->linear[linear2].offset;
    int better_offm1 = hitbuffer->linear[better_boundary - 1].offset;
    int better_off = hitbuffer->linear[better_boundary].offset;
    int better_offp1 = hitbuffer->linear[better_boundary + 1].offset;
    string old0(&text[lin0_off], lin1_off - lin0_off);
    string old1(&text[lin1_off], lin2_off - lin1_off);
    string new0(&text[lin0_off], better_offm1 - lin0_off);
    string new0m1(&text[better_offm1], better_off - better_offm1);
    string new1(&text[better_off], better_offp1 - better_off);
    string new1p1(&text[better_offp1], lin2_off - better_offp1);
    fprintf(scoringcontext->debug_file, "%s^^%s => <br>\n%s^%s^^%s^%s<br>\n",
            GetHtmlEscapedText(old0).c_str(),
            GetHtmlEscapedText(old1).c_str(),
            GetHtmlEscapedText(new0).c_str(),
            GetHtmlEscapedText(new0m1).c_str(),
            GetHtmlEscapedText(new1).c_str(),
            GetHtmlEscapedText(new1p1).c_str());
    
    int d;
    for (int i = linear0; i < linear2; ++i) {
      if (i == better_boundary) {
        fprintf(scoringcontext->debug_file, "^^ ");
      }
      uint32 langprob = hitbuffer->linear[i].langprob;
      d = GetLangScore(langprob, pslang0) - GetLangScore(langprob, pslang1);
      const char* s = "=";
      
      if (d > 2) {s = "#";}
      else if (d > 0) {s = "+";}
      else if (d < -2) {s = "_";}
      else if (d < 0) {s = "-";}
      fprintf(scoringcontext->debug_file, "%s ", s);
    }
    fprintf(scoringcontext->debug_file, " &nbsp;&nbsp;(scale: #+=-_)<br>\n");
  }
  return better_boundary;
}





void SharpenBoundaries(const char* text,
                       bool more_to_come,
                       ScoringHitBuffer* hitbuffer,
                       ScoringContext* scoringcontext,
                       SummaryBuffer* summarybuffer) {

  int prior_linear = summarybuffer->chunksummary[0].chunk_start;
  uint16 prior_lang = summarybuffer->chunksummary[0].lang1;

  if (scoringcontext->flags_cld2_verbose) {
    fprintf(scoringcontext->debug_file, "<br>SharpenBoundaries<br>\n");
  }
  for (int i = 1; i < summarybuffer->n; ++i) {
    ChunkSummary* cs = &summarybuffer->chunksummary[i];
    uint16 this_lang = cs->lang1;
    if (this_lang == prior_lang) {
      prior_linear = cs->chunk_start;
      continue;
    }

    int this_linear = cs->chunk_start;
    int next_linear = summarybuffer->chunksummary[i + 1].chunk_start;

    
    if (SameCloseSet(prior_lang, this_lang)) {
      prior_linear = this_linear;
      prior_lang = this_lang;
      continue;
    }


    
    
    
    
    
    

    uint8 pslang0 = PerScriptNumber(scoringcontext->ulscript,
                                    static_cast<Language>(prior_lang));
    uint8 pslang1 = PerScriptNumber(scoringcontext->ulscript,
                                    static_cast<Language>(this_lang));
    int better_linear = BetterBoundary(text,
                                       hitbuffer,
                                       scoringcontext,
                                       pslang0, pslang1,
                                       prior_linear, this_linear, next_linear);

    int old_offset = hitbuffer->linear[this_linear].offset;
    int new_offset = hitbuffer->linear[better_linear].offset;
    cs->chunk_start = better_linear;
    cs->offset = new_offset;
    
    
    cs->bytes -= (new_offset - old_offset);
    summarybuffer->chunksummary[i - 1].bytes += (new_offset - old_offset);

    this_linear = better_linear;    

    

    
    prior_linear = this_linear;
    prior_lang = this_lang;
  }
}


uint32 DefaultLangProb(ULScript ulscript) {
  Language default_lang = DefaultLanguage(ulscript);
  return MakeLangProb(default_lang, 1);
}




void LinearizeAll(ScoringContext* scoringcontext, bool score_cjk,
                  ScoringHitBuffer* hitbuffer) {
  const CLD2TableSummary* base_obj;       
  const CLD2TableSummary* base_obj2;      
  const CLD2TableSummary* delta_obj;      
  const CLD2TableSummary* distinct_obj;   
  uint16 base_hit;
  if (score_cjk) {
    base_obj = scoringcontext->scoringtables->unigram_compat_obj;
    base_obj2 = scoringcontext->scoringtables->unigram_compat_obj;
    delta_obj = scoringcontext->scoringtables->deltabi_obj;
    distinct_obj = scoringcontext->scoringtables->distinctbi_obj;
    base_hit = UNIHIT;
  } else {
    base_obj = scoringcontext->scoringtables->quadgram_obj;
    base_obj2 = scoringcontext->scoringtables->quadgram_obj2;
    delta_obj = scoringcontext->scoringtables->deltaocta_obj;
    distinct_obj = scoringcontext->scoringtables->distinctocta_obj;
    base_hit = QUADHIT;
  }

  int base_limit = hitbuffer->next_base;
  int delta_limit = hitbuffer->next_delta;
  int distinct_limit = hitbuffer->next_distinct;
  int base_i = 0;
  int delta_i = 0;
  int distinct_i = 0;
  int linear_i = 0;

  
  
  hitbuffer->linear[linear_i].offset = hitbuffer->lowest_offset;
  hitbuffer->linear[linear_i].type = base_hit;
  hitbuffer->linear[linear_i].langprob =
    DefaultLangProb(scoringcontext->ulscript);
  ++linear_i;

  while ((base_i < base_limit) || (delta_i < delta_limit) ||
         (distinct_i < distinct_limit)) {
    int base_off = hitbuffer->base[base_i].offset;
    int delta_off = hitbuffer->delta[delta_i].offset;
    int distinct_off = hitbuffer->distinct[distinct_i].offset;

    
    if ((delta_i < delta_limit) &&
        (delta_off <= base_off) && (delta_off <= distinct_off)) {
      
      int indirect = hitbuffer->delta[delta_i].indirect;
      ++delta_i;
      uint32 langprob = delta_obj->kCLDTableInd[indirect];
      if (langprob > 0) {
        hitbuffer->linear[linear_i].offset = delta_off;
        hitbuffer->linear[linear_i].type = DELTAHIT;
        hitbuffer->linear[linear_i].langprob = langprob;
        ++linear_i;
      }
    }
    else if ((distinct_i < distinct_limit) &&
             (distinct_off <= base_off) && (distinct_off <= delta_off)) {
      
      int indirect = hitbuffer->distinct[distinct_i].indirect;
      ++distinct_i;
      uint32 langprob = distinct_obj->kCLDTableInd[indirect];
      if (langprob > 0) {
        hitbuffer->linear[linear_i].offset = distinct_off;
        hitbuffer->linear[linear_i].type = DISTINCTHIT;
        hitbuffer->linear[linear_i].langprob = langprob;
        ++linear_i;
      }
    }
    else {
      
      int indirect = hitbuffer->base[base_i].indirect;
      
      const CLD2TableSummary* local_base_obj = base_obj;
      if ((indirect & 0x80000000u) != 0) {
        local_base_obj = base_obj2;
        indirect &= ~0x80000000u;
      }
      ++base_i;
      
      
      if (indirect < static_cast<int>(local_base_obj->kCLDTableSizeOne)) {
        
        uint32 langprob = local_base_obj->kCLDTableInd[indirect];
        if (langprob > 0) {
          hitbuffer->linear[linear_i].offset = base_off;
          hitbuffer->linear[linear_i].type = base_hit;
          hitbuffer->linear[linear_i].langprob = langprob;
          ++linear_i;
        }
      } else {
        
        indirect += (indirect - local_base_obj->kCLDTableSizeOne);
        uint32 langprob = local_base_obj->kCLDTableInd[indirect];
        uint32 langprob2 = local_base_obj->kCLDTableInd[indirect + 1];
        if (langprob > 0) {
          hitbuffer->linear[linear_i].offset = base_off;
          hitbuffer->linear[linear_i].type = base_hit;
          hitbuffer->linear[linear_i].langprob = langprob;
          ++linear_i;
        }
        if (langprob2 > 0) {
          hitbuffer->linear[linear_i].offset = base_off;
          hitbuffer->linear[linear_i].type = base_hit;
          hitbuffer->linear[linear_i].langprob = langprob2;
          ++linear_i;
        }
      }
    }
  }

  
  hitbuffer->next_linear = linear_i;

  
  hitbuffer->linear[linear_i].offset =
  hitbuffer->base[hitbuffer->next_base].offset;
  hitbuffer->linear[linear_i].langprob = 0;
}


void ChunkAll(int letter_offset, bool score_cjk, ScoringHitBuffer* hitbuffer) {
  int chunksize;
  uint16 base_hit;
  if (score_cjk) {
    chunksize = kChunksizeUnis;
    base_hit = UNIHIT;
  } else {
    chunksize = kChunksizeQuads;
    base_hit = QUADHIT;
  }

  int linear_i = 0;
  int linear_off_end = hitbuffer->next_linear;
  int text_i = letter_offset;               
  int next_chunk_start = 0;
  int bases_left = hitbuffer->next_base;
  while (bases_left > 0) {
    
    int base_len = chunksize;     
    if (bases_left < (chunksize + (chunksize >> 1))) {
      
      base_len = bases_left;
    } else if (bases_left < (2 * chunksize)) {
      
      base_len = (bases_left + 1) >> 1;
    }

    hitbuffer->chunk_start[next_chunk_start] = linear_i;
    hitbuffer->chunk_offset[next_chunk_start] = text_i;
    ++next_chunk_start;

    int base_count = 0;
    while ((base_count < base_len) && (linear_i < linear_off_end)) {
      if (hitbuffer->linear[linear_i].type == base_hit) {++base_count;}
      ++linear_i;
    }
    text_i = hitbuffer->linear[linear_i].offset;    
    bases_left -= base_len;
  }

  
  if (next_chunk_start == 0) {
     hitbuffer->chunk_start[next_chunk_start] = 0;
     hitbuffer->chunk_offset[next_chunk_start] = hitbuffer->linear[0].offset;
     ++next_chunk_start;
  }

  
  hitbuffer->next_chunk_start = next_chunk_start;

  
  hitbuffer->chunk_start[next_chunk_start] = hitbuffer->next_linear;
  hitbuffer->chunk_offset[next_chunk_start] = text_i;
}











void LinearizeHitBuffer(int letter_offset,
                        ScoringContext* scoringcontext,
                        bool more_to_come, bool score_cjk,
                        ScoringHitBuffer* hitbuffer) {
  LinearizeAll(scoringcontext, score_cjk, hitbuffer);
  ChunkAll(letter_offset, score_cjk, hitbuffer);
}

















void ProcessHitBuffer(const LangSpan& scriptspan,
                      int letter_offset,
                      ScoringContext* scoringcontext,
                      DocTote* doc_tote,
                      ResultChunkVector* vec,
                      bool more_to_come, bool score_cjk,
                      ScoringHitBuffer* hitbuffer) {
  if (scoringcontext->flags_cld2_verbose) {
    fprintf(scoringcontext->debug_file, "Hitbuffer[) ");
    DumpHitBuffer(scoringcontext->debug_file, scriptspan.text, hitbuffer);
  }

  LinearizeHitBuffer(letter_offset, scoringcontext, more_to_come, score_cjk,
                     hitbuffer);

  if (scoringcontext->flags_cld2_verbose) {
    fprintf(scoringcontext->debug_file, "Linear[) ");
    DumpLinearBuffer(scoringcontext->debug_file, scriptspan.text, hitbuffer);
  }

  SummaryBuffer summarybuffer;
  summarybuffer.n = 0;
  ChunkSpan last_cspan;
  ScoreAllHits(scriptspan.text, scriptspan.ulscript,
                    more_to_come, score_cjk, hitbuffer,
                    scoringcontext,
                    &summarybuffer, &last_cspan);

  if (scoringcontext->flags_cld2_verbose) {
    DumpSummaryBuffer(scoringcontext->debug_file, &summarybuffer);
  }

  if (vec != NULL) {
    
    
    SharpenBoundaries(scriptspan.text, more_to_come, hitbuffer, scoringcontext,
                      &summarybuffer);
    
    
    

    if (scoringcontext->flags_cld2_verbose) {
      DumpSummaryBuffer(scoringcontext->debug_file, &summarybuffer);
    }
  }

  SummaryBufferToDocTote(&summarybuffer, more_to_come, doc_tote);
  SummaryBufferToVector(scoringcontext->scanner, scriptspan.text,
                        &summarybuffer, more_to_come, vec);
}

void SpliceHitBuffer(ScoringHitBuffer* hitbuffer, int next_offset) {
  
  
  hitbuffer->next_base = 0;
  hitbuffer->next_delta = 0;
  hitbuffer->next_distinct = 0;
  hitbuffer->next_linear = 0;
  hitbuffer->next_chunk_start = 0;
  hitbuffer->lowest_offset = next_offset;
}




void ScoreEntireScriptSpan(const LangSpan& scriptspan,
                           ScoringContext* scoringcontext,
                           DocTote* doc_tote,
                           ResultChunkVector* vec) {
  int bytes = scriptspan.text_bytes;
  
  int score = bytes;
  int reliability = 100;
  
  Language one_one_lang = DefaultLanguage(scriptspan.ulscript);
  doc_tote->Add(one_one_lang, bytes, score, reliability);

  if (scoringcontext->flags_cld2_html) {
    ChunkSummary chunksummary = {
      1, 0,
      one_one_lang, UNKNOWN_LANGUAGE, score, 1,
      bytes, 0, scriptspan.ulscript, reliability, reliability
    };
    CLD2_Debug(scriptspan.text, 1, scriptspan.text_bytes,
               false, false, NULL,
               scoringcontext, NULL, &chunksummary);
  }

  
  JustOneItemToVector(scoringcontext->scanner, scriptspan.text,
                      one_one_lang, 1, bytes - 1, vec);

  scoringcontext->prior_chunk_lang = UNKNOWN_LANGUAGE;
}


void ScoreCJKScriptSpan(const LangSpan& scriptspan,
                        ScoringContext* scoringcontext,
                        DocTote* doc_tote,
                        ResultChunkVector* vec) {
  
  ScoringHitBuffer* hitbuffer = new ScoringHitBuffer;
  hitbuffer->init();
  hitbuffer->ulscript = scriptspan.ulscript;

  scoringcontext->prior_chunk_lang = UNKNOWN_LANGUAGE;
  scoringcontext->oldest_distinct_boost = 0;

  
  

  int letter_offset = 1;        
  hitbuffer->lowest_offset = letter_offset;
  int letter_limit = scriptspan.text_bytes;
  while (letter_offset < letter_limit) {
    if (scoringcontext->flags_cld2_verbose) {
      fprintf(scoringcontext->debug_file, " ScoreCJKScriptSpan[%d,%d)<br>\n",
              letter_offset, letter_limit);
    }
    
    
    
    
    
    
    int next_offset = GetUniHits(scriptspan.text, letter_offset, letter_limit,
                                  scoringcontext, hitbuffer);
    
    
    GetBiHits(scriptspan.text, letter_offset, next_offset,
                scoringcontext, hitbuffer);

    
    
    
    bool more_to_come = next_offset < letter_limit;
    bool score_cjk = true;
    ProcessHitBuffer(scriptspan, letter_offset, scoringcontext, doc_tote, vec,
                     more_to_come, score_cjk, hitbuffer);
    SpliceHitBuffer(hitbuffer, next_offset);

    letter_offset = next_offset;
  }

  delete hitbuffer;
  
  scoringcontext->prior_chunk_lang = UNKNOWN_LANGUAGE;
}
















void ScoreQuadScriptSpan(const LangSpan& scriptspan,
                         ScoringContext* scoringcontext,
                         DocTote* doc_tote,
                         ResultChunkVector* vec) {
  
  ScoringHitBuffer* hitbuffer = new ScoringHitBuffer;
  hitbuffer->init();
  hitbuffer->ulscript = scriptspan.ulscript;

  scoringcontext->prior_chunk_lang = UNKNOWN_LANGUAGE;
  scoringcontext->oldest_distinct_boost = 0;

  
  

  int letter_offset = 1;        
  hitbuffer->lowest_offset = letter_offset;
  int letter_limit = scriptspan.text_bytes;
  while (letter_offset < letter_limit) {
    
    
    
    
    
    
    int next_offset = GetQuadHits(scriptspan.text, letter_offset, letter_limit,
                                  scoringcontext, hitbuffer);
    
    
    
    GetOctaHits(scriptspan.text, letter_offset, next_offset,
                scoringcontext, hitbuffer);

    
    
    
    bool more_to_come = next_offset < letter_limit;
    bool score_cjk = false;
    ProcessHitBuffer(scriptspan, letter_offset, scoringcontext, doc_tote, vec,
                     more_to_come, score_cjk, hitbuffer);
    SpliceHitBuffer(hitbuffer, next_offset);

    letter_offset = next_offset;
  }

  delete hitbuffer;
}
























void ScoreOneScriptSpan(const LangSpan& scriptspan,
                        ScoringContext* scoringcontext,
                        DocTote* doc_tote,
                        ResultChunkVector* vec) {
  if (scoringcontext->flags_cld2_verbose) {
    fprintf(scoringcontext->debug_file, "<br>ScoreOneScriptSpan(%s,%d) ",
            ULScriptCode(scriptspan.ulscript), scriptspan.text_bytes);
    
    string temp(&scriptspan.text[0], scriptspan.text_bytes);
    fprintf(scoringcontext->debug_file, "'%s'",
            GetHtmlEscapedText(temp).c_str());
    fprintf(scoringcontext->debug_file, "<br>\n");
  }
  scoringcontext->prior_chunk_lang = UNKNOWN_LANGUAGE;
  scoringcontext->oldest_distinct_boost = 0;
  ULScriptRType rtype = ULScriptRecognitionType(scriptspan.ulscript);
  if (scoringcontext->flags_cld2_score_as_quads && (rtype != RTypeCJK)) {
    rtype = RTypeMany;
  }
  switch (rtype) {
  case RTypeNone:
  case RTypeOne:
    ScoreEntireScriptSpan(scriptspan, scoringcontext, doc_tote, vec);
    break;
  case RTypeCJK:
    ScoreCJKScriptSpan(scriptspan, scoringcontext, doc_tote, vec);
    break;
  case RTypeMany:
    ScoreQuadScriptSpan(scriptspan, scoringcontext, doc_tote, vec);
    break;
  }
}

}       

