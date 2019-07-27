




















































































#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>


#include "mdef.h"


#define MODEL_DEF_VERSION	"0.3"

static void
ciphone_add(mdef_t * m, char *ci, int p)
{
    assert(p < m->n_ciphone);

    m->ciphone[p].name = (char *) ckd_salloc(ci);       
    if (hash_table_enter(m->ciphone_ht, m->ciphone[p].name,
                         (void *)(long)p) != (void *)(long)p)
        E_FATAL("hash_table_enter(%s) failed; duplicate CIphone?\n",
                m->ciphone[p].name);
}


static ph_lc_t *
find_ph_lc(ph_lc_t * lclist, int lc)
{
    ph_lc_t *lcptr;

    for (lcptr = lclist; lcptr && (lcptr->lc != lc); lcptr = lcptr->next);
    return lcptr;
}


static ph_rc_t *
find_ph_rc(ph_rc_t * rclist, int rc)
{
    ph_rc_t *rcptr;

    for (rcptr = rclist; rcptr && (rcptr->rc != rc); rcptr = rcptr->next);
    return rcptr;
}


static void
triphone_add(mdef_t * m,
             int ci, int lc, int rc, word_posn_t wpos,
             int p)
{
    ph_lc_t *lcptr;
    ph_rc_t *rcptr;

    assert(p < m->n_phone);

    
    m->phone[p].ci = ci;
    m->phone[p].lc = lc;
    m->phone[p].rc = rc;
    m->phone[p].wpos = wpos;

    
    if (p >= m->n_ciphone) {
        if ((lcptr = find_ph_lc(m->wpos_ci_lclist[wpos][(int) ci], lc))
            == NULL) {
            lcptr = (ph_lc_t *) ckd_calloc(1, sizeof(ph_lc_t)); 
            lcptr->lc = lc;
            lcptr->next = m->wpos_ci_lclist[wpos][(int) ci];
            m->wpos_ci_lclist[wpos][(int) ci] = lcptr;  
        }
        if ((rcptr = find_ph_rc(lcptr->rclist, rc)) != NULL) {
            __BIGSTACKVARIABLE__ char buf[4096];

            mdef_phone_str(m, rcptr->pid, buf);
            E_FATAL("Duplicate triphone: %s\n", buf);
        }

        rcptr = (ph_rc_t *) ckd_calloc(1, sizeof(ph_rc_t));     
        rcptr->rc = rc;
        rcptr->pid = p;
        rcptr->next = lcptr->rclist;
        lcptr->rclist = rcptr;
    }
}


int
mdef_ciphone_id(mdef_t * m, char *ci)
{
    int32 id;
    if (hash_table_lookup_int32(m->ciphone_ht, ci, &id) < 0)
        return -1;
    return id;
}


const char *
mdef_ciphone_str(mdef_t * m, int id)
{
    assert(m);
    assert((id >= 0) && (id < m->n_ciphone));

    return (m->ciphone[id].name);
}


int
mdef_phone_str(mdef_t * m, int pid, char *buf)
{
    char *wpos_name;

    assert(m);
    assert((pid >= 0) && (pid < m->n_phone));
    wpos_name = WPOS_NAME;

    buf[0] = '\0';
    if (pid < m->n_ciphone)
        sprintf(buf, "%s", mdef_ciphone_str(m, pid));
    else {
        sprintf(buf, "%s %s %s %c",
                mdef_ciphone_str(m, m->phone[pid].ci),
                mdef_ciphone_str(m, m->phone[pid].lc),
                mdef_ciphone_str(m, m->phone[pid].rc),
                wpos_name[m->phone[pid].wpos]);
    }
    return 0;
}


int
mdef_phone_id(mdef_t * m,
              int ci, int lc, int rc, word_posn_t wpos)
{
    ph_lc_t *lcptr;
    ph_rc_t *rcptr;
    int newl, newr;

    assert(m);
    assert((ci >= 0) && (ci < m->n_ciphone));
    assert((lc >= 0) && (lc < m->n_ciphone));
    assert((rc >= 0) && (rc < m->n_ciphone));
    assert((wpos >= 0) && (wpos < N_WORD_POSN));

    if (((lcptr =
          find_ph_lc(m->wpos_ci_lclist[wpos][(int) ci], lc)) == NULL)
        || ((rcptr = find_ph_rc(lcptr->rclist, rc)) == NULL)) {
        
        if (m->sil < 0)
            return -1;

        newl = m->ciphone[(int) lc].filler ? m->sil : lc;
        newr = m->ciphone[(int) rc].filler ? m->sil : rc;
        if ((newl == lc) && (newr == rc))
            return -1;

        return (mdef_phone_id(m, ci, newl, newr, wpos));
    }

    return (rcptr->pid);
}

int
mdef_is_ciphone(mdef_t * m, int p)
{
    assert(m);
    assert((p >= 0) && (p < m->n_phone));

    return ((p < m->n_ciphone) ? 1 : 0);
}

int
mdef_is_cisenone(mdef_t * m, int s)
{
    assert(m);
    if (s >= m->n_sen) {
        return 0;
    }
    assert(s >= 0);
    return ((s == m->cd2cisen[s]) ? 1 : 0);
}



static void
parse_tmat_senmap(mdef_t * m, char *line, long off, int p)
{
    int32 wlen, n, s;
    char *lp;
    __BIGSTACKVARIABLE__ char word[1024];

    lp = line + off;

    
    if ((sscanf(lp, "%d%n", &n, &wlen) != 1) || (n < 0))
        E_FATAL("Missing or bad transition matrix id: %s\n", line);
    m->phone[p].tmat = n;
    if (m->n_tmat <= n)
        E_FATAL("tmat-id(%d) > #tmat in header(%d): %s\n", n, m->n_tmat,
                line);
    lp += wlen;

    
    for (n = 0; n < m->n_emit_state; n++) {
        if ((sscanf(lp, "%d%n", &s, &wlen) != 1) || (s < 0))
            E_FATAL("Missing or bad state[%d]->senone mapping: %s\n", n,
                    line);

        if ((p < m->n_ciphone) && (m->n_ci_sen <= s))
            E_FATAL("CI-senone-id(%d) > #CI-senones(%d): %s\n", s,
                    m->n_ci_sen, line);
        if (m->n_sen <= s)
            E_FATAL("Senone-id(%d) > #senones(%d): %s\n", s, m->n_sen,
                    line);

        m->sseq[p][n] = s;
        lp += wlen;
    }

    
    if ((sscanf(lp, "%s%n", word, &wlen) != 1) || (strcmp(word, "N") != 0))
        E_FATAL("Missing non-emitting state spec: %s\n", line);
    lp += wlen;

    
    if (sscanf(lp, "%s%n", word, &wlen) == 1)
        E_FATAL("Non-empty beyond non-emitting final state: %s\n", line);
}


static void
parse_base_line(mdef_t * m, char *line, int p)
{
    int32 wlen, n;
    __BIGSTACKVARIABLE__ char word[1024], *lp;
    int ci;

    lp = line;

    
    if (sscanf(lp, "%s%n", word, &wlen) != 1)
        E_FATAL("Missing base phone name: %s\n", line);
    lp += wlen;

    
    ci = mdef_ciphone_id(m, word);
    if (ci >= 0)
        E_FATAL("Duplicate base phone: %s\n", line);

    
    ciphone_add(m, word, p);
    ci = (int) p;

    
    for (n = 0; n < 3; n++) {
        if ((sscanf(lp, "%s%n", word, &wlen) != 1)
            || (strcmp(word, "-") != 0))
            E_FATAL("Bad context info for base phone: %s\n", line);
        lp += wlen;
    }

    
    if (sscanf(lp, "%s%n", word, &wlen) != 1)
        E_FATAL("Missing filler atribute field: %s\n", line);
    lp += wlen;
    if (strcmp(word, "filler") == 0)
        m->ciphone[(int) ci].filler = 1;
    else if (strcmp(word, "n/a") == 0)
        m->ciphone[(int) ci].filler = 0;
    else
        E_FATAL("Bad filler attribute field: %s\n", line);

    triphone_add(m, ci, -1, -1, WORD_POSN_UNDEFINED, p);

    
    parse_tmat_senmap(m, line, lp - line, p);
}


static void
parse_tri_line(mdef_t * m, char *line, int p)
{
    int32 wlen;
    __BIGSTACKVARIABLE__ char word[1024], *lp;
    int ci, lc, rc;
    word_posn_t wpos = WORD_POSN_BEGIN;

    lp = line;

    
    if (sscanf(lp, "%s%n", word, &wlen) != 1)
        E_FATAL("Missing base phone name: %s\n", line);
    lp += wlen;

    ci = mdef_ciphone_id(m, word);
    if (ci < 0)
        E_FATAL("Unknown base phone: %s\n", line);

    
    if (sscanf(lp, "%s%n", word, &wlen) != 1)
        E_FATAL("Missing left context: %s\n", line);
    lp += wlen;
    lc = mdef_ciphone_id(m, word);
    if (lc < 0)
        E_FATAL("Unknown left context: %s\n", line);

    
    if (sscanf(lp, "%s%n", word, &wlen) != 1)
        E_FATAL("Missing right context: %s\n", line);
    lp += wlen;
    rc = mdef_ciphone_id(m, word);
    if (rc < 0)
        E_FATAL("Unknown right  context: %s\n", line);

    
    if ((sscanf(lp, "%s%n", word, &wlen) != 1) || (word[1] != '\0'))
        E_FATAL("Missing or bad word-position spec: %s\n", line);
    lp += wlen;
    switch (word[0]) {
    case 'b':
        wpos = WORD_POSN_BEGIN;
        break;
    case 'e':
        wpos = WORD_POSN_END;
        break;
    case 's':
        wpos = WORD_POSN_SINGLE;
        break;
    case 'i':
        wpos = WORD_POSN_INTERNAL;
        break;
    default:
        E_FATAL("Bad word-position spec: %s\n", line);
    }

    
    if (sscanf(lp, "%s%n", word, &wlen) != 1)
        E_FATAL("Missing filler attribute field: %s\n", line);
    lp += wlen;
    if (((strcmp(word, "filler") == 0) && (m->ciphone[(int) ci].filler)) ||
        ((strcmp(word, "n/a") == 0) && (!m->ciphone[(int) ci].filler))) {
        
    }
    else
        E_FATAL("Bad filler attribute field: %s\n", line);

    triphone_add(m, ci, lc, rc, wpos, p);

    
    parse_tmat_senmap(m, line, lp - line, p);
}


static void
sseq_compress(mdef_t * m)
{
    hash_table_t *h;
    uint16 **sseq;
    int32 n_sseq;
    int32 p, j, k;
    glist_t g;
    gnode_t *gn;
    hash_entry_t *he;

    k = m->n_emit_state * sizeof(int16);

    h = hash_table_new(m->n_phone, HASH_CASE_YES);
    n_sseq = 0;

    
    for (p = 0; p < m->n_phone; p++) {
        
	if (n_sseq
            == (j = hash_table_enter_bkey_int32(h, (char *)m->sseq[p], k, n_sseq)))
            n_sseq++;

        m->phone[p].ssid = j;
    }

    
    sseq = ckd_calloc_2d(n_sseq, m->n_emit_state, sizeof(**sseq)); 

    g = hash_table_tolist(h, &j);
    assert(j == n_sseq);

    for (gn = g; gn; gn = gnode_next(gn)) {
        he = (hash_entry_t *) gnode_ptr(gn);
        j = (int32)(long)hash_entry_val(he);
        memcpy(sseq[j], hash_entry_key(he), k);
    }
    glist_free(g);

    
    ckd_free_2d(m->sseq);
    m->sseq = sseq;
    m->n_sseq = n_sseq;

    hash_table_free(h);
}


static int32
noncomment_line(char *line, int32 size, FILE * fp)
{
    while (fgets(line, size, fp) != NULL) {
        if (line[0] != '#')
            return 0;
    }
    return -1;
}





mdef_t *
mdef_init(char *mdeffile, int32 breport)
{
    FILE *fp;
    int32 n_ci, n_tri, n_map, n;
    __BIGSTACKVARIABLE__ char tag[1024], buf[1024];
    uint16 **senmap;
    int p;
    int32 s, ci, cd;
    mdef_t *m;

    if (!mdeffile)
        E_FATAL("No mdef-file\n");

    if (breport)
        E_INFO("Reading model definition: %s\n", mdeffile);

    m = (mdef_t *) ckd_calloc(1, sizeof(mdef_t));       

    if ((fp = fopen(mdeffile, "r")) == NULL)
        E_FATAL_SYSTEM("Failed to open mdef file '%s' for reading", mdeffile);

    if (noncomment_line(buf, sizeof(buf), fp) < 0)
        E_FATAL("Empty file: %s\n", mdeffile);

    if (strncmp(buf, "BMDF", 4) == 0 || strncmp(buf, "FDMB", 4) == 0) {
        E_INFO
            ("Found byte-order mark %.4s, assuming this is a binary mdef file\n",
             buf);
        fclose(fp);
        ckd_free(m);
        return NULL;
    }
    if (strncmp(buf, MODEL_DEF_VERSION, strlen(MODEL_DEF_VERSION)) != 0)
        E_FATAL("Version error: Expecing %s, but read %s\n",
                MODEL_DEF_VERSION, buf);

    
    n_ci = -1;
    n_tri = -1;
    n_map = -1;
    m->n_ci_sen = -1;
    m->n_sen = -1;
    m->n_tmat = -1;
    do {
        if (noncomment_line(buf, sizeof(buf), fp) < 0)
            E_FATAL("Incomplete header\n");

        if ((sscanf(buf, "%d %s", &n, tag) != 2) || (n < 0))
            E_FATAL("Error in header: %s\n", buf);

        if (strcmp(tag, "n_base") == 0)
            n_ci = n;
        else if (strcmp(tag, "n_tri") == 0)
            n_tri = n;
        else if (strcmp(tag, "n_state_map") == 0)
            n_map = n;
        else if (strcmp(tag, "n_tied_ci_state") == 0)
            m->n_ci_sen = n;
        else if (strcmp(tag, "n_tied_state") == 0)
            m->n_sen = n;
        else if (strcmp(tag, "n_tied_tmat") == 0)
            m->n_tmat = n;
        else
            E_FATAL("Unknown header line: %s\n", buf);
    } while ((n_ci < 0) || (n_tri < 0) || (n_map < 0) ||
             (m->n_ci_sen < 0) || (m->n_sen < 0) || (m->n_tmat < 0));

    if ((n_ci == 0) || (m->n_ci_sen == 0) || (m->n_tmat == 0)
        || (m->n_ci_sen > m->n_sen))
        E_FATAL("%s: Error in header\n", mdeffile);

    
    if (n_ci >= MAX_INT16)
        E_FATAL("%s: #CI phones (%d) exceeds limit (%d)\n", mdeffile, n_ci,
                MAX_INT16);
    if (n_ci + n_tri >= MAX_INT32) 
        E_FATAL("%s: #Phones (%d) exceeds limit (%d)\n", mdeffile,
                n_ci + n_tri, MAX_INT32);
    if (m->n_sen >= MAX_INT16)
        E_FATAL("%s: #senones (%d) exceeds limit (%d)\n", mdeffile,
                m->n_sen, MAX_INT16);
    if (m->n_tmat >= MAX_INT32) 
        E_FATAL("%s: #tmats (%d) exceeds limit (%d)\n", mdeffile,
                m->n_tmat, MAX_INT32);

    m->n_emit_state = (n_map / (n_ci + n_tri)) - 1;
    if ((m->n_emit_state + 1) * (n_ci + n_tri) != n_map)
        E_FATAL
            ("Header error: n_state_map not a multiple of n_ci*n_tri\n");

    
    m->n_ciphone = n_ci;
    m->ciphone_ht = hash_table_new(n_ci, HASH_CASE_YES);  
    m->ciphone = (ciphone_t *) ckd_calloc(n_ci, sizeof(ciphone_t));     

    
    m->n_phone = n_ci + n_tri;
    m->phone = (phone_t *) ckd_calloc(m->n_phone, sizeof(phone_t));     

    
    senmap = ckd_calloc_2d(m->n_phone, m->n_emit_state, sizeof(**senmap));      
    m->sseq = senmap;           

    
    m->wpos_ci_lclist = (ph_lc_t ***) ckd_calloc_2d(N_WORD_POSN, m->n_ciphone, sizeof(ph_lc_t *));      

    




    
    for (p = 0; p < n_ci; p++) {
        if (noncomment_line(buf, sizeof(buf), fp) < 0)
            E_FATAL("Premature EOF reading CIphone %d\n", p);
        parse_base_line(m, buf, p);
    }
    m->sil = mdef_ciphone_id(m, S3_SILENCE_CIPHONE);

    
    for (; p < m->n_phone; p++) {
        if (noncomment_line(buf, sizeof(buf), fp) < 0)
            E_FATAL("Premature EOF reading phone %d\n", p);
        parse_tri_line(m, buf, p);
    }

    if (noncomment_line(buf, sizeof(buf), fp) >= 0)
        E_ERROR("Non-empty file beyond expected #phones (%d)\n",
                m->n_phone);

    
    if (m->n_ciphone * m->n_emit_state != m->n_ci_sen)
        E_FATAL
            ("#CI-senones(%d) != #CI-phone(%d) x #emitting-states(%d)\n",
             m->n_ci_sen, m->n_ciphone, m->n_emit_state);
    m->cd2cisen = (int16 *) ckd_calloc(m->n_sen, sizeof(*m->cd2cisen)); 

    m->sen2cimap = (int16 *) ckd_calloc(m->n_sen, sizeof(*m->sen2cimap)); 

    for (s = 0; s < m->n_sen; s++)
        m->sen2cimap[s] = -1;
    for (s = 0; s < m->n_ci_sen; s++) { 
        m->cd2cisen[s] = s;
        m->sen2cimap[s] = s / m->n_emit_state;
    }
    for (p = n_ci; p < m->n_phone; p++) {       
        for (s = 0; s < m->n_emit_state; s++) {
            cd = m->sseq[p][s];
            ci = m->sseq[m->phone[p].ci][s];
            m->cd2cisen[cd] = ci;
            m->sen2cimap[cd] = m->phone[p].ci;
        }
    }

    sseq_compress(m);
    fclose(fp);

    return m;
}

void
mdef_report(mdef_t * m)
{
    E_INFO_NOFN("Initialization of mdef_t, report:\n");
    E_INFO_NOFN
        ("%d CI-phone, %d CD-phone, %d emitstate/phone, %d CI-sen, %d Sen, %d Sen-Seq\n",
         m->n_ciphone, m->n_phone - m->n_ciphone, m->n_emit_state,
         m->n_ci_sen, m->n_sen, m->n_sseq);
    E_INFO_NOFN("\n");

}












void
mdef_free_recursive_lc(ph_lc_t * lc)
{
    if (lc == NULL)
        return;

    if (lc->rclist)
        mdef_free_recursive_rc(lc->rclist);

    if (lc->next)
        mdef_free_recursive_lc(lc->next);

    ckd_free((void *) lc);
}

void
mdef_free_recursive_rc(ph_rc_t * rc)
{
    if (rc == NULL)
        return;

    if (rc->next)
        mdef_free_recursive_rc(rc->next);

    ckd_free((void *) rc);
}






void
mdef_free(mdef_t * m)
{
    int i, j;

    if (m) {
        if (m->sen2cimap)
            ckd_free((void *) m->sen2cimap);
        if (m->cd2cisen)
            ckd_free((void *) m->cd2cisen);

        
        for (i = 0; i < N_WORD_POSN; i++)
            for (j = 0; j < m->n_ciphone; j++)
                if (m->wpos_ci_lclist[i][j]) {
                    mdef_free_recursive_lc(m->wpos_ci_lclist[i][j]->next);
                    mdef_free_recursive_rc(m->wpos_ci_lclist[i][j]->
                                           rclist);
                }

        for (i = 0; i < N_WORD_POSN; i++)
            for (j = 0; j < m->n_ciphone; j++)
                if (m->wpos_ci_lclist[i][j])
                    ckd_free((void *) m->wpos_ci_lclist[i][j]);


        if (m->wpos_ci_lclist)
            ckd_free_2d((void *) m->wpos_ci_lclist);
        if (m->sseq)
            ckd_free_2d((void *) m->sseq);
        
        if (m->phone)
            ckd_free((void *) m->phone);
        if (m->ciphone_ht)
            hash_table_free(m->ciphone_ht);

        for (i = 0; i < m->n_ciphone; i++) {
            if (m->ciphone[i].name)
                ckd_free((void *) m->ciphone[i].name);
        }


        if (m->ciphone)
            ckd_free((void *) m->ciphone);

        ckd_free((void *) m);
    }
}
