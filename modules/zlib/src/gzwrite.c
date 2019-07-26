




#include "gzguts.h"


local int gz_init OF((gz_statep));
local int gz_comp OF((gz_statep, int));
local int gz_zero OF((gz_statep, z_off64_t));



local int gz_init(state)
    gz_statep state;
{
    int ret;
    z_streamp strm = &(state->strm);

    
    state->in = (unsigned char *)malloc(state->want);
    if (state->in == NULL) {
        gz_error(state, Z_MEM_ERROR, "out of memory");
        return -1;
    }

    
    if (!state->direct) {
        
        state->out = (unsigned char *)malloc(state->want);
        if (state->out == NULL) {
            free(state->in);
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }

        
        strm->zalloc = Z_NULL;
        strm->zfree = Z_NULL;
        strm->opaque = Z_NULL;
        ret = deflateInit2(strm, state->level, Z_DEFLATED,
                           MAX_WBITS + 16, DEF_MEM_LEVEL, state->strategy);
        if (ret != Z_OK) {
            free(state->out);
            free(state->in);
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }
    }

    
    state->size = state->want;

    
    if (!state->direct) {
        strm->avail_out = state->size;
        strm->next_out = state->out;
        state->x.next = strm->next_out;
    }
    return 0;
}







local int gz_comp(state, flush)
    gz_statep state;
    int flush;
{
    int ret, got;
    unsigned have;
    z_streamp strm = &(state->strm);

    
    if (state->size == 0 && gz_init(state) == -1)
        return -1;

    
    if (state->direct) {
        got = write(state->fd, strm->next_in, strm->avail_in);
        if (got < 0 || (unsigned)got != strm->avail_in) {
            gz_error(state, Z_ERRNO, zstrerror());
            return -1;
        }
        strm->avail_in = 0;
        return 0;
    }

    
    ret = Z_OK;
    do {
        

        if (strm->avail_out == 0 || (flush != Z_NO_FLUSH &&
            (flush != Z_FINISH || ret == Z_STREAM_END))) {
            have = (unsigned)(strm->next_out - state->x.next);
            if (have && ((got = write(state->fd, state->x.next, have)) < 0 ||
                         (unsigned)got != have)) {
                gz_error(state, Z_ERRNO, zstrerror());
                return -1;
            }
            if (strm->avail_out == 0) {
                strm->avail_out = state->size;
                strm->next_out = state->out;
            }
            state->x.next = strm->next_out;
        }

        
        have = strm->avail_out;
        ret = deflate(strm, flush);
        if (ret == Z_STREAM_ERROR) {
            gz_error(state, Z_STREAM_ERROR,
                      "internal error: deflate stream corrupt");
            return -1;
        }
        have -= strm->avail_out;
    } while (have);

    
    if (flush == Z_FINISH)
        deflateReset(strm);

    
    return 0;
}


local int gz_zero(state, len)
    gz_statep state;
    z_off64_t len;
{
    int first;
    unsigned n;
    z_streamp strm = &(state->strm);

    
    if (strm->avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
        return -1;

    
    first = 1;
    while (len) {
        n = GT_OFF(state->size) || (z_off64_t)state->size > len ?
            (unsigned)len : state->size;
        if (first) {
            memset(state->in, 0, n);
            first = 0;
        }
        strm->avail_in = n;
        strm->next_in = state->in;
        state->x.pos += n;
        if (gz_comp(state, Z_NO_FLUSH) == -1)
            return -1;
        len -= n;
    }
    return 0;
}


int ZEXPORT gzwrite(file, buf, len)
    gzFile file;
    voidpc buf;
    unsigned len;
{
    unsigned put = len;
    gz_statep state;
    z_streamp strm;

    
    if (file == NULL)
        return 0;
    state = (gz_statep)file;
    strm = &(state->strm);

    
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return 0;

    

    if ((int)len < 0) {
        gz_error(state, Z_DATA_ERROR, "requested length does not fit in int");
        return 0;
    }

    
    if (len == 0)
        return 0;

    
    if (state->size == 0 && gz_init(state) == -1)
        return 0;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return 0;
    }

    
    if (len < state->size) {
        
        do {
            unsigned have, copy;

            if (strm->avail_in == 0)
                strm->next_in = state->in;
            have = (unsigned)((strm->next_in + strm->avail_in) - state->in);
            copy = state->size - have;
            if (copy > len)
                copy = len;
            memcpy(state->in + have, buf, copy);
            strm->avail_in += copy;
            state->x.pos += copy;
            buf = (const char *)buf + copy;
            len -= copy;
            if (len && gz_comp(state, Z_NO_FLUSH) == -1)
                return 0;
        } while (len);
    }
    else {
        
        if (strm->avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
            return 0;

        
        strm->avail_in = len;
        strm->next_in = (z_const Bytef *)buf;
        state->x.pos += len;
        if (gz_comp(state, Z_NO_FLUSH) == -1)
            return 0;
    }

    
    return (int)put;
}


int ZEXPORT gzputc(file, c)
    gzFile file;
    int c;
{
    unsigned have;
    unsigned char buf[1];
    gz_statep state;
    z_streamp strm;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    strm = &(state->strm);

    
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return -1;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return -1;
    }

    

    if (state->size) {
        if (strm->avail_in == 0)
            strm->next_in = state->in;
        have = (unsigned)((strm->next_in + strm->avail_in) - state->in);
        if (have < state->size) {
            state->in[have] = c;
            strm->avail_in++;
            state->x.pos++;
            return c & 0xff;
        }
    }

    
    buf[0] = c;
    if (gzwrite(file, buf, 1) != 1)
        return -1;
    return c & 0xff;
}


int ZEXPORT gzputs(file, str)
    gzFile file;
    const char *str;
{
    int ret;
    unsigned len;

    
    len = (unsigned)strlen(str);
    ret = gzwrite(file, str, len);
    return ret == 0 && len != 0 ? -1 : ret;
}

#if defined(STDC) || defined(Z_HAVE_STDARG_H)
#include <stdarg.h>


int ZEXPORTVA gzvprintf(gzFile file, const char *format, va_list va)
{
    int size, len;
    gz_statep state;
    z_streamp strm;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    strm = &(state->strm);

    
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return 0;

    
    if (state->size == 0 && gz_init(state) == -1)
        return 0;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return 0;
    }

    
    if (strm->avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
        return 0;

    
    size = (int)(state->size);
    state->in[size - 1] = 0;
#ifdef NO_vsnprintf
#  ifdef HAS_vsprintf_void
    (void)vsprintf((char *)(state->in), format, va);
    for (len = 0; len < size; len++)
        if (state->in[len] == 0) break;
#  else
    len = vsprintf((char *)(state->in), format, va);
#  endif
#else
#  ifdef HAS_vsnprintf_void
    (void)vsnprintf((char *)(state->in), size, format, va);
    len = strlen((char *)(state->in));
#  else
    len = vsnprintf((char *)(state->in), size, format, va);
#  endif
#endif

    
    if (len <= 0 || len >= (int)size || state->in[size - 1] != 0)
        return 0;

    
    strm->avail_in = (unsigned)len;
    strm->next_in = state->in;
    state->x.pos += len;
    return len;
}

int ZEXPORTVA gzprintf(gzFile file, const char *format, ...)
{
    va_list va;
    int ret;

    va_start(va, format);
    ret = gzvprintf(file, format, va);
    va_end(va);
    return ret;
}

#else 


int ZEXPORTVA gzprintf (file, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
                       a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
    gzFile file;
    const char *format;
    int a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
        a11, a12, a13, a14, a15, a16, a17, a18, a19, a20;
{
    int size, len;
    gz_statep state;
    z_streamp strm;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    strm = &(state->strm);

    
    if (sizeof(int) != sizeof(void *))
        return 0;

    
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return 0;

    
    if (state->size == 0 && gz_init(state) == -1)
        return 0;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return 0;
    }

    
    if (strm->avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
        return 0;

    
    size = (int)(state->size);
    state->in[size - 1] = 0;
#ifdef NO_snprintf
#  ifdef HAS_sprintf_void
    sprintf((char *)(state->in), format, a1, a2, a3, a4, a5, a6, a7, a8,
            a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    for (len = 0; len < size; len++)
        if (state->in[len] == 0) break;
#  else
    len = sprintf((char *)(state->in), format, a1, a2, a3, a4, a5, a6, a7, a8,
                  a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
#  endif
#else
#  ifdef HAS_snprintf_void
    snprintf((char *)(state->in), size, format, a1, a2, a3, a4, a5, a6, a7, a8,
             a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    len = strlen((char *)(state->in));
#  else
    len = snprintf((char *)(state->in), size, format, a1, a2, a3, a4, a5, a6,
                   a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18,
                   a19, a20);
#  endif
#endif

    
    if (len <= 0 || len >= (int)size || state->in[size - 1] != 0)
        return 0;

    
    strm->avail_in = (unsigned)len;
    strm->next_in = state->in;
    state->x.pos += len;
    return len;
}

#endif


int ZEXPORT gzflush(file, flush)
    gzFile file;
    int flush;
{
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return Z_STREAM_ERROR;

    
    if (flush < 0 || flush > Z_FINISH)
        return Z_STREAM_ERROR;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return -1;
    }

    
    gz_comp(state, flush);
    return state->err;
}


int ZEXPORT gzsetparams(file, level, strategy)
    gzFile file;
    int level;
    int strategy;
{
    gz_statep state;
    z_streamp strm;

    
    if (file == NULL)
        return Z_STREAM_ERROR;
    state = (gz_statep)file;
    strm = &(state->strm);

    
    if (state->mode != GZ_WRITE || state->err != Z_OK)
        return Z_STREAM_ERROR;

    
    if (level == state->level && strategy == state->strategy)
        return Z_OK;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            return -1;
    }

    
    if (state->size) {
        
        if (strm->avail_in && gz_comp(state, Z_PARTIAL_FLUSH) == -1)
            return state->err;
        deflateParams(strm, level, strategy);
    }
    state->level = level;
    state->strategy = strategy;
    return Z_OK;
}


int ZEXPORT gzclose_w(file)
    gzFile file;
{
    int ret = Z_OK;
    gz_statep state;

    
    if (file == NULL)
        return Z_STREAM_ERROR;
    state = (gz_statep)file;

    
    if (state->mode != GZ_WRITE)
        return Z_STREAM_ERROR;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_zero(state, state->skip) == -1)
            ret = state->err;
    }

    
    if (gz_comp(state, Z_FINISH) == -1)
        ret = state->err;
    if (state->size) {
        if (!state->direct) {
            (void)deflateEnd(&(state->strm));
            free(state->out);
        }
        free(state->in);
    }
    gz_error(state, Z_OK, NULL);
    free(state->path);
    if (close(state->fd) == -1)
        ret = Z_ERRNO;
    free(state);
    return ret;
}
