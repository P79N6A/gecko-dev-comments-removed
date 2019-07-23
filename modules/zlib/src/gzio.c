








#include <stdio.h>

#include "zutil.h"

#ifdef NO_DEFLATE       
#  define NO_GZCOMPRESS
#endif

#ifndef NO_DUMMY_DECL
struct internal_state {int dummy;}; 
#endif

#ifndef Z_BUFSIZE
#  ifdef MAXSEG_64K
#    define Z_BUFSIZE 4096 /* minimize memory usage for 16-bit DOS */
#  else
#    define Z_BUFSIZE 16384
#  endif
#endif
#ifndef Z_PRINTF_BUFSIZE
#  define Z_PRINTF_BUFSIZE 4096
#endif

#ifdef __MVS__
#  pragma map (fdopen , "\174\174FDOPEN")
   FILE *fdopen(int, const char *);
#endif

#ifndef STDC
extern voidp  malloc OF((uInt size));
extern void   free   OF((voidpf ptr));
#endif

#define ALLOC(size) malloc(size)
#define TRYFREE(p) {if (p) free(p);}

static int const gz_magic[2] = {0x1f, 0x8b}; 


#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

typedef struct gz_stream {
    z_stream stream;
    int      z_err;   
    int      z_eof;   
    FILE     *file;   
    Byte     *inbuf;  
    Byte     *outbuf; 
    uLong    crc;     
    char     *msg;    
    char     *path;   
    int      transparent; 
    char     mode;    
    z_off_t  start;   
    z_off_t  in;      
    z_off_t  out;     
    int      back;    
    int      last;    
} gz_stream;


local gzFile gz_open      OF((const char *path, const char *mode, int  fd));
local int do_flush        OF((gzFile file, int flush));
local int    get_byte     OF((gz_stream *s));
local void   check_header OF((gz_stream *s));
local int    destroy      OF((gz_stream *s));
local void   putLong      OF((FILE *file, uLong x));
local uLong  getLong      OF((gz_stream *s));










local gzFile gz_open (path, mode, fd)
    const char *path;
    const char *mode;
    int  fd;
{
    int err;
    int level = Z_DEFAULT_COMPRESSION; 
    int strategy = Z_DEFAULT_STRATEGY; 
    char *p = (char*)mode;
    gz_stream *s;
    char fmode[80]; 
    char *m = fmode;

    if (!path || !mode) return Z_NULL;

    s = (gz_stream *)ALLOC(sizeof(gz_stream));
    if (!s) return Z_NULL;

    s->stream.zalloc = (alloc_func)0;
    s->stream.zfree = (free_func)0;
    s->stream.opaque = (voidpf)0;
    s->stream.next_in = s->inbuf = Z_NULL;
    s->stream.next_out = s->outbuf = Z_NULL;
    s->stream.avail_in = s->stream.avail_out = 0;
    s->file = NULL;
    s->z_err = Z_OK;
    s->z_eof = 0;
    s->in = 0;
    s->out = 0;
    s->back = EOF;
    s->crc = crc32(0L, Z_NULL, 0);
    s->msg = NULL;
    s->transparent = 0;

    s->path = (char*)ALLOC(strlen(path)+1);
    if (s->path == NULL) {
        return destroy(s), (gzFile)Z_NULL;
    }
    strcpy(s->path, path); 

    s->mode = '\0';
    do {
        if (*p == 'r') s->mode = 'r';
        if (*p == 'w' || *p == 'a') s->mode = 'w';
        if (*p >= '0' && *p <= '9') {
            level = *p - '0';
        } else if (*p == 'f') {
          strategy = Z_FILTERED;
        } else if (*p == 'h') {
          strategy = Z_HUFFMAN_ONLY;
        } else if (*p == 'R') {
          strategy = Z_RLE;
        } else {
            *m++ = *p; 
        }
    } while (*p++ && m != fmode + sizeof(fmode));
    if (s->mode == '\0') return destroy(s), (gzFile)Z_NULL;

    if (s->mode == 'w') {
#ifdef NO_GZCOMPRESS
        err = Z_STREAM_ERROR;
#else
        err = deflateInit2(&(s->stream), level,
                           Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, strategy);
        

        s->stream.next_out = s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
#endif
        if (err != Z_OK || s->outbuf == Z_NULL) {
            return destroy(s), (gzFile)Z_NULL;
        }
    } else {
        s->stream.next_in  = s->inbuf = (Byte*)ALLOC(Z_BUFSIZE);

        err = inflateInit2(&(s->stream), -MAX_WBITS);
        





        if (err != Z_OK || s->inbuf == Z_NULL) {
            return destroy(s), (gzFile)Z_NULL;
        }
    }
    s->stream.avail_out = Z_BUFSIZE;

    errno = 0;
    s->file = fd < 0 ? F_OPEN(path, fmode) : (FILE*)fdopen(fd, fmode);

    if (s->file == NULL) {
        return destroy(s), (gzFile)Z_NULL;
    }
    if (s->mode == 'w') {
        

        fprintf(s->file, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
             Z_DEFLATED, 0 , 0,0,0,0 , 0 , OS_CODE);
        s->start = 10L;
        




    } else {
        check_header(s); 
        s->start = ftell(s->file) - s->stream.avail_in;
    }

    return (gzFile)s;
}




gzFile ZEXPORT gzopen (path, mode)
    const char *path;
    const char *mode;
{
    return gz_open (path, mode, -1);
}





gzFile ZEXPORT gzdopen (fd, mode)
    int fd;
    const char *mode;
{
    char name[46];      

    if (fd < 0) return (gzFile)Z_NULL;
    sprintf(name, "<fd:%d>", fd); 

    return gz_open (name, mode, fd);
}




int ZEXPORT gzsetparams (file, level, strategy)
    gzFile file;
    int level;
    int strategy;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    
    if (s->stream.avail_out == 0) {

        s->stream.next_out = s->outbuf;
        if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
            s->z_err = Z_ERRNO;
        }
        s->stream.avail_out = Z_BUFSIZE;
    }

    return deflateParams (&(s->stream), level, strategy);
}






local int get_byte(s)
    gz_stream *s;
{
    if (s->z_eof) return EOF;
    if (s->stream.avail_in == 0) {
        errno = 0;
        s->stream.avail_in = (uInt)fread(s->inbuf, 1, Z_BUFSIZE, s->file);
        if (s->stream.avail_in == 0) {
            s->z_eof = 1;
            if (ferror(s->file)) s->z_err = Z_ERRNO;
            return EOF;
        }
        s->stream.next_in = s->inbuf;
    }
    s->stream.avail_in--;
    return *(s->stream.next_in)++;
}










local void check_header(s)
    gz_stream *s;
{
    int method; 
    int flags;  
    uInt len;
    int c;

    


    len = s->stream.avail_in;
    if (len < 2) {
        if (len) s->inbuf[0] = s->stream.next_in[0];
        errno = 0;
        len = (uInt)fread(s->inbuf + len, 1, Z_BUFSIZE >> len, s->file);
        if (len == 0 && ferror(s->file)) s->z_err = Z_ERRNO;
        s->stream.avail_in += len;
        s->stream.next_in = s->inbuf;
        if (s->stream.avail_in < 2) {
            s->transparent = s->stream.avail_in;
            return;
        }
    }

    
    if (s->stream.next_in[0] != gz_magic[0] ||
        s->stream.next_in[1] != gz_magic[1]) {
        s->transparent = 1;
        return;
    }
    s->stream.avail_in -= 2;
    s->stream.next_in += 2;

    
    method = get_byte(s);
    flags = get_byte(s);
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
        s->z_err = Z_DATA_ERROR;
        return;
    }

    
    for (len = 0; len < 6; len++) (void)get_byte(s);

    if ((flags & EXTRA_FIELD) != 0) { 
        len  =  (uInt)get_byte(s);
        len += ((uInt)get_byte(s))<<8;
        
        while (len-- != 0 && get_byte(s) != EOF) ;
    }
    if ((flags & ORIG_NAME) != 0) { 
        while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & COMMENT) != 0) {   
        while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & HEAD_CRC) != 0) {  
        for (len = 0; len < 2; len++) (void)get_byte(s);
    }
    s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

 



local int destroy (s)
    gz_stream *s;
{
    int err = Z_OK;

    if (!s) return Z_STREAM_ERROR;

    TRYFREE(s->msg);

    if (s->stream.state != NULL) {
        if (s->mode == 'w') {
#ifdef NO_GZCOMPRESS
            err = Z_STREAM_ERROR;
#else
            err = deflateEnd(&(s->stream));
#endif
        } else if (s->mode == 'r') {
            err = inflateEnd(&(s->stream));
        }
    }
    if (s->file != NULL && fclose(s->file)) {
#ifdef ESPIPE
        if (errno != ESPIPE) 
#endif
            err = Z_ERRNO;
    }
    if (s->z_err < 0) err = s->z_err;

    TRYFREE(s->inbuf);
    TRYFREE(s->outbuf);
    TRYFREE(s->path);
    TRYFREE(s);
    return err;
}





int ZEXPORT gzread (file, buf, len)
    gzFile file;
    voidp buf;
    unsigned len;
{
    gz_stream *s = (gz_stream*)file;
    Bytef *start = (Bytef*)buf; 
    Byte  *next_out; 

    if (s == NULL || s->mode != 'r') return Z_STREAM_ERROR;

    if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
    if (s->z_err == Z_STREAM_END) return 0;  

    next_out = (Byte*)buf;
    s->stream.next_out = (Bytef*)buf;
    s->stream.avail_out = len;

    if (s->stream.avail_out && s->back != EOF) {
        *next_out++ = s->back;
        s->stream.next_out++;
        s->stream.avail_out--;
        s->back = EOF;
        s->out++;
        start++;
        if (s->last) {
            s->z_err = Z_STREAM_END;
            return 1;
        }
    }

    while (s->stream.avail_out != 0) {

        if (s->transparent) {
            
            uInt n = s->stream.avail_in;
            if (n > s->stream.avail_out) n = s->stream.avail_out;
            if (n > 0) {
                zmemcpy(s->stream.next_out, s->stream.next_in, n);
                next_out += n;
                s->stream.next_out = next_out;
                s->stream.next_in   += n;
                s->stream.avail_out -= n;
                s->stream.avail_in  -= n;
            }
            if (s->stream.avail_out > 0) {
                s->stream.avail_out -=
                    (uInt)fread(next_out, 1, s->stream.avail_out, s->file);
            }
            len -= s->stream.avail_out;
            s->in  += len;
            s->out += len;
            if (len == 0) s->z_eof = 1;
            return (int)len;
        }
        if (s->stream.avail_in == 0 && !s->z_eof) {

            errno = 0;
            s->stream.avail_in = (uInt)fread(s->inbuf, 1, Z_BUFSIZE, s->file);
            if (s->stream.avail_in == 0) {
                s->z_eof = 1;
                if (ferror(s->file)) {
                    s->z_err = Z_ERRNO;
                    break;
                }
            }
            s->stream.next_in = s->inbuf;
        }
        s->in += s->stream.avail_in;
        s->out += s->stream.avail_out;
        s->z_err = inflate(&(s->stream), Z_NO_FLUSH);
        s->in -= s->stream.avail_in;
        s->out -= s->stream.avail_out;

        if (s->z_err == Z_STREAM_END) {
            
            s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
            start = s->stream.next_out;

            if (getLong(s) != s->crc) {
                s->z_err = Z_DATA_ERROR;
            } else {
                (void)getLong(s);
                



                check_header(s);
                if (s->z_err == Z_OK) {
                    inflateReset(&(s->stream));
                    s->crc = crc32(0L, Z_NULL, 0);
                }
            }
        }
        if (s->z_err != Z_OK || s->z_eof) break;
    }
    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));

    if (len == s->stream.avail_out &&
        (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO))
        return -1;
    return (int)(len - s->stream.avail_out);
}






int ZEXPORT gzgetc(file)
    gzFile file;
{
    unsigned char c;

    return gzread(file, &c, 1) == 1 ? c : -1;
}





int ZEXPORT gzungetc(c, file)
    int c;
    gzFile file;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'r' || c == EOF || s->back != EOF) return EOF;
    s->back = c;
    s->out--;
    s->last = (s->z_err == Z_STREAM_END);
    if (s->last) s->z_err = Z_OK;
    s->z_eof = 0;
    return c;
}











char * ZEXPORT gzgets(file, buf, len)
    gzFile file;
    char *buf;
    int len;
{
    char *b = buf;
    if (buf == Z_NULL || len <= 0) return Z_NULL;

    while (--len > 0 && gzread(file, buf, 1) == 1 && *buf++ != '\n') ;
    *buf = '\0';
    return b == buf && len > 0 ? Z_NULL : b;
}


#ifndef NO_GZCOMPRESS




int ZEXPORT gzwrite (file, buf, len)
    gzFile file;
    voidpc buf;
    unsigned len;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    s->stream.next_in = (Bytef*)buf;
    s->stream.avail_in = len;

    while (s->stream.avail_in != 0) {

        if (s->stream.avail_out == 0) {

            s->stream.next_out = s->outbuf;
            if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
                s->z_err = Z_ERRNO;
                break;
            }
            s->stream.avail_out = Z_BUFSIZE;
        }
        s->in += s->stream.avail_in;
        s->out += s->stream.avail_out;
        s->z_err = deflate(&(s->stream), Z_NO_FLUSH);
        s->in -= s->stream.avail_in;
        s->out -= s->stream.avail_out;
        if (s->z_err != Z_OK) break;
    }
    s->crc = crc32(s->crc, (const Bytef *)buf, len);

    return (int)(len - s->stream.avail_in);
}







#ifdef STDC
#include <stdarg.h>

int ZEXPORTVA gzprintf (gzFile file, const char *format,  ...)
{
    char buf[Z_PRINTF_BUFSIZE];
    va_list va;
    int len;

    buf[sizeof(buf) - 1] = 0;
    va_start(va, format);
#ifdef NO_vsnprintf
#  ifdef HAS_vsprintf_void
    (void)vsprintf(buf, format, va);
    va_end(va);
    for (len = 0; len < sizeof(buf); len++)
        if (buf[len] == 0) break;
#  else
    len = vsprintf(buf, format, va);
    va_end(va);
#  endif
#else
#  ifdef HAS_vsnprintf_void
    (void)vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
    len = strlen(buf);
#  else
    len = vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
#  endif
#endif
    if (len <= 0 || len >= (int)sizeof(buf) || buf[sizeof(buf) - 1] != 0)
        return 0;
    return gzwrite(file, buf, (unsigned)len);
}
#else 

int ZEXPORTVA gzprintf (file, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
                       a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
    gzFile file;
    const char *format;
    int a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
        a11, a12, a13, a14, a15, a16, a17, a18, a19, a20;
{
    char buf[Z_PRINTF_BUFSIZE];
    int len;

    buf[sizeof(buf) - 1] = 0;
#ifdef NO_snprintf
#  ifdef HAS_sprintf_void
    sprintf(buf, format, a1, a2, a3, a4, a5, a6, a7, a8,
            a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    for (len = 0; len < sizeof(buf); len++)
        if (buf[len] == 0) break;
#  else
    len = sprintf(buf, format, a1, a2, a3, a4, a5, a6, a7, a8,
                a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
#  endif
#else
#  ifdef HAS_snprintf_void
    snprintf(buf, sizeof(buf), format, a1, a2, a3, a4, a5, a6, a7, a8,
             a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
    len = strlen(buf);
#  else
    len = snprintf(buf, sizeof(buf), format, a1, a2, a3, a4, a5, a6, a7, a8,
                 a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
#  endif
#endif
    if (len <= 0 || len >= sizeof(buf) || buf[sizeof(buf) - 1] != 0)
        return 0;
    return gzwrite(file, buf, len);
}
#endif





int ZEXPORT gzputc(file, c)
    gzFile file;
    int c;
{
    unsigned char cc = (unsigned char) c; 

    return gzwrite(file, &cc, 1) == 1 ? (int)cc : -1;
}







int ZEXPORT gzputs(file, s)
    gzFile file;
    const char *s;
{
    return gzwrite(file, (char*)s, (unsigned)strlen(s));
}






local int do_flush (file, flush)
    gzFile file;
    int flush;
{
    uInt len;
    int done = 0;
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    s->stream.avail_in = 0; 

    for (;;) {
        len = Z_BUFSIZE - s->stream.avail_out;

        if (len != 0) {
            if ((uInt)fwrite(s->outbuf, 1, len, s->file) != len) {
                s->z_err = Z_ERRNO;
                return Z_ERRNO;
            }
            s->stream.next_out = s->outbuf;
            s->stream.avail_out = Z_BUFSIZE;
        }
        if (done) break;
        s->out += s->stream.avail_out;
        s->z_err = deflate(&(s->stream), flush);
        s->out -= s->stream.avail_out;

        
        if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;

        


        done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);

        if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
    }
    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

int ZEXPORT gzflush (file, flush)
     gzFile file;
     int flush;
{
    gz_stream *s = (gz_stream*)file;
    int err = do_flush (file, flush);

    if (err) return err;
    fflush(s->file);
    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}
#endif 









z_off_t ZEXPORT gzseek (file, offset, whence)
    gzFile file;
    z_off_t offset;
    int whence;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || whence == SEEK_END ||
        s->z_err == Z_ERRNO || s->z_err == Z_DATA_ERROR) {
        return -1L;
    }

    if (s->mode == 'w') {
#ifdef NO_GZCOMPRESS
        return -1L;
#else
        if (whence == SEEK_SET) {
            offset -= s->in;
        }
        if (offset < 0) return -1L;

        
        if (s->inbuf == Z_NULL) {
            s->inbuf = (Byte*)ALLOC(Z_BUFSIZE); 
            if (s->inbuf == Z_NULL) return -1L;
            zmemzero(s->inbuf, Z_BUFSIZE);
        }
        while (offset > 0)  {
            uInt size = Z_BUFSIZE;
            if (offset < Z_BUFSIZE) size = (uInt)offset;

            size = gzwrite(file, s->inbuf, size);
            if (size == 0) return -1L;

            offset -= size;
        }
        return s->in;
#endif
    }
    

    
    if (whence == SEEK_CUR) {
        offset += s->out;
    }
    if (offset < 0) return -1L;

    if (s->transparent) {
        
        s->back = EOF;
        s->stream.avail_in = 0;
        s->stream.next_in = s->inbuf;
        if (fseek(s->file, offset, SEEK_SET) < 0) return -1L;

        s->in = s->out = offset;
        return offset;
    }

    
    if (offset >= s->out) {
        offset -= s->out;
    } else if (gzrewind(file) < 0) {
        return -1L;
    }
    

    if (offset != 0 && s->outbuf == Z_NULL) {
        s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
        if (s->outbuf == Z_NULL) return -1L;
    }
    if (offset && s->back != EOF) {
        s->back = EOF;
        s->out++;
        offset--;
        if (s->last) s->z_err = Z_STREAM_END;
    }
    while (offset > 0)  {
        int size = Z_BUFSIZE;
        if (offset < Z_BUFSIZE) size = (int)offset;

        size = gzread(file, s->outbuf, (uInt)size);
        if (size <= 0) return -1L;
        offset -= size;
    }
    return s->out;
}




int ZEXPORT gzrewind (file)
    gzFile file;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'r') return -1;

    s->z_err = Z_OK;
    s->z_eof = 0;
    s->back = EOF;
    s->stream.avail_in = 0;
    s->stream.next_in = s->inbuf;
    s->crc = crc32(0L, Z_NULL, 0);
    if (!s->transparent) (void)inflateReset(&s->stream);
    s->in = 0;
    s->out = 0;
    return fseek(s->file, s->start, SEEK_SET);
}






z_off_t ZEXPORT gztell (file)
    gzFile file;
{
    return gzseek(file, 0L, SEEK_CUR);
}





int ZEXPORT gzeof (file)
    gzFile file;
{
    gz_stream *s = (gz_stream*)file;

    



    if (s == NULL || s->mode != 'r') return 0;
    if (s->z_eof) return 1;
    return s->z_err == Z_STREAM_END;
}




int ZEXPORT gzdirect (file)
    gzFile file;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'r') return 0;
    return s->transparent;
}




local void putLong (file, x)
    FILE *file;
    uLong x;
{
    int n;
    for (n = 0; n < 4; n++) {
        fputc((int)(x & 0xff), file);
        x >>= 8;
    }
}





local uLong getLong (s)
    gz_stream *s;
{
    uLong x = (uLong)get_byte(s);
    int c;

    x += ((uLong)get_byte(s))<<8;
    x += ((uLong)get_byte(s))<<16;
    c = get_byte(s);
    if (c == EOF) s->z_err = Z_DATA_ERROR;
    x += ((uLong)c)<<24;
    return x;
}





int ZEXPORT gzclose (file)
    gzFile file;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL) return Z_STREAM_ERROR;

    if (s->mode == 'w') {
#ifdef NO_GZCOMPRESS
        return Z_STREAM_ERROR;
#else
        if (do_flush (file, Z_FINISH) != Z_OK)
            return destroy((gz_stream*)file);

        putLong (s->file, s->crc);
        putLong (s->file, (uLong)(s->in & 0xffffffff));
#endif
    }
    return destroy((gz_stream*)file);
}

#ifdef STDC
#  define zstrerror(errnum) strerror(errnum)
#else
#  define zstrerror(errnum) ""
#endif








const char * ZEXPORT gzerror (file, errnum)
    gzFile file;
    int *errnum;
{
    char *m;
    gz_stream *s = (gz_stream*)file;

    if (s == NULL) {
        *errnum = Z_STREAM_ERROR;
        return (const char*)ERR_MSG(Z_STREAM_ERROR);
    }
    *errnum = s->z_err;
    if (*errnum == Z_OK) return (const char*)"";

    m = (char*)(*errnum == Z_ERRNO ? zstrerror(errno) : s->stream.msg);

    if (m == NULL || *m == '\0') m = (char*)ERR_MSG(s->z_err);

    TRYFREE(s->msg);
    s->msg = (char*)ALLOC(strlen(s->path) + strlen(m) + 3);
    if (s->msg == Z_NULL) return (const char*)ERR_MSG(Z_MEM_ERROR);
    strcpy(s->msg, s->path);
    strcat(s->msg, ": ");
    strcat(s->msg, m);
    return (const char*)s->msg;
}




void ZEXPORT gzclearerr (file)
    gzFile file;
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL) return;
    if (s->z_err != Z_STREAM_END) s->z_err = Z_OK;
    s->z_eof = 0;
    clearerr(s->file);
}
