




#include "gzguts.h"


local int gz_load OF((gz_statep, unsigned char *, unsigned, unsigned *));
local int gz_avail OF((gz_statep));
local int gz_next4 OF((gz_statep, unsigned long *));
local int gz_head OF((gz_statep));
local int gz_decomp OF((gz_statep));
local int gz_make OF((gz_statep));
local int gz_skip OF((gz_statep, z_off64_t));





local int gz_load(state, buf, len, have)
    gz_statep state;
    unsigned char *buf;
    unsigned len;
    unsigned *have;
{
    int ret;

    *have = 0;
    do {
        ret = read(state->fd, buf + *have, len - *have);
        if (ret <= 0)
            break;
        *have += ret;
    } while (*have < len);
    if (ret < 0) {
        gz_error(state, Z_ERRNO, zstrerror());
        return -1;
    }
    if (ret == 0)
        state->eof = 1;
    return 0;
}






local int gz_avail(state)
    gz_statep state;
{
    z_streamp strm = &(state->strm);

    if (state->err != Z_OK)
        return -1;
    if (state->eof == 0) {
        if (gz_load(state, state->in, state->size,
                (unsigned *)&(strm->avail_in)) == -1)
            return -1;
        strm->next_in = state->in;
    }
    return 0;
}


#define NEXT() ((strm->avail_in == 0 && gz_avail(state) == -1) ? -1 : \
                (strm->avail_in == 0 ? -1 : \
                 (strm->avail_in--, *(strm->next_in)++)))



local int gz_next4(state, ret)
    gz_statep state;
    unsigned long *ret;
{
    int ch;
    unsigned long val;
    z_streamp strm = &(state->strm);

    val = NEXT();
    val += (unsigned)NEXT() << 8;
    val += (unsigned long)NEXT() << 16;
    ch = NEXT();
    if (ch == -1)
        return -1;
    val += (unsigned long)ch << 24;
    *ret = val;
    return 0;
}













local int gz_head(state)
    gz_statep state;
{
    z_streamp strm = &(state->strm);
    int flags;
    unsigned len;

    
    if (state->size == 0) {
        
        state->in = malloc(state->want);
        state->out = malloc(state->want << 1);
        if (state->in == NULL || state->out == NULL) {
            if (state->out != NULL)
                free(state->out);
            if (state->in != NULL)
                free(state->in);
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }
        state->size = state->want;

        
        state->strm.zalloc = Z_NULL;
        state->strm.zfree = Z_NULL;
        state->strm.opaque = Z_NULL;
        state->strm.avail_in = 0;
        state->strm.next_in = Z_NULL;
        if (inflateInit2(&(state->strm), -15) != Z_OK) {    
            free(state->out);
            free(state->in);
            state->size = 0;
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }
    }

    
    if (strm->avail_in == 0) {
        if (gz_avail(state) == -1)
            return -1;
        if (strm->avail_in == 0)
            return 0;
    }

    
    if (strm->next_in[0] == 31) {
        strm->avail_in--;
        strm->next_in++;
        if (strm->avail_in == 0 && gz_avail(state) == -1)
            return -1;
        if (strm->avail_in && strm->next_in[0] == 139) {
            
            strm->avail_in--;
            strm->next_in++;

            
            if (NEXT() != 8) {      
                gz_error(state, Z_DATA_ERROR, "unknown compression method");
                return -1;
            }
            flags = NEXT();
            if (flags & 0xe0) {     
                gz_error(state, Z_DATA_ERROR, "unknown header flags set");
                return -1;
            }
            NEXT();                 
            NEXT();
            NEXT();
            NEXT();
            NEXT();                 
            NEXT();                 
            if (flags & 4) {        
                len = (unsigned)NEXT();
                len += (unsigned)NEXT() << 8;
                while (len--)
                    if (NEXT() < 0)
                        break;
            }
            if (flags & 8)          
                while (NEXT() > 0)
                    ;
            if (flags & 16)         
                while (NEXT() > 0)
                    ;
            if (flags & 2) {        
                NEXT();
                NEXT();
            }
            


            
            inflateReset(strm);
            strm->adler = crc32(0L, Z_NULL, 0);
            state->how = GZIP;
            state->direct = 0;
            return 0;
        }
        else {
            
            state->out[0] = 31;
            state->have = 1;
        }
    }

    


    state->raw = state->pos;
    state->next = state->out;
    if (strm->avail_in) {
        memcpy(state->next + state->have, strm->next_in, strm->avail_in);
        state->have += strm->avail_in;
        strm->avail_in = 0;
    }
    state->how = COPY;
    state->direct = 1;
    return 0;
}









local int gz_decomp(state)
    gz_statep state;
{
    int ret;
    unsigned had;
    unsigned long crc, len;
    z_streamp strm = &(state->strm);

    
    had = strm->avail_out;
    do {
        
        if (strm->avail_in == 0 && gz_avail(state) == -1)
            return -1;
        if (strm->avail_in == 0) {
            gz_error(state, Z_DATA_ERROR, "unexpected end of file");
            return -1;
        }

        
        ret = inflate(strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT) {
            gz_error(state, Z_STREAM_ERROR,
                      "internal error: inflate stream corrupt");
            return -1;
        }
        if (ret == Z_MEM_ERROR) {
            gz_error(state, Z_MEM_ERROR, "out of memory");
            return -1;
        }
        if (ret == Z_DATA_ERROR) {              
            gz_error(state, Z_DATA_ERROR,
                      strm->msg == NULL ? "compressed data error" : strm->msg);
            return -1;
        }
    } while (strm->avail_out && ret != Z_STREAM_END);

    
    state->have = had - strm->avail_out;
    state->next = strm->next_out - state->have;
    strm->adler = crc32(strm->adler, state->next, state->have);

    
    if (ret == Z_STREAM_END) {
        if (gz_next4(state, &crc) == -1 || gz_next4(state, &len) == -1) {
            gz_error(state, Z_DATA_ERROR, "unexpected end of file");
            return -1;
        }
        if (crc != strm->adler) {
            gz_error(state, Z_DATA_ERROR, "incorrect data check");
            return -1;
        }
        if (len != (strm->total_out & 0xffffffffL)) {
            gz_error(state, Z_DATA_ERROR, "incorrect length check");
            return -1;
        }
        state->how = LOOK;      

    }

    
    return 0;
}








local int gz_make(state)
    gz_statep state;
{
    z_streamp strm = &(state->strm);

    if (state->how == LOOK) {           
        if (gz_head(state) == -1)
            return -1;
        if (state->have)                
            return 0;
    }
    if (state->how == COPY) {           
        if (gz_load(state, state->out, state->size << 1, &(state->have)) == -1)
            return -1;
        state->next = state->out;
    }
    else if (state->how == GZIP) {      
        strm->avail_out = state->size << 1;
        strm->next_out = state->out;
        if (gz_decomp(state) == -1)
            return -1;
    }
    return 0;
}


local int gz_skip(state, len)
    gz_statep state;
    z_off64_t len;
{
    unsigned n;

    
    while (len)
        
        if (state->have) {
            n = GT_OFF(state->have) || (z_off64_t)state->have > len ?
                (unsigned)len : state->have;
            state->have -= n;
            state->next += n;
            state->pos += n;
            len -= n;
        }

        
        else if (state->eof && state->strm.avail_in == 0)
            break;

        
        else {
            
            if (gz_make(state) == -1)
                return -1;
        }
    return 0;
}


int ZEXPORT gzread(file, buf, len)
    gzFile file;
    voidp buf;
    unsigned len;
{
    unsigned got, n;
    gz_statep state;
    z_streamp strm;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    strm = &(state->strm);

    
    if (state->mode != GZ_READ || state->err != Z_OK)
        return -1;

    

    if ((int)len < 0) {
        gz_error(state, Z_BUF_ERROR, "requested length does not fit in int");
        return -1;
    }

    
    if (len == 0)
        return 0;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_skip(state, state->skip) == -1)
            return -1;
    }

    
    got = 0;
    do {
        
        if (state->have) {
            n = state->have > len ? len : state->have;
            memcpy(buf, state->next, n);
            state->next += n;
            state->have -= n;
        }

        
        else if (state->eof && strm->avail_in == 0)
            break;

        

        else if (state->how == LOOK || len < (state->size << 1)) {
            
            if (gz_make(state) == -1)
                return -1;
            continue;       
            

        }

        
        else if (state->how == COPY) {      
            if (gz_load(state, buf, len, &n) == -1)
                return -1;
        }

        
        else {  
            strm->avail_out = len;
            strm->next_out = buf;
            if (gz_decomp(state) == -1)
                return -1;
            n = state->have;
            state->have = 0;
        }

        
        len -= n;
        buf = (char *)buf + n;
        got += n;
        state->pos += n;
    } while (len);

    
    return (int)got;
}


int ZEXPORT gzgetc(file)
    gzFile file;
{
    int ret;
    unsigned char buf[1];
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    
    if (state->mode != GZ_READ || state->err != Z_OK)
        return -1;

    
    if (state->have) {
        state->have--;
        state->pos++;
        return *(state->next)++;
    }

    
    ret = gzread(file, buf, 1);
    return ret < 1 ? -1 : buf[0];
}


int ZEXPORT gzungetc(c, file)
    int c;
    gzFile file;
{
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    
    if (state->mode != GZ_READ || state->err != Z_OK)
        return -1;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_skip(state, state->skip) == -1)
            return -1;
    }

    
    if (c < 0)
        return -1;

    
    if (state->have == 0) {
        state->have = 1;
        state->next = state->out + (state->size << 1) - 1;
        state->next[0] = c;
        state->pos--;
        return c;
    }

    
    if (state->have == (state->size << 1)) {
        gz_error(state, Z_BUF_ERROR, "out of room to push characters");
        return -1;
    }

    
    if (state->next == state->out) {
        unsigned char *src = state->out + state->have;
        unsigned char *dest = state->out + (state->size << 1);
        while (src > state->out)
            *--dest = *--src;
        state->next = dest;
    }
    state->have++;
    state->next--;
    state->next[0] = c;
    state->pos--;
    return c;
}


char * ZEXPORT gzgets(file, buf, len)
    gzFile file;
    char *buf;
    int len;
{
    unsigned left, n;
    char *str;
    unsigned char *eol;
    gz_statep state;

    
    if (file == NULL || buf == NULL || len < 1)
        return NULL;
    state = (gz_statep)file;

    
    if (state->mode != GZ_READ || state->err != Z_OK)
        return NULL;

    
    if (state->seek) {
        state->seek = 0;
        if (gz_skip(state, state->skip) == -1)
            return NULL;
    }

    


    str = buf;
    left = (unsigned)len - 1;
    if (left) do {
        
        if (state->have == 0) {
            if (gz_make(state) == -1)
                return NULL;            
            if (state->have == 0) {     
                if (buf == str)         
                    return NULL;
                break;                  
            }
        }

        
        n = state->have > left ? left : state->have;
        eol = memchr(state->next, '\n', n);
        if (eol != NULL)
            n = (unsigned)(eol - state->next) + 1;

        
        memcpy(buf, state->next, n);
        state->have -= n;
        state->next += n;
        state->pos += n;
        left -= n;
        buf += n;
    } while (left && eol == NULL);

    
    buf[0] = 0;
    return str;
}


int ZEXPORT gzdirect(file)
    gzFile file;
{
    gz_statep state;

    
    if (file == NULL)
        return 0;
    state = (gz_statep)file;

    
    if (state->mode != GZ_READ)
        return 0;

    

    if (state->how == LOOK && state->have == 0)
        (void)gz_head(state);

    
    return state->direct;
}


int ZEXPORT gzclose_r(file)
    gzFile file;
{
    int ret;
    gz_statep state;

    
    if (file == NULL)
        return Z_STREAM_ERROR;
    state = (gz_statep)file;

    
    if (state->mode != GZ_READ)
        return Z_STREAM_ERROR;

    
    if (state->size) {
        inflateEnd(&(state->strm));
        free(state->out);
        free(state->in);
    }
    gz_error(state, Z_OK, NULL);
    free(state->path);
    ret = close(state->fd);
    free(state);
    return ret ? Z_ERRNO : Z_OK;
}
