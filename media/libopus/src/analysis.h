


























#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "celt.h"
#include "opus_private.h"

#define NB_FRAMES 8
#define NB_TBANDS 18
#define NB_TOT_BANDS 21
#define ANALYSIS_BUF_SIZE 720 /* 15 ms at 48 kHz */

#define DETECT_SIZE 200

typedef struct {
   float angle[240];
   float d_angle[240];
   float d2_angle[240];
   opus_val32 inmem[ANALYSIS_BUF_SIZE];
   int   mem_fill;                      
   float prev_band_tonality[NB_TBANDS];
   float prev_tonality;
   float E[NB_FRAMES][NB_TBANDS];
   float lowE[NB_TBANDS];
   float highE[NB_TBANDS];
   float meanE[NB_TOT_BANDS];
   float mem[32];
   float cmean[8];
   float std[9];
   float music_prob;
   float Etracker;
   float lowECount;
   int E_count;
   int last_music;
   int last_transition;
   int count;
   float subframe_mem[3];
   int analysis_offset;
   

   float pspeech[DETECT_SIZE];
   

   float pmusic[DETECT_SIZE];
   float speech_confidence;
   float music_confidence;
   int speech_confidence_count;
   int music_confidence_count;
   int write_pos;
   int read_pos;
   int read_subframe;
   AnalysisInfo info[DETECT_SIZE];
} TonalityAnalysisState;

void tonality_analysis(TonalityAnalysisState *tonal, AnalysisInfo *info,
     const CELTMode *celt_mode, const void *x, int len, int offset, int c1, int c2, int C, int lsb_depth, downmix_func downmix);

void tonality_get_info(TonalityAnalysisState *tonal, AnalysisInfo *info_out, int len);

void run_analysis(TonalityAnalysisState *analysis, const CELTMode *celt_mode, const void *analysis_pcm,
                 int analysis_frame_size, int frame_size, int c1, int c2, int C, opus_int32 Fs,
                 int lsb_depth, downmix_func downmix, AnalysisInfo *analysis_info);

#endif
