










#include "vp8/common/onyxc_int.h"
#if CONFIG_POSTPROC
#include "vp8/common/postproc.h"
#endif
#include "vp8/common/onyxd.h"
#include "onyxd_int.h"
#include "vpx_mem/vpx_mem.h"
#include "vp8/common/alloccommon.h"
#include "vp8/common/loopfilter.h"
#include "vp8/common/swapyv12buffer.h"
#include "vp8/common/threading.h"
#include "decoderthreading.h"
#include <stdio.h>
#include <assert.h>

#include "vp8/common/quant_common.h"
#include "./vpx_scale_rtcd.h"
#include "vpx_scale/vpx_scale.h"
#include "vp8/common/systemdependent.h"
#include "vpx_ports/vpx_timer.h"
#include "detokenize.h"
#if CONFIG_ERROR_CONCEALMENT
#include "error_concealment.h"
#endif
#if ARCH_ARM
#include "vpx_ports/arm.h"
#endif

extern void vp8_init_loop_filter(VP8_COMMON *cm);
extern void vp8cx_init_de_quantizer(VP8D_COMP *pbi);
static int get_free_fb (VP8_COMMON *cm);
static void ref_cnt_fb (int *buf, int *idx, int new_idx);

static void remove_decompressor(VP8D_COMP *pbi)
{
#if CONFIG_ERROR_CONCEALMENT
    vp8_de_alloc_overlap_lists(pbi);
#endif
    vp8_remove_common(&pbi->common);
    vpx_free(pbi);
}

static struct VP8D_COMP * create_decompressor(VP8D_CONFIG *oxcf)
{
    VP8D_COMP *pbi = vpx_memalign(32, sizeof(VP8D_COMP));

    if (!pbi)
        return NULL;

    vpx_memset(pbi, 0, sizeof(VP8D_COMP));

    if (setjmp(pbi->common.error.jmp))
    {
        pbi->common.error.setjmp = 0;
        remove_decompressor(pbi);
        return 0;
    }

    pbi->common.error.setjmp = 1;

    vp8_create_common(&pbi->common);

    pbi->common.current_video_frame = 0;
    pbi->ready_for_new_data = 1;

    


    vp8cx_init_de_quantizer(pbi);

    vp8_loop_filter_init(&pbi->common);

    pbi->common.error.setjmp = 0;

#if CONFIG_ERROR_CONCEALMENT
    pbi->ec_enabled = oxcf->error_concealment;
    pbi->overlaps = NULL;
#else
    pbi->ec_enabled = 0;
#endif
    


    pbi->ec_active = 0;

    pbi->decoded_key_frame = 0;

    



    pbi->independent_partitions = 0;

    vp8_setup_block_dptrs(&pbi->mb);

    return pbi;
}

vpx_codec_err_t vp8dx_get_reference(VP8D_COMP *pbi, enum vpx_ref_frame_type ref_frame_flag, YV12_BUFFER_CONFIG *sd)
{
    VP8_COMMON *cm = &pbi->common;
    int ref_fb_idx;

    if (ref_frame_flag == VP8_LAST_FRAME)
        ref_fb_idx = cm->lst_fb_idx;
    else if (ref_frame_flag == VP8_GOLD_FRAME)
        ref_fb_idx = cm->gld_fb_idx;
    else if (ref_frame_flag == VP8_ALTR_FRAME)
        ref_fb_idx = cm->alt_fb_idx;
    else{
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
            "Invalid reference frame");
        return pbi->common.error.error_code;
    }

    if(cm->yv12_fb[ref_fb_idx].y_height != sd->y_height ||
        cm->yv12_fb[ref_fb_idx].y_width != sd->y_width ||
        cm->yv12_fb[ref_fb_idx].uv_height != sd->uv_height ||
        cm->yv12_fb[ref_fb_idx].uv_width != sd->uv_width){
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
            "Incorrect buffer dimensions");
    }
    else
        vp8_yv12_copy_frame(&cm->yv12_fb[ref_fb_idx], sd);

    return pbi->common.error.error_code;
}


vpx_codec_err_t vp8dx_set_reference(VP8D_COMP *pbi, enum vpx_ref_frame_type ref_frame_flag, YV12_BUFFER_CONFIG *sd)
{
    VP8_COMMON *cm = &pbi->common;
    int *ref_fb_ptr = NULL;
    int free_fb;

    if (ref_frame_flag == VP8_LAST_FRAME)
        ref_fb_ptr = &cm->lst_fb_idx;
    else if (ref_frame_flag == VP8_GOLD_FRAME)
        ref_fb_ptr = &cm->gld_fb_idx;
    else if (ref_frame_flag == VP8_ALTR_FRAME)
        ref_fb_ptr = &cm->alt_fb_idx;
    else{
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
            "Invalid reference frame");
        return pbi->common.error.error_code;
    }

    if(cm->yv12_fb[*ref_fb_ptr].y_height != sd->y_height ||
        cm->yv12_fb[*ref_fb_ptr].y_width != sd->y_width ||
        cm->yv12_fb[*ref_fb_ptr].uv_height != sd->uv_height ||
        cm->yv12_fb[*ref_fb_ptr].uv_width != sd->uv_width){
        vpx_internal_error(&pbi->common.error, VPX_CODEC_ERROR,
            "Incorrect buffer dimensions");
    }
    else{
        
        free_fb = get_free_fb(cm);
        

        cm->fb_idx_ref_cnt[free_fb]--;

        
        ref_cnt_fb (cm->fb_idx_ref_cnt, ref_fb_ptr, free_fb);
        vp8_yv12_copy_frame(sd, &cm->yv12_fb[*ref_fb_ptr]);
    }

   return pbi->common.error.error_code;
}


#if HAVE_NEON
extern void vp8_push_neon(int64_t *store);
extern void vp8_pop_neon(int64_t *store);
#endif

static int get_free_fb (VP8_COMMON *cm)
{
    int i;
    for (i = 0; i < NUM_YV12_BUFFERS; i++)
        if (cm->fb_idx_ref_cnt[i] == 0)
            break;

    assert(i < NUM_YV12_BUFFERS);
    cm->fb_idx_ref_cnt[i] = 1;
    return i;
}

static void ref_cnt_fb (int *buf, int *idx, int new_idx)
{
    if (buf[*idx] > 0)
        buf[*idx]--;

    *idx = new_idx;

    buf[new_idx]++;
}


static int swap_frame_buffers (VP8_COMMON *cm)
{
    int err = 0;

    




    if (cm->copy_buffer_to_arf)
    {
        int new_fb = 0;

        if (cm->copy_buffer_to_arf == 1)
            new_fb = cm->lst_fb_idx;
        else if (cm->copy_buffer_to_arf == 2)
            new_fb = cm->gld_fb_idx;
        else
            err = -1;

        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->alt_fb_idx, new_fb);
    }

    if (cm->copy_buffer_to_gf)
    {
        int new_fb = 0;

        if (cm->copy_buffer_to_gf == 1)
            new_fb = cm->lst_fb_idx;
        else if (cm->copy_buffer_to_gf == 2)
            new_fb = cm->alt_fb_idx;
        else
            err = -1;

        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->gld_fb_idx, new_fb);
    }

    if (cm->refresh_golden_frame)
        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->gld_fb_idx, cm->new_fb_idx);

    if (cm->refresh_alt_ref_frame)
        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->alt_fb_idx, cm->new_fb_idx);

    if (cm->refresh_last_frame)
    {
        ref_cnt_fb (cm->fb_idx_ref_cnt, &cm->lst_fb_idx, cm->new_fb_idx);

        cm->frame_to_show = &cm->yv12_fb[cm->lst_fb_idx];
    }
    else
        cm->frame_to_show = &cm->yv12_fb[cm->new_fb_idx];

    cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

    return err;
}

int check_fragments_for_errors(VP8D_COMP *pbi)
{
    if (!pbi->ec_active &&
        pbi->fragments.count <= 1 && pbi->fragments.sizes[0] == 0)
    {
        VP8_COMMON *cm = &pbi->common;

        


        if (cm->fb_idx_ref_cnt[cm->lst_fb_idx] > 1)
        {
            



            const int prev_idx = cm->lst_fb_idx;
            cm->fb_idx_ref_cnt[prev_idx]--;
            cm->lst_fb_idx = get_free_fb(cm);
            vp8_yv12_copy_frame(&cm->yv12_fb[prev_idx],
                                    &cm->yv12_fb[cm->lst_fb_idx]);
        }
        




        cm->yv12_fb[cm->lst_fb_idx].corrupted = 1;

        
        cm->show_frame = 0;

        
        return 0;
    }

    return 1;
}

int vp8dx_receive_compressed_data(VP8D_COMP *pbi, size_t size,
                                  const uint8_t *source,
                                  int64_t time_stamp)
{
#if HAVE_NEON
    int64_t dx_store_reg[8];
#endif
    VP8_COMMON *cm = &pbi->common;
    int retcode = -1;

    pbi->common.error.error_code = VPX_CODEC_OK;

    retcode = check_fragments_for_errors(pbi);
    if(retcode <= 0)
        return retcode;

#if HAVE_NEON
#if CONFIG_RUNTIME_CPU_DETECT
    if (cm->cpu_caps & HAS_NEON)
#endif
    {
        vp8_push_neon(dx_store_reg);
    }
#endif

    cm->new_fb_idx = get_free_fb (cm);

    
    pbi->dec_fb_ref[INTRA_FRAME]  = &cm->yv12_fb[cm->new_fb_idx];
    pbi->dec_fb_ref[LAST_FRAME]   = &cm->yv12_fb[cm->lst_fb_idx];
    pbi->dec_fb_ref[GOLDEN_FRAME] = &cm->yv12_fb[cm->gld_fb_idx];
    pbi->dec_fb_ref[ALTREF_FRAME] = &cm->yv12_fb[cm->alt_fb_idx];

    if (setjmp(pbi->common.error.jmp))
    {
       



        cm->yv12_fb[cm->lst_fb_idx].corrupted = 1;

        if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
          cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

        goto decode_exit;
    }

    pbi->common.error.setjmp = 1;

    retcode = vp8_decode_frame(pbi);

    if (retcode < 0)
    {
        if (cm->fb_idx_ref_cnt[cm->new_fb_idx] > 0)
          cm->fb_idx_ref_cnt[cm->new_fb_idx]--;

        pbi->common.error.error_code = VPX_CODEC_ERROR;
        goto decode_exit;
    }

    if (swap_frame_buffers (cm))
    {
        pbi->common.error.error_code = VPX_CODEC_ERROR;
        goto decode_exit;
    }

    vp8_clear_system_state();

    if (cm->show_frame)
    {
        cm->current_video_frame++;
        cm->show_frame_mi = cm->mi;
    }

    #if CONFIG_ERROR_CONCEALMENT
    
    if (pbi->ec_enabled && pbi->common.prev_mi)
    {
        MODE_INFO* tmp = pbi->common.prev_mi;
        int row, col;
        pbi->common.prev_mi = pbi->common.mi;
        pbi->common.mi = tmp;

        
        for (row = 0; row < pbi->common.mb_rows; ++row)
        {
            for (col = 0; col < pbi->common.mb_cols; ++col)
            {
                const int i = row*pbi->common.mode_info_stride + col;
                pbi->common.mi[i].mbmi.segment_id =
                        pbi->common.prev_mi[i].mbmi.segment_id;
            }
        }
    }
#endif

    pbi->ready_for_new_data = 0;
    pbi->last_time_stamp = time_stamp;

decode_exit:
#if HAVE_NEON
#if CONFIG_RUNTIME_CPU_DETECT
    if (cm->cpu_caps & HAS_NEON)
#endif
    {
        vp8_pop_neon(dx_store_reg);
    }
#endif

    pbi->common.error.setjmp = 0;
    return retcode;
}
int vp8dx_get_raw_frame(VP8D_COMP *pbi, YV12_BUFFER_CONFIG *sd, int64_t *time_stamp, int64_t *time_end_stamp, vp8_ppflags_t *flags)
{
    int ret = -1;

    if (pbi->ready_for_new_data == 1)
        return ret;

    
    if (pbi->common.show_frame == 0)
        return ret;

    pbi->ready_for_new_data = 1;
    *time_stamp = pbi->last_time_stamp;
    *time_end_stamp = 0;

#if CONFIG_POSTPROC
    ret = vp8_post_proc_frame(&pbi->common, sd, flags);
#else

    if (pbi->common.frame_to_show)
    {
        *sd = *pbi->common.frame_to_show;
        sd->y_width = pbi->common.Width;
        sd->y_height = pbi->common.Height;
        sd->uv_height = pbi->common.Height / 2;
        ret = 0;
    }
    else
    {
        ret = -1;
    }

#endif 
    vp8_clear_system_state();
    return ret;
}






int vp8dx_references_buffer( VP8_COMMON *oci, int ref_frame )
{
    const MODE_INFO *mi = oci->mi;
    int mb_row, mb_col;

    for (mb_row = 0; mb_row < oci->mb_rows; mb_row++)
    {
        for (mb_col = 0; mb_col < oci->mb_cols; mb_col++,mi++)
        {
            if( mi->mbmi.ref_frame == ref_frame)
              return 1;
        }
        mi++;
    }
    return 0;

}

int vp8_create_decoder_instances(struct frame_buffers *fb, VP8D_CONFIG *oxcf)
{
    if(!fb->use_frame_threads)
    {
        
        fb->pbi[0] = create_decompressor(oxcf);
        if(!fb->pbi[0])
            return VPX_CODEC_ERROR;

#if CONFIG_MULTITHREAD
        

        fb->pbi[0]->max_threads = oxcf->max_threads;
        vp8_decoder_create_threads(fb->pbi[0]);
#endif
    }
    else
    {
        

    }

    return VPX_CODEC_OK;
}

int vp8_remove_decoder_instances(struct frame_buffers *fb)
{
    if(!fb->use_frame_threads)
    {
        VP8D_COMP *pbi = fb->pbi[0];

        if (!pbi)
            return VPX_CODEC_ERROR;
#if CONFIG_MULTITHREAD
        if (pbi->b_multithreaded_rd)
            vp8mt_de_alloc_temp_buffers(pbi, pbi->common.mb_rows);
        vp8_decoder_remove_threads(pbi);
#endif

        
        remove_decompressor(pbi);
    }
    else
    {
        

    }

    return VPX_CODEC_OK;
}
