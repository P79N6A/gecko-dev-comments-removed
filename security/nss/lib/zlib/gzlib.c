




#include "gzguts.h"

#if defined(_LARGEFILE64_SOURCE) && _LFS64_LARGEFILE-0
#  define LSEEK lseek64
#else
#  define LSEEK lseek
#endif


local void gz_reset OF((gz_statep));
local gzFile gz_open OF((const char *, int, const char *));

#if defined UNDER_CE










char ZLIB_INTERNAL *gz_strwinerror (error)
     DWORD error;
{
    static char buf[1024];

    wchar_t *msgbuf;
    DWORD lasterr = GetLastError();
    DWORD chars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL,
        error,
        0, 
        (LPVOID)&msgbuf,
        0,
        NULL);
    if (chars != 0) {
        
        if (chars >= 2
            && msgbuf[chars - 2] == '\r' && msgbuf[chars - 1] == '\n') {
            chars -= 2;
            msgbuf[chars] = 0;
        }

        if (chars > sizeof (buf) - 1) {
            chars = sizeof (buf) - 1;
            msgbuf[chars] = 0;
        }

        wcstombs(buf, msgbuf, chars + 1);
        LocalFree(msgbuf);
    }
    else {
        sprintf(buf, "unknown win32 error (%ld)", error);
    }

    SetLastError(lasterr);
    return buf;
}

#endif 


local void gz_reset(state)
    gz_statep state;
{
    if (state->mode == GZ_READ) {   
        state->have = 0;            
        state->eof = 0;             
        state->how = LOOK;          
        state->direct = 1;          
    }
    state->seek = 0;                
    gz_error(state, Z_OK, NULL);    
    state->pos = 0;                 
    state->strm.avail_in = 0;       
}


local gzFile gz_open(path, fd, mode)
    const char *path;
    int fd;
    const char *mode;
{
    gz_statep state;

    
    state = malloc(sizeof(gz_state));
    if (state == NULL)
        return NULL;
    state->size = 0;            
    state->want = GZBUFSIZE;    
    state->msg = NULL;          

    
    state->mode = GZ_NONE;
    state->level = Z_DEFAULT_COMPRESSION;
    state->strategy = Z_DEFAULT_STRATEGY;
    while (*mode) {
        if (*mode >= '0' && *mode <= '9')
            state->level = *mode - '0';
        else
            switch (*mode) {
            case 'r':
                state->mode = GZ_READ;
                break;
#ifndef NO_GZCOMPRESS
            case 'w':
                state->mode = GZ_WRITE;
                break;
            case 'a':
                state->mode = GZ_APPEND;
                break;
#endif
            case '+':       
                free(state);
                return NULL;
            case 'b':       
                break;
            case 'f':
                state->strategy = Z_FILTERED;
                break;
            case 'h':
                state->strategy = Z_HUFFMAN_ONLY;
                break;
            case 'R':
                state->strategy = Z_RLE;
                break;
            case 'F':
                state->strategy = Z_FIXED;
            default:        
                ;
            }
        mode++;
    }

    
    if (state->mode == GZ_NONE) {
        free(state);
        return NULL;
    }

    
    state->path = malloc(strlen(path) + 1);
    if (state->path == NULL) {
        free(state);
        return NULL;
    }
    strcpy(state->path, path);

    
    state->fd = fd != -1 ? fd :
        open(path,
#ifdef O_LARGEFILE
            O_LARGEFILE |
#endif
#ifdef O_BINARY
            O_BINARY |
#endif
            (state->mode == GZ_READ ?
                O_RDONLY :
                (O_WRONLY | O_CREAT | (
                    state->mode == GZ_WRITE ?
                        O_TRUNC :
                        O_APPEND))),
            0666);
    if (state->fd == -1) {
        free(state->path);
        free(state);
        return NULL;
    }
    if (state->mode == GZ_APPEND)
        state->mode = GZ_WRITE;         

    
    if (state->mode == GZ_READ) {
        state->start = LSEEK(state->fd, 0, SEEK_CUR);
        if (state->start == -1) state->start = 0;
    }

    
    gz_reset(state);

    
    return (gzFile)state;
}


gzFile ZEXPORT gzopen(path, mode)
    const char *path;
    const char *mode;
{
    return gz_open(path, -1, mode);
}


gzFile ZEXPORT gzopen64(path, mode)
    const char *path;
    const char *mode;
{
    return gz_open(path, -1, mode);
}


gzFile ZEXPORT gzdopen(fd, mode)
    int fd;
    const char *mode;
{
    char *path;         
    gzFile gz;

    if (fd == -1 || (path = malloc(7 + 3 * sizeof(int))) == NULL)
        return NULL;
    sprintf(path, "<fd:%d>", fd);   
    gz = gz_open(path, fd, mode);
    free(path);
    return gz;
}


int ZEXPORT gzbuffer(file, size)
    gzFile file;
    unsigned size;
{
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return -1;

    
    if (state->size != 0)
        return -1;

    
    if (size == 0)
        return -1;
    state->want = size;
    return 0;
}


int ZEXPORT gzrewind(file)
    gzFile file;
{
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;

    
    if (state->mode != GZ_READ || state->err != Z_OK)
        return -1;

    
    if (LSEEK(state->fd, state->start, SEEK_SET) == -1)
        return -1;
    gz_reset(state);
    return 0;
}


z_off64_t ZEXPORT gzseek64(file, offset, whence)
    gzFile file;
    z_off64_t offset;
    int whence;
{
    unsigned n;
    z_off64_t ret;
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return -1;

    
    if (state->err != Z_OK)
        return -1;

    
    if (whence != SEEK_SET && whence != SEEK_CUR)
        return -1;

    
    if (whence == SEEK_SET)
        offset -= state->pos;
    else if (state->seek)
        offset += state->skip;
    state->seek = 0;

    
    if (state->mode == GZ_READ && state->how == COPY &&
        state->pos + offset >= state->raw) {
        ret = LSEEK(state->fd, offset - state->have, SEEK_CUR);
        if (ret == -1)
            return -1;
        state->have = 0;
        state->eof = 0;
        state->seek = 0;
        gz_error(state, Z_OK, NULL);
        state->strm.avail_in = 0;
        state->pos += offset;
        return state->pos;
    }

    
    if (offset < 0) {
        if (state->mode != GZ_READ)         
            return -1;
        offset += state->pos;
        if (offset < 0)                     
            return -1;
        if (gzrewind(file) == -1)           
            return -1;
    }

    
    if (state->mode == GZ_READ) {
        n = GT_OFF(state->have) || (z_off64_t)state->have > offset ?
            (unsigned)offset : state->have;
        state->have -= n;
        state->next += n;
        state->pos += n;
        offset -= n;
    }

    
    if (offset) {
        state->seek = 1;
        state->skip = offset;
    }
    return state->pos + offset;
}


z_off_t ZEXPORT gzseek(file, offset, whence)
    gzFile file;
    z_off_t offset;
    int whence;
{
    z_off64_t ret;

    ret = gzseek64(file, (z_off64_t)offset, whence);
    return ret == (z_off_t)ret ? (z_off_t)ret : -1;
}


z_off64_t ZEXPORT gztell64(file)
    gzFile file;
{
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return -1;

    
    return state->pos + (state->seek ? state->skip : 0);
}


z_off_t ZEXPORT gztell(file)
    gzFile file;
{
    z_off64_t ret;

    ret = gztell64(file);
    return ret == (z_off_t)ret ? (z_off_t)ret : -1;
}


z_off64_t ZEXPORT gzoffset64(file)
    gzFile file;
{
    z_off64_t offset;
    gz_statep state;

    
    if (file == NULL)
        return -1;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return -1;

    
    offset = LSEEK(state->fd, 0, SEEK_CUR);
    if (offset == -1)
        return -1;
    if (state->mode == GZ_READ)             
        offset -= state->strm.avail_in;     
    return offset;
}


z_off_t ZEXPORT gzoffset(file)
    gzFile file;
{
    z_off64_t ret;

    ret = gzoffset64(file);
    return ret == (z_off_t)ret ? (z_off_t)ret : -1;
}


int ZEXPORT gzeof(file)
    gzFile file;
{
    gz_statep state;

    
    if (file == NULL)
        return 0;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return 0;

    
    return state->mode == GZ_READ ?
        (state->eof && state->strm.avail_in == 0 && state->have == 0) : 0;
}


const char * ZEXPORT gzerror(file, errnum)
    gzFile file;
    int *errnum;
{
    gz_statep state;

    
    if (file == NULL)
        return NULL;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return NULL;

    
    if (errnum != NULL)
        *errnum = state->err;
    return state->msg == NULL ? "" : state->msg;
}


void ZEXPORT gzclearerr(file)
    gzFile file;
{
    gz_statep state;

    
    if (file == NULL)
        return;
    state = (gz_statep)file;
    if (state->mode != GZ_READ && state->mode != GZ_WRITE)
        return;

    
    if (state->mode == GZ_READ)
        state->eof = 0;
    gz_error(state, Z_OK, NULL);
}







void ZLIB_INTERNAL gz_error(state, err, msg)
    gz_statep state;
    int err;
    const char *msg;
{
    
    if (state->msg != NULL) {
        if (state->err != Z_MEM_ERROR)
            free(state->msg);
        state->msg = NULL;
    }

    
    state->err = err;
    if (msg == NULL)
        return;

    
    if (err == Z_MEM_ERROR) {
        state->msg = (char *)msg;
        return;
    }

    
    if ((state->msg = malloc(strlen(state->path) + strlen(msg) + 3)) == NULL) {
        state->err = Z_MEM_ERROR;
        state->msg = (char *)"out of memory";
        return;
    }
    strcpy(state->msg, state->path);
    strcat(state->msg, ": ");
    strcat(state->msg, msg);
    return;
}

#ifndef INT_MAX




unsigned ZLIB_INTERNAL gz_intmax()
{
    unsigned p, q;

    p = 1;
    do {
        q = p;
        p <<= 1;
        p++;
    } while (p > q);
    return q >> 1;
}
#endif
