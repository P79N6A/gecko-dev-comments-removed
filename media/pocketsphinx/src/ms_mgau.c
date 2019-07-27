






































































#include "ms_mgau.h"

static ps_mgaufuncs_t ms_mgau_funcs = {
    "ms",
    ms_cont_mgau_frame_eval, 
    ms_mgau_mllr_transform,  
    ms_mgau_free             
};

ps_mgau_t *
ms_mgau_init(acmod_t *acmod, logmath_t *lmath, bin_mdef_t *mdef)
{
    
    ms_mgau_model_t *msg;
    ps_mgau_t *mg;
    gauden_t *g;
    senone_t *s;
    cmd_ln_t *config;
    int i;

    config = acmod->config;

    msg = (ms_mgau_model_t *) ckd_calloc(1, sizeof(ms_mgau_model_t));
    msg->config = config;
    msg->g = NULL;
    msg->s = NULL;
    
    g = msg->g = gauden_init(cmd_ln_str_r(config, "-mean"),
                             cmd_ln_str_r(config, "-var"),
                             cmd_ln_float32_r(config, "-varfloor"),
                             lmath);

    
    if (g->n_feat != feat_dimension1(acmod->fcb)) {
        E_ERROR("Number of streams does not match: %d != %d\n",
                g->n_feat, feat_dimension1(acmod->fcb));
        goto error_out;
    }
    for (i = 0; i < g->n_feat; ++i) {
        if (g->featlen[i] != feat_dimension2(acmod->fcb, i)) {
            E_ERROR("Dimension of stream %d does not match: %d != %d\n", i,
                    g->featlen[i], feat_dimension2(acmod->fcb, i));
            goto error_out;
        }
    }

    s = msg->s = senone_init(msg->g,
                             cmd_ln_str_r(config, "-mixw"),
                             cmd_ln_str_r(config, "-senmgau"),
                             cmd_ln_float32_r(config, "-mixwfloor"),
                             lmath, mdef);

    s->aw = cmd_ln_int32_r(config, "-aw");

    
    if (s->n_feat != g->n_feat)
        E_FATAL("#Feature mismatch: gauden= %d, senone= %d\n", g->n_feat,
                s->n_feat);
    if (s->n_cw != g->n_density)
        E_FATAL("#Densities mismatch: gauden= %d, senone= %d\n",
                g->n_density, s->n_cw);
    if (s->n_gauden > g->n_mgau)
        E_FATAL("Senones need more codebooks (%d) than present (%d)\n",
                s->n_gauden, g->n_mgau);
    if (s->n_gauden < g->n_mgau)
        E_ERROR("Senones use fewer codebooks (%d) than present (%d)\n",
                s->n_gauden, g->n_mgau);

    msg->topn = cmd_ln_int32_r(config, "-topn");
    E_INFO("The value of topn: %d\n", msg->topn);
    if (msg->topn == 0 || msg->topn > msg->g->n_density) {
        E_WARN
            ("-topn argument (%d) invalid or > #density codewords (%d); set to latter\n",
             msg->topn, msg->g->n_density);
        msg->topn = msg->g->n_density;
    }

    msg->dist = (gauden_dist_t ***)
        ckd_calloc_3d(g->n_mgau, g->n_feat, msg->topn,
                      sizeof(gauden_dist_t));
    msg->mgau_active = ckd_calloc(g->n_mgau, sizeof(int8));

    mg = (ps_mgau_t *)msg;
    mg->vt = &ms_mgau_funcs;
    return mg;
error_out:
    ms_mgau_free(ps_mgau_base(msg));
    return NULL;    
}

void
ms_mgau_free(ps_mgau_t * mg)
{
    ms_mgau_model_t *msg = (ms_mgau_model_t *)mg;
    if (msg == NULL)
        return;

    if (msg->g)
	gauden_free(msg->g);
    if (msg->s)
        senone_free(msg->s);
    if (msg->dist)
        ckd_free_3d((void *) msg->dist);
    if (msg->mgau_active)
        ckd_free(msg->mgau_active);
    
    ckd_free(msg);
}

int
ms_mgau_mllr_transform(ps_mgau_t *s,
		       ps_mllr_t *mllr)
{
    ms_mgau_model_t *msg = (ms_mgau_model_t *)s;
    return gauden_mllr_transform(msg->g, mllr, msg->config);
}

int32
ms_cont_mgau_frame_eval(ps_mgau_t * mg,
			int16 *senscr,
			uint8 *senone_active,
			int32 n_senone_active,
                        mfcc_t ** feat,
			int32 frame,
			int32 compallsen)
{
    ms_mgau_model_t *msg = (ms_mgau_model_t *)mg;
    int32 gid;
    int32 topn;
    int32 best;
    gauden_t *g;
    senone_t *sen;

    topn = ms_mgau_topn(msg);
    g = ms_mgau_gauden(msg);
    sen = ms_mgau_senone(msg);

    if (compallsen) {
	int32 s;

	for (gid = 0; gid < g->n_mgau; gid++)
	    gauden_dist(g, gid, topn, feat, msg->dist[gid]);

	best = (int32) 0x7fffffff;
	for (s = 0; s < sen->n_sen; s++) {
	    senscr[s] = senone_eval(sen, s, msg->dist[sen->mgau[s]], topn);
	    if (best > senscr[s]) {
		best = senscr[s];
	    }
	}

	
	for (s = 0; s < sen->n_sen; s++) {
	    int32 bs = senscr[s] - best;
	    if (bs > 32767)
		bs = 32767;
	    if (bs < -32768)
		bs = -32768;
	    senscr[s] = bs;
	}
    }
    else {
	int32 i, n;
	
	for (gid = 0; gid < g->n_mgau; gid++)
	    msg->mgau_active[gid] = 0;

	n = 0;
	for (i = 0; i < n_senone_active; i++) {
	    
	    int32 s = senone_active[i] + n;
	    msg->mgau_active[sen->mgau[s]] = 1;
	    n = s;
	}

	
	for (gid = 0; gid < g->n_mgau; gid++) {
	    if (msg->mgau_active[gid])
		gauden_dist(g, gid, topn, feat, msg->dist[gid]);
	}

	best = (int32) 0x7fffffff;
	n = 0;
	for (i = 0; i < n_senone_active; i++) {
	    int32 s = senone_active[i] + n;
	    senscr[s] = senone_eval(sen, s, msg->dist[sen->mgau[s]], topn);
	    if (best > senscr[s]) {
		best = senscr[s];
	    }
	    n = s;
	}

	
	n = 0;
	for (i = 0; i < n_senone_active; i++) {
	    int32 s = senone_active[i] + n;
	    int32 bs = senscr[s] - best;
	    if (bs > 32767)
		bs = 32767;
	    if (bs < -32768)
		bs = -32768;
	    senscr[s] = bs;
	    n = s;
	}
    }

    return 0;
}
