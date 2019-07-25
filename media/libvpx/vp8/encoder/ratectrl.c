










#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "math.h"
#include "vp8/common/common.h"
#include "ratectrl.h"
#include "vp8/common/entropymode.h"
#include "vpx_mem/vpx_mem.h"
#include "vp8/common/systemdependent.h"
#include "encodemv.h"


#define MIN_BPB_FACTOR          0.01
#define MAX_BPB_FACTOR          50

extern const MB_PREDICTION_MODE vp8_mode_order[MAX_MODES];
extern const MV_REFERENCE_FRAME vp8_ref_frame_order[MAX_MODES];



#ifdef MODE_STATS
extern int y_modes[5];
extern int uv_modes[4];
extern int b_modes[10];

extern int inter_y_modes[10];
extern int inter_uv_modes[4];
extern int inter_b_modes[10];
#endif


#define BPER_MB_NORMBITS    9




const int vp8_bits_per_mb[2][QINDEX_RANGE] =
{
    
    {
        1125000,900000, 750000, 642857, 562500, 500000, 450000, 450000,
        409090, 375000, 346153, 321428, 300000, 281250, 264705, 264705,
        250000, 236842, 225000, 225000, 214285, 214285, 204545, 204545,
        195652, 195652, 187500, 180000, 180000, 173076, 166666, 160714,
        155172, 150000, 145161, 140625, 136363, 132352, 128571, 125000,
        121621, 121621, 118421, 115384, 112500, 109756, 107142, 104651,
        102272, 100000, 97826,  97826,  95744,  93750,  91836,  90000,
        88235,  86538,  84905,  83333,  81818,  80357,  78947,  77586,
        76271,  75000,  73770,  72580,  71428,  70312,  69230,  68181,
        67164,  66176,  65217,  64285,  63380,  62500,  61643,  60810,
        60000,  59210,  59210,  58441,  57692,  56962,  56250,  55555,
        54878,  54216,  53571,  52941,  52325,  51724,  51136,  50561,
        49450,  48387,  47368,  46875,  45918,  45000,  44554,  44117,
        43269,  42452,  41666,  40909,  40178,  39473,  38793,  38135,
        36885,  36290,  35714,  35156,  34615,  34090,  33582,  33088,
        32608,  32142,  31468,  31034,  30405,  29801,  29220,  28662,
    },
    
    {
        712500, 570000, 475000, 407142, 356250, 316666, 285000, 259090,
        237500, 219230, 203571, 190000, 178125, 167647, 158333, 150000,
        142500, 135714, 129545, 123913, 118750, 114000, 109615, 105555,
        101785, 98275,  95000,  91935,  89062,  86363,  83823,  81428,
        79166,  77027,  75000,  73076,  71250,  69512,  67857,  66279,
        64772,  63333,  61956,  60638,  59375,  58163,  57000,  55882,
        54807,  53773,  52777,  51818,  50892,  50000,  49137,  47500,
        45967,  44531,  43181,  41911,  40714,  39583,  38513,  37500,
        36538,  35625,  34756,  33928,  33139,  32386,  31666,  30978,
        30319,  29687,  29081,  28500,  27941,  27403,  26886,  26388,
        25909,  25446,  25000,  24568,  23949,  23360,  22800,  22265,
        21755,  21268,  20802,  20357,  19930,  19520,  19127,  18750,
        18387,  18037,  17701,  17378,  17065,  16764,  16473,  16101,
        15745,  15405,  15079,  14766,  14467,  14179,  13902,  13636,
        13380,  13133,  12895,  12666,  12445,  12179,  11924,  11632,
        11445,  11220,  11003,  10795,  10594,  10401,  10215,  10035,
    }
};

static const int kf_boost_qadjustment[QINDEX_RANGE] =
{
    128, 129, 130, 131, 132, 133, 134, 135,
    136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167,
    168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183,
    184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199,
    200, 200, 201, 201, 202, 203, 203, 203,
    204, 204, 205, 205, 206, 206, 207, 207,
    208, 208, 209, 209, 210, 210, 211, 211,
    212, 212, 213, 213, 214, 214, 215, 215,
    216, 216, 217, 217, 218, 218, 219, 219,
    220, 220, 220, 220, 220, 220, 220, 220,
    220, 220, 220, 220, 220, 220, 220, 220,
};


#define GFQ_ADJUSTMENT vp8_gf_boost_qadjustment[Q]
const int vp8_gf_boost_qadjustment[QINDEX_RANGE] =
{
    80, 82, 84, 86, 88, 90, 92, 94,
    96, 97, 98, 99, 100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135,
    136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167,
    168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183,
    184, 184, 185, 185, 186, 186, 187, 187,
    188, 188, 189, 189, 190, 190, 191, 191,
    192, 192, 193, 193, 194, 194, 194, 194,
    195, 195, 196, 196, 197, 197, 198, 198
};























static const int kf_gf_boost_qlimits[QINDEX_RANGE] =
{
    150, 155, 160, 165, 170, 175, 180, 185,
    190, 195, 200, 205, 210, 215, 220, 225,
    230, 235, 240, 245, 250, 255, 260, 265,
    270, 275, 280, 285, 290, 295, 300, 305,
    310, 320, 330, 340, 350, 360, 370, 380,
    390, 400, 410, 420, 430, 440, 450, 460,
    470, 480, 490, 500, 510, 520, 530, 540,
    550, 560, 570, 580, 590, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
    600, 600, 600, 600, 600, 600, 600, 600,
};


static const int kf_boost_seperation_adjustment[16] =
{
    30,   40,   50,   55,   60,   65,   70,   75,
    80,   85,   90,   95,  100,  100,  100,  100,
};


static const int gf_adjust_table[101] =
{
    100,
    115, 130, 145, 160, 175, 190, 200, 210, 220, 230,
    240, 260, 270, 280, 290, 300, 310, 320, 330, 340,
    350, 360, 370, 380, 390, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
    400, 400, 400, 400, 400, 400, 400, 400, 400, 400,
};

static const int gf_intra_usage_adjustment[20] =
{
    125, 120, 115, 110, 105, 100,  95,  85,  80,  75,
    70,  65,  60,  55,  50,  50,  50,  50,  50,  50,
};

static const int gf_interval_table[101] =
{
    7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
};

static const unsigned int prior_key_frame_weight[KEY_FRAME_CONTEXT] = { 1, 2, 3, 4, 5 };


void vp8_save_coding_context(VP8_COMP *cpi)
{
    CODING_CONTEXT *const cc = & cpi->coding_context;

    
    
    
    

    cc->frames_since_key          = cpi->frames_since_key;
    cc->filter_level             = cpi->common.filter_level;
    cc->frames_till_gf_update_due   = cpi->frames_till_gf_update_due;
    cc->frames_since_golden       = cpi->common.frames_since_golden;

    vp8_copy(cc->mvc,      cpi->common.fc.mvc);
    vp8_copy(cc->mvcosts,  cpi->mb.mvcosts);

    vp8_copy(cc->kf_ymode_prob,   cpi->common.kf_ymode_prob);
    vp8_copy(cc->ymode_prob,   cpi->common.fc.ymode_prob);
    vp8_copy(cc->kf_uv_mode_prob,  cpi->common.kf_uv_mode_prob);
    vp8_copy(cc->uv_mode_prob,  cpi->common.fc.uv_mode_prob);

    vp8_copy(cc->ymode_count, cpi->ymode_count);
    vp8_copy(cc->uv_mode_count, cpi->uv_mode_count);


    
#ifdef MODE_STATS
    vp8_copy(cc->y_modes,       y_modes);
    vp8_copy(cc->uv_modes,      uv_modes);
    vp8_copy(cc->b_modes,       b_modes);
    vp8_copy(cc->inter_y_modes,  inter_y_modes);
    vp8_copy(cc->inter_uv_modes, inter_uv_modes);
    vp8_copy(cc->inter_b_modes,  inter_b_modes);
#endif

    cc->this_frame_percent_intra = cpi->this_frame_percent_intra;
}


void vp8_restore_coding_context(VP8_COMP *cpi)
{
    CODING_CONTEXT *const cc = & cpi->coding_context;

    
    

    cpi->frames_since_key         =   cc->frames_since_key;
    cpi->common.filter_level     =   cc->filter_level;
    cpi->frames_till_gf_update_due  =   cc->frames_till_gf_update_due;
    cpi->common.frames_since_golden       =   cc->frames_since_golden;

    vp8_copy(cpi->common.fc.mvc, cc->mvc);

    vp8_copy(cpi->mb.mvcosts, cc->mvcosts);

    vp8_copy(cpi->common.kf_ymode_prob,   cc->kf_ymode_prob);
    vp8_copy(cpi->common.fc.ymode_prob,   cc->ymode_prob);
    vp8_copy(cpi->common.kf_uv_mode_prob,  cc->kf_uv_mode_prob);
    vp8_copy(cpi->common.fc.uv_mode_prob,  cc->uv_mode_prob);

    vp8_copy(cpi->ymode_count, cc->ymode_count);
    vp8_copy(cpi->uv_mode_count, cc->uv_mode_count);

    
#ifdef MODE_STATS
    vp8_copy(y_modes, cc->y_modes);
    vp8_copy(uv_modes, cc->uv_modes);
    vp8_copy(b_modes, cc->b_modes);
    vp8_copy(inter_y_modes, cc->inter_y_modes);
    vp8_copy(inter_uv_modes, cc->inter_uv_modes);
    vp8_copy(inter_b_modes, cc->inter_b_modes);
#endif


    cpi->this_frame_percent_intra = cc->this_frame_percent_intra;
}


void vp8_setup_key_frame(VP8_COMP *cpi)
{
    

    vp8_default_coef_probs(& cpi->common);
    vp8_kf_default_bmode_probs(cpi->common.kf_bmode_prob);

    vpx_memcpy(cpi->common.fc.mvc, vp8_default_mv_context, sizeof(vp8_default_mv_context));
    {
        int flag[2] = {1, 1};
        vp8_build_component_cost_table(cpi->mb.mvcost, (const MV_CONTEXT *) cpi->common.fc.mvc, flag);
    }

    vpx_memset(cpi->common.fc.pre_mvc, 0, sizeof(cpi->common.fc.pre_mvc));  

    
    cpi->common.filter_level = cpi->common.base_qindex * 3 / 8 ;

    
    if (cpi->auto_gold)
        
        cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;
    else
        cpi->frames_till_gf_update_due = cpi->goldfreq;

    cpi->common.refresh_golden_frame = TRUE;
    cpi->common.refresh_alt_ref_frame = TRUE;
}


static int estimate_bits_at_q(int frame_kind, int Q, int MBs,
                              double correction_factor)
{
    int Bpm = (int)(.5 + correction_factor * vp8_bits_per_mb[frame_kind][Q]);

    



    if (MBs > (1 << 11))
        return (Bpm >> BPER_MB_NORMBITS) * MBs;
    else
        return (Bpm * MBs) >> BPER_MB_NORMBITS;
}


static void calc_iframe_target_size(VP8_COMP *cpi)
{
    
    int kf_boost;
    int target;

    
    vp8_clear_system_state();  

    if (cpi->oxcf.fixed_q >= 0)
    {
        int Q = cpi->oxcf.key_q;

        target = estimate_bits_at_q(INTRA_FRAME, Q, cpi->common.MBs,
                                    cpi->key_frame_rate_correction_factor);
    }
    else if (cpi->pass == 2)
    {
        
        target = cpi->per_frame_bandwidth;
    }
    
    else if (cpi->common.current_video_frame == 0)
    {
        



        target = cpi->oxcf.starting_buffer_level / 2;

        if(target > cpi->oxcf.target_bandwidth * 3 / 2)
            target = cpi->oxcf.target_bandwidth * 3 / 2;
    }
    else
    {
        
        int Q = (cpi->common.frame_flags & FRAMEFLAGS_KEY)
                ? cpi->avg_frame_qindex : cpi->ni_av_qi;

        
        kf_boost = (int)(2 * cpi->output_frame_rate - 16);

        
        kf_boost = kf_boost * kf_boost_qadjustment[Q] / 100;

        
        if (cpi->frames_since_key  < cpi->output_frame_rate / 2)
            kf_boost = (int)(kf_boost
                       * cpi->frames_since_key / (cpi->output_frame_rate / 2));

        if (kf_boost < 16)
            kf_boost = 16;

        target = ((16 + kf_boost) * cpi->per_frame_bandwidth) >> 4;
    }


    if (cpi->oxcf.rc_max_intra_bitrate_pct)
    {
        unsigned int max_rate = cpi->per_frame_bandwidth
                                * cpi->oxcf.rc_max_intra_bitrate_pct / 100;

        if (target > max_rate)
            target = max_rate;
    }

    cpi->this_frame_target = target;

    
    
    if (cpi->pass != 2)
        cpi->active_worst_quality = cpi->worst_quality;

#if 0
    {
        FILE *f;

        f = fopen("kf_boost.stt", "a");
        
        

        fprintf(f, " %8u %10d %10d %10d\n",
                cpi->common.current_video_frame,  cpi->gfu_boost, cpi->baseline_gf_interval, cpi->source_alt_ref_pending);

        fclose(f);
    }
#endif
}



static void calc_gf_params(VP8_COMP *cpi)
{
    int Q = (cpi->oxcf.fixed_q < 0) ? cpi->last_q[INTER_FRAME] : cpi->oxcf.fixed_q;
    int Boost = 0;

    int gf_frame_useage = 0;      
    int tot_mbs = cpi->recent_ref_frame_usage[INTRA_FRAME]  +
                  cpi->recent_ref_frame_usage[LAST_FRAME]   +
                  cpi->recent_ref_frame_usage[GOLDEN_FRAME] +
                  cpi->recent_ref_frame_usage[ALTREF_FRAME];

    int pct_gf_active = (100 * cpi->gf_active_count) / (cpi->common.mb_rows * cpi->common.mb_cols);

    
    

    if (tot_mbs)
        gf_frame_useage = (cpi->recent_ref_frame_usage[GOLDEN_FRAME] + cpi->recent_ref_frame_usage[ALTREF_FRAME]) * 100 / tot_mbs;

    if (pct_gf_active > gf_frame_useage)
        gf_frame_useage = pct_gf_active;

    
    if (cpi->pass != 2)
    {
        
        if (FALSE)
        {
        }

        
        else
        {
#if 0
            
            int index = cpi->one_pass_frame_index;
            int frames_to_scan = (cpi->max_gf_interval <= MAX_LAG_BUFFERS) ? cpi->max_gf_interval : MAX_LAG_BUFFERS;

            








































#else

            
            

            
            Boost = GFQ_ADJUSTMENT;

            
            Boost = Boost * gf_intra_usage_adjustment[(cpi->this_frame_percent_intra < 15) ? cpi->this_frame_percent_intra : 14] / 100;

            
            Boost = Boost * gf_adjust_table[gf_frame_useage] / 100;
#endif
        }

        
        if (!cpi->sf.recode_loop)
        {
            if (cpi->compressor_speed == 2)
                Boost = Boost / 2;
        }

        
        if (Boost > kf_gf_boost_qlimits[Q] && (cpi->pass == 0))
            Boost = kf_gf_boost_qlimits[Q];

        
        else if (Boost < 110)
            Boost = 110;

        
        cpi->last_boost = Boost;

    }

    
    
    if (cpi->oxcf.fixed_q == -1)
    {
        if (cpi->pass == 2)         
        {
            cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;
        }
        else                            
        {
            cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;

            if (cpi->last_boost > 750)
                cpi->frames_till_gf_update_due++;

            if (cpi->last_boost > 1000)
                cpi->frames_till_gf_update_due++;

            if (cpi->last_boost > 1250)
                cpi->frames_till_gf_update_due++;

            if (cpi->last_boost >= 1500)
                cpi->frames_till_gf_update_due ++;

            if (gf_interval_table[gf_frame_useage] > cpi->frames_till_gf_update_due)
                cpi->frames_till_gf_update_due = gf_interval_table[gf_frame_useage];

            if (cpi->frames_till_gf_update_due > cpi->max_gf_interval)
                cpi->frames_till_gf_update_due = cpi->max_gf_interval;
        }
    }
    else
        cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;

    
    if (cpi->pass != 2)
    {
        
        cpi->source_alt_ref_pending = FALSE;

        






    }
}


static void calc_pframe_target_size(VP8_COMP *cpi)
{
    int min_frame_target;
    int Adjustment;

    min_frame_target = 0;

    if (cpi->pass == 2)
    {
        min_frame_target = cpi->min_frame_bandwidth;

        if (min_frame_target < (cpi->av_per_frame_bandwidth >> 5))
            min_frame_target = cpi->av_per_frame_bandwidth >> 5;
    }
    else if (min_frame_target < cpi->per_frame_bandwidth / 4)
        min_frame_target = cpi->per_frame_bandwidth / 4;


    
    if (cpi->common.refresh_alt_ref_frame)
    {
        if (cpi->pass == 2)
        {
            cpi->per_frame_bandwidth = cpi->twopass.gf_bits;                       
            cpi->this_frame_target = cpi->per_frame_bandwidth;
        }

        
        

























































    }

    
    else
    {
        
        if (cpi->pass == 2)
        {
            cpi->this_frame_target = cpi->per_frame_bandwidth;
        }
        
        else
        {
            
            
            if (cpi->kf_overspend_bits > 0)
            {
                Adjustment = (cpi->kf_bitrate_adjustment <= cpi->kf_overspend_bits) ? cpi->kf_bitrate_adjustment : cpi->kf_overspend_bits;

                if (Adjustment > (cpi->per_frame_bandwidth - min_frame_target))
                    Adjustment = (cpi->per_frame_bandwidth - min_frame_target);

                cpi->kf_overspend_bits -= Adjustment;

                
                
                cpi->this_frame_target = cpi->per_frame_bandwidth - Adjustment;

                if (cpi->this_frame_target < min_frame_target)
                    cpi->this_frame_target = min_frame_target;
            }
            else
                cpi->this_frame_target = cpi->per_frame_bandwidth;

            
            if ((cpi->gf_overspend_bits > 0) && (cpi->this_frame_target > min_frame_target))
            {
                int Adjustment = (cpi->non_gf_bitrate_adjustment <= cpi->gf_overspend_bits) ? cpi->non_gf_bitrate_adjustment : cpi->gf_overspend_bits;

                if (Adjustment > (cpi->this_frame_target - min_frame_target))
                    Adjustment = (cpi->this_frame_target - min_frame_target);

                cpi->gf_overspend_bits -= Adjustment;
                cpi->this_frame_target -= Adjustment;
            }

            
            if ((cpi->last_boost > 150) && (cpi->frames_till_gf_update_due > 0) &&
                (cpi->current_gf_interval >= (MIN_GF_INTERVAL << 1)))
            {
                
                Adjustment = (cpi->last_boost - 100) >> 5;

                if (Adjustment < 1)
                    Adjustment = 1;
                else if (Adjustment > 10)
                    Adjustment = 10;

                
                Adjustment = (cpi->this_frame_target * Adjustment) / 100;

                if (Adjustment > (cpi->this_frame_target - min_frame_target))
                    Adjustment = (cpi->this_frame_target - min_frame_target);

                if (cpi->common.frames_since_golden == (cpi->current_gf_interval >> 1))
                    cpi->this_frame_target += ((cpi->current_gf_interval - 1) * Adjustment);
                else
                    cpi->this_frame_target -= Adjustment;
            }
        }
    }

    
    
    
    
    
    if (cpi->this_frame_target < min_frame_target)
        cpi->this_frame_target = min_frame_target;

    if (!cpi->common.refresh_alt_ref_frame)
        
        cpi->inter_frame_target = cpi->this_frame_target;

    
    if (cpi->pass == 0)
    {
        
        if (cpi->buffered_mode)
        {
            int one_percent_bits = 1 + cpi->oxcf.optimal_buffer_level / 100;

            if ((cpi->buffer_level < cpi->oxcf.optimal_buffer_level) ||
                (cpi->bits_off_target < cpi->oxcf.optimal_buffer_level))
            {
                int percent_low = 0;

                
                
                
                
                
                if ((cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER) &&
                    (cpi->buffer_level < cpi->oxcf.optimal_buffer_level))
                {
                    percent_low =
                        (cpi->oxcf.optimal_buffer_level - cpi->buffer_level) /
                        one_percent_bits;
                }
                
                else if (cpi->bits_off_target < 0)
                {
                    
                    percent_low = (int)(100 * -cpi->bits_off_target /
                                       (cpi->total_byte_count * 8));
                }

                if (percent_low > cpi->oxcf.under_shoot_pct)
                    percent_low = cpi->oxcf.under_shoot_pct;
                else if (percent_low < 0)
                    percent_low = 0;

                
                cpi->this_frame_target -= (cpi->this_frame_target * percent_low)
                                          / 200;

                
                
                if (cpi->auto_worst_q)
                {
                    int critical_buffer_level;

                    
                    
                    
                    
                    
                    if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
                    {
                        
                        
                        critical_buffer_level =
                            (cpi->buffer_level < cpi->bits_off_target)
                            ? cpi->buffer_level : cpi->bits_off_target;
                    }
                    
                    
                    else
                    {
                        
                        
                        critical_buffer_level = cpi->bits_off_target;
                    }

                    
                    
                    if (critical_buffer_level < cpi->oxcf.optimal_buffer_level)
                    {
                        if ( critical_buffer_level >
                             (cpi->oxcf.optimal_buffer_level >> 2) )
                        {
                            int64_t qadjustment_range =
                                      cpi->worst_quality - cpi->ni_av_qi;
                            int64_t above_base =
                                      (critical_buffer_level -
                                       (cpi->oxcf.optimal_buffer_level >> 2));

                            
                            
                            
                            
                            
                            
                            cpi->active_worst_quality =
                                cpi->worst_quality -
                                ((qadjustment_range * above_base) /
                                 (cpi->oxcf.optimal_buffer_level*3>>2));
                        }
                        else
                        {
                            cpi->active_worst_quality = cpi->worst_quality;
                        }
                    }
                    else
                    {
                        cpi->active_worst_quality = cpi->ni_av_qi;
                    }
                }
                else
                {
                    cpi->active_worst_quality = cpi->worst_quality;
                }
            }
            else
            {
                int percent_high = 0;

                if ((cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
                     && (cpi->buffer_level > cpi->oxcf.optimal_buffer_level))
                {
                    percent_high = (cpi->buffer_level
                                    - cpi->oxcf.optimal_buffer_level)
                                   / one_percent_bits;
                }
                else if (cpi->bits_off_target > cpi->oxcf.optimal_buffer_level)
                {
                    percent_high = (int)((100 * cpi->bits_off_target)
                                         / (cpi->total_byte_count * 8));
                }

                if (percent_high > cpi->oxcf.over_shoot_pct)
                    percent_high = cpi->oxcf.over_shoot_pct;
                else if (percent_high < 0)
                    percent_high = 0;

                cpi->this_frame_target += (cpi->this_frame_target *
                                           percent_high) / 200;


                
                if (cpi->auto_worst_q)
                {
                    
                    cpi->active_worst_quality = cpi->ni_av_qi;
                }
                else
                {
                    cpi->active_worst_quality = cpi->worst_quality;
                }
            }

            
            cpi->active_best_quality = cpi->best_quality;

            
            if (cpi->active_worst_quality <= cpi->active_best_quality)
                cpi->active_worst_quality = cpi->active_best_quality + 1;

        }
        
        else
        {
            
            cpi->active_worst_quality = cpi->worst_quality;
        }

        
        
        
        if ( cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY &&
             cpi->active_worst_quality < cpi->cq_target_quality)
        {
            cpi->active_worst_quality = cpi->cq_target_quality;
        }
    }

    
    
    
    
    
    if (cpi->drop_frames_allowed && cpi->buffered_mode &&
        (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER) &&
        ((cpi->common.frame_type != KEY_FRAME))) 
    {
        
        if ((cpi->buffer_level < 0))
        {
#if 0
            FILE *f = fopen("dec.stt", "a");
            fprintf(f, "%10d %10d %10d %10d ***** BUFFER EMPTY\n",
                    (int) cpi->common.current_video_frame,
                    cpi->decimation_factor, cpi->common.horiz_scale,
                    (cpi->buffer_level * 100) / cpi->oxcf.optimal_buffer_level);
            fclose(f);
#endif
            

            cpi->drop_frame = TRUE;
        }

#if 0
        
        else if ((cpi->buffer_level < cpi->oxcf.drop_frames_water_mark * cpi->oxcf.optimal_buffer_level / 100) &&
                 (cpi->drop_count < cpi->max_drop_count) && (cpi->pass == 0))
        {
            cpi->drop_frame = TRUE;
        }

#endif

        if (cpi->drop_frame)
        {
            
            cpi->bits_off_target += cpi->av_per_frame_bandwidth;
            cpi->buffer_level = cpi->bits_off_target;
        }
        else
            cpi->drop_count = 0;
    }

    
    if (cpi->oxcf.error_resilient_mode == 0 &&
        (cpi->frames_till_gf_update_due == 0) && !cpi->drop_frame)
    {
        
        int Q = (cpi->oxcf.fixed_q < 0) ? cpi->last_q[INTER_FRAME] : cpi->oxcf.fixed_q;

        int gf_frame_useage = 0;      
        int tot_mbs = cpi->recent_ref_frame_usage[INTRA_FRAME]  +
                      cpi->recent_ref_frame_usage[LAST_FRAME]   +
                      cpi->recent_ref_frame_usage[GOLDEN_FRAME] +
                      cpi->recent_ref_frame_usage[ALTREF_FRAME];

        int pct_gf_active = (100 * cpi->gf_active_count) / (cpi->common.mb_rows * cpi->common.mb_cols);

        
        

        if (tot_mbs)
            gf_frame_useage = (cpi->recent_ref_frame_usage[GOLDEN_FRAME] + cpi->recent_ref_frame_usage[ALTREF_FRAME]) * 100 / tot_mbs;

        if (pct_gf_active > gf_frame_useage)
            gf_frame_useage = pct_gf_active;

        
        if (cpi->auto_gold)
        {
            
            if ((cpi->pass == 0) && (cpi->this_frame_percent_intra < 15 || gf_frame_useage >= 5))
                cpi->common.refresh_golden_frame = TRUE;

            
            else if (cpi->pass == 2)
                cpi->common.refresh_golden_frame = TRUE;
        }

#if 0

        
        if (0)
        {
            FILE *f;

            f = fopen("gf_useaget.stt", "a");
            fprintf(f, " %8ld %10ld %10ld %10ld %10ld\n",
                    cpi->common.current_video_frame,  cpi->gfu_boost, GFQ_ADJUSTMENT, cpi->gfu_boost, gf_frame_useage);
            fclose(f);
        }

#endif

        if (cpi->common.refresh_golden_frame == TRUE)
        {
#if 0

            if (0)   
            {
                FILE *f;

                f = fopen("GFexit.stt", "a");
                fprintf(f, "%8ld GF coded\n", cpi->common.current_video_frame);
                fclose(f);
            }

#endif

            if (cpi->auto_adjust_gold_quantizer)
            {
                calc_gf_params(cpi);
            }

            
            
            
            if (!cpi->source_alt_ref_active)
            {
                if (cpi->oxcf.fixed_q < 0)
                {
                    if (cpi->pass == 2)
                    {
                        cpi->this_frame_target = cpi->per_frame_bandwidth;          
                    }
                    else
                    {
                        int Boost = cpi->last_boost;
                        int frames_in_section = cpi->frames_till_gf_update_due + 1;
                        int allocation_chunks = (frames_in_section * 100) + (Boost - 100);
                        int bits_in_section = cpi->inter_frame_target * frames_in_section;

                        
                        while (Boost > 1000)
                        {
                            Boost /= 2;
                            allocation_chunks /= 2;
                        }

                        
                        if ((bits_in_section >> 7) > allocation_chunks)
                            cpi->this_frame_target = Boost * (bits_in_section / allocation_chunks);
                        else
                            cpi->this_frame_target = (Boost * bits_in_section) / allocation_chunks;
                    }
                }
                else
                    cpi->this_frame_target =
                        (estimate_bits_at_q(1, Q, cpi->common.MBs, 1.0)
                         * cpi->last_boost) / 100;

            }
            
            
            
            
            else
            {
                cpi->this_frame_target = 0;
            }

            cpi->current_gf_interval = cpi->frames_till_gf_update_due;

        }
    }
}


void vp8_update_rate_correction_factors(VP8_COMP *cpi, int damp_var)
{
    int    Q = cpi->common.base_qindex;
    int    correction_factor = 100;
    double rate_correction_factor;
    double adjustment_limit;

    int    projected_size_based_on_q = 0;

    
    vp8_clear_system_state();  

    if (cpi->common.frame_type == KEY_FRAME)
    {
        rate_correction_factor = cpi->key_frame_rate_correction_factor;
    }
    else
    {
        if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
            rate_correction_factor = cpi->gf_rate_correction_factor;
        else
            rate_correction_factor = cpi->rate_correction_factor;
    }

    
    
    
    projected_size_based_on_q = (int)(((.5 + rate_correction_factor * vp8_bits_per_mb[cpi->common.frame_type][Q]) * cpi->common.MBs) / (1 << BPER_MB_NORMBITS));

    
    if (cpi->zbin_over_quant > 0)
    {
        int Z = cpi->zbin_over_quant;
        double Factor = 0.99;
        double factor_adjustment = 0.01 / 256.0; 

        while (Z > 0)
        {
            Z --;
            projected_size_based_on_q =
                (int)(Factor * projected_size_based_on_q);
            Factor += factor_adjustment;

            if (Factor  >= 0.999)
                Factor = 0.999;
        }
    }

    
    
    
    if (projected_size_based_on_q > 0)
        correction_factor = (100 * cpi->projected_frame_size) / projected_size_based_on_q;

    
    switch (damp_var)
    {
    case 0:
        adjustment_limit = 0.75;
        break;
    case 1:
        adjustment_limit = 0.375;
        break;
    case 2:
    default:
        adjustment_limit = 0.25;
        break;
    }

    
    if (correction_factor > 102)
    {
        
        correction_factor = (int)(100.5 + ((correction_factor - 100) * adjustment_limit));
        rate_correction_factor = ((rate_correction_factor * correction_factor) / 100);

        
        if (rate_correction_factor > MAX_BPB_FACTOR)
            rate_correction_factor = MAX_BPB_FACTOR;
    }
    
    else if (correction_factor < 99)
    {
        
        correction_factor = (int)(100.5 - ((100 - correction_factor) * adjustment_limit));
        rate_correction_factor = ((rate_correction_factor * correction_factor) / 100);

        
        if (rate_correction_factor < MIN_BPB_FACTOR)
            rate_correction_factor = MIN_BPB_FACTOR;
    }

    if (cpi->common.frame_type == KEY_FRAME)
        cpi->key_frame_rate_correction_factor = rate_correction_factor;
    else
    {
        if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
            cpi->gf_rate_correction_factor = rate_correction_factor;
        else
            cpi->rate_correction_factor = rate_correction_factor;
    }
}


int vp8_regulate_q(VP8_COMP *cpi, int target_bits_per_frame)
{
    int Q = cpi->active_worst_quality;

    
    cpi->zbin_over_quant = 0;

    if (cpi->oxcf.fixed_q >= 0)
    {
        Q = cpi->oxcf.fixed_q;

        if (cpi->common.frame_type == KEY_FRAME)
        {
            Q = cpi->oxcf.key_q;
        }
        else if (cpi->common.refresh_alt_ref_frame)
        {
            Q = cpi->oxcf.alt_q;
        }
        else if (cpi->common.refresh_golden_frame)
        {
            Q = cpi->oxcf.gold_q;
        }

    }
    else
    {
        int i;
        int last_error = INT_MAX;
        int target_bits_per_mb;
        int bits_per_mb_at_this_q;
        double correction_factor;

        
        if (cpi->common.frame_type == KEY_FRAME)
            correction_factor = cpi->key_frame_rate_correction_factor;
        else
        {
            if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
                correction_factor = cpi->gf_rate_correction_factor;
            else
                correction_factor = cpi->rate_correction_factor;
        }

        
        if (target_bits_per_frame >= (INT_MAX >> BPER_MB_NORMBITS))
            target_bits_per_mb = (target_bits_per_frame / cpi->common.MBs) << BPER_MB_NORMBITS;       
        else
            target_bits_per_mb = (target_bits_per_frame << BPER_MB_NORMBITS) / cpi->common.MBs;

        i = cpi->active_best_quality;

        do
        {
            bits_per_mb_at_this_q = (int)(.5 + correction_factor * vp8_bits_per_mb[cpi->common.frame_type][i]);

            if (bits_per_mb_at_this_q <= target_bits_per_mb)
            {
                if ((target_bits_per_mb - bits_per_mb_at_this_q) <= last_error)
                    Q = i;
                else
                    Q = i - 1;

                break;
            }
            else
                last_error = bits_per_mb_at_this_q - target_bits_per_mb;
        }
        while (++i <= cpi->active_worst_quality);


        
        
        if (Q >= MAXQ)
        {
            int zbin_oqmax;

            double Factor = 0.99;
            double factor_adjustment = 0.01 / 256.0; 

            if (cpi->common.frame_type == KEY_FRAME)
                zbin_oqmax = 0; 
            else if (cpi->common.refresh_alt_ref_frame || (cpi->common.refresh_golden_frame && !cpi->source_alt_ref_active))
                zbin_oqmax = 16;
            else
                zbin_oqmax = ZBIN_OQ_MAX;

            













            
            
            
            
            while (cpi->zbin_over_quant < zbin_oqmax)
            {
                cpi->zbin_over_quant ++;

                if (cpi->zbin_over_quant > zbin_oqmax)
                    cpi->zbin_over_quant = zbin_oqmax;

                
                bits_per_mb_at_this_q = (int)(Factor * bits_per_mb_at_this_q);
                Factor += factor_adjustment;

                if (Factor  >= 0.999)
                    Factor = 0.999;

                if (bits_per_mb_at_this_q <= target_bits_per_mb)    
                    break;
            }

        }
    }

    return Q;
}


static int estimate_keyframe_frequency(VP8_COMP *cpi)
{
    int i;

    
    int av_key_frame_frequency = 0;

    


    if (cpi->key_frame_count == 1)
    {
        


        int key_freq = cpi->oxcf.key_freq>0 ? cpi->oxcf.key_freq : 1;
        av_key_frame_frequency = (int)cpi->output_frame_rate * 2;

        if (cpi->oxcf.auto_key && av_key_frame_frequency > key_freq)
            av_key_frame_frequency = cpi->oxcf.key_freq;

        cpi->prior_key_frame_distance[KEY_FRAME_CONTEXT - 1]
            = av_key_frame_frequency;
    }
    else
    {
        unsigned int total_weight = 0;
        int last_kf_interval =
                (cpi->frames_since_key > 0) ? cpi->frames_since_key : 1;

        


        for (i = 0; i < KEY_FRAME_CONTEXT; i++)
        {
            if (i < KEY_FRAME_CONTEXT - 1)
                cpi->prior_key_frame_distance[i]
                    = cpi->prior_key_frame_distance[i+1];
            else
                cpi->prior_key_frame_distance[i] = last_kf_interval;

            av_key_frame_frequency += prior_key_frame_weight[i]
                                      * cpi->prior_key_frame_distance[i];
            total_weight += prior_key_frame_weight[i];
        }

        av_key_frame_frequency  /= total_weight;

    }
    return av_key_frame_frequency;
}


void vp8_adjust_key_frame_context(VP8_COMP *cpi)
{
    
    vp8_clear_system_state();

    
    
    if ((cpi->pass != 2)
         && (cpi->projected_frame_size > cpi->per_frame_bandwidth))
    {
        int overspend;

        





        overspend = (cpi->projected_frame_size - cpi->per_frame_bandwidth);
        cpi->kf_overspend_bits += overspend * 7 / 8;
        cpi->gf_overspend_bits += overspend * 1 / 8;

        
        cpi->kf_bitrate_adjustment = cpi->kf_overspend_bits
                                     / estimate_keyframe_frequency(cpi);
    }

    cpi->frames_since_key = 0;
    cpi->key_frame_count++;
}


void vp8_compute_frame_size_bounds(VP8_COMP *cpi, int *frame_under_shoot_limit, int *frame_over_shoot_limit)
{
    
    if (cpi->oxcf.fixed_q >= 0)
    {
        
        *frame_under_shoot_limit = 0;
        *frame_over_shoot_limit  = INT_MAX;
    }
    else
    {
        if (cpi->common.frame_type == KEY_FRAME)
        {
            *frame_over_shoot_limit  = cpi->this_frame_target * 9 / 8;
            *frame_under_shoot_limit = cpi->this_frame_target * 7 / 8;
        }
        else
        {
            if (cpi->common.refresh_alt_ref_frame || cpi->common.refresh_golden_frame)
            {
                *frame_over_shoot_limit  = cpi->this_frame_target * 9 / 8;
                *frame_under_shoot_limit = cpi->this_frame_target * 7 / 8;
            }
            else
            {
                
                if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
                {
                    if (cpi->buffer_level >= ((cpi->oxcf.optimal_buffer_level + cpi->oxcf.maximum_buffer_size) >> 1))
                    {
                        
                        *frame_over_shoot_limit  = cpi->this_frame_target * 12 / 8;
                        *frame_under_shoot_limit = cpi->this_frame_target * 6 / 8;
                    }
                    else if (cpi->buffer_level <= (cpi->oxcf.optimal_buffer_level >> 1))
                    {
                        
                        *frame_over_shoot_limit  = cpi->this_frame_target * 10 / 8;
                        *frame_under_shoot_limit = cpi->this_frame_target * 4 / 8;
                    }
                    else
                    {
                        *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
                        *frame_under_shoot_limit = cpi->this_frame_target * 5 / 8;
                    }
                }
                
                
                else
                {
                    
                    if (cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY)
                    {
                        *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
                        *frame_under_shoot_limit = cpi->this_frame_target * 2 / 8;
                    }
                    else
                    {
                        *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
                        *frame_under_shoot_limit = cpi->this_frame_target * 5 / 8;
                    }
                }
            }
        }
    }
}



int vp8_pick_frame_size(VP8_COMP *cpi)
{
    VP8_COMMON *cm = &cpi->common;

    if (cm->frame_type == KEY_FRAME)
        calc_iframe_target_size(cpi);
    else
    {
        calc_pframe_target_size(cpi);

        
        if (cpi->drop_frame)
        {
            cpi->drop_frame = FALSE;
            cpi->drop_count++;
            return 0;
        }
    }
    return 1;
}
