
















#include <stdio.h>
#include "zlib.h"

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

#ifdef USE_MMAP
#  include <sys/types.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#endif

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#ifdef RISCOS
#  define unlink remove
#  define GZ_SUFFIX "-gz"
#  define fileno(file) file->__file
#endif
#if defined(__MWERKS__) && __dest_os != __be_os && __dest_os != __win32_os
#  include <unix.h> 
#endif

#ifndef WIN32 
  extern int unlink OF((const char *));
#endif

#ifndef GZ_SUFFIX
#  define GZ_SUFFIX ".gz"
#endif
#define SUFFIX_LEN (sizeof(GZ_SUFFIX)-1)

#define BUFLEN      16384
#define MAX_NAME_LEN 1024

#ifdef MAXSEG_64K
#  define local static
   
#else
#  define local
#endif

char *prog;

void error            OF((const char *msg));
void gz_compress      OF((FILE   *in, gzFile out));
#ifdef USE_MMAP
int  gz_compress_mmap OF((FILE   *in, gzFile out));
#endif
void gz_uncompress    OF((gzFile in, FILE   *out));
void file_compress    OF((char  *file, char *mode));
void file_uncompress  OF((char  *file));
int  main             OF((int argc, char *argv[]));




void error(msg)
    const char *msg;
{
    fprintf(stderr, "%s: %s\n", prog, msg);
    exit(1);
}





void gz_compress(in, out)
    FILE   *in;
    gzFile out;
{
    local char buf[BUFLEN];
    int len;
    int err;

#ifdef USE_MMAP
    


    if (gz_compress_mmap(in, out) == Z_OK) return;
#endif
    for (;;) {
        len = (int)fread(buf, 1, sizeof(buf), in);
        if (ferror(in)) {
            perror("fread");
            exit(1);
        }
        if (len == 0) break;

        if (gzwrite(out, buf, (unsigned)len) != len) error(gzerror(out, &err));
    }
    fclose(in);
    if (gzclose(out) != Z_OK) error("failed gzclose");
}

#ifdef USE_MMAP 




int gz_compress_mmap(in, out)
    FILE   *in;
    gzFile out;
{
    int len;
    int err;
    int ifd = fileno(in);
    caddr_t buf;    
    off_t buf_len;  
    struct stat sb;

    
    if (fstat(ifd, &sb) < 0) return Z_ERRNO;
    buf_len = sb.st_size;
    if (buf_len <= 0) return Z_ERRNO;

    
    buf = mmap((caddr_t) 0, buf_len, PROT_READ, MAP_SHARED, ifd, (off_t)0);
    if (buf == (caddr_t)(-1)) return Z_ERRNO;

    
    len = gzwrite(out, (char *)buf, (unsigned)buf_len);

    if (len != (int)buf_len) error(gzerror(out, &err));

    munmap(buf, buf_len);
    fclose(in);
    if (gzclose(out) != Z_OK) error("failed gzclose");
    return Z_OK;
}
#endif 




void gz_uncompress(in, out)
    gzFile in;
    FILE   *out;
{
    local char buf[BUFLEN];
    int len;
    int err;

    for (;;) {
        len = gzread(in, buf, sizeof(buf));
        if (len < 0) error (gzerror(in, &err));
        if (len == 0) break;

        if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
            error("failed fwrite");
        }
    }
    if (fclose(out)) error("failed fclose");

    if (gzclose(in) != Z_OK) error("failed gzclose");
}






void file_compress(file, mode)
    char  *file;
    char  *mode;
{
    local char outfile[MAX_NAME_LEN];
    FILE  *in;
    gzFile out;

    strcpy(outfile, file);
    strcat(outfile, GZ_SUFFIX);

    in = fopen(file, "rb");
    if (in == NULL) {
        perror(file);
        exit(1);
    }
    out = gzopen(outfile, mode);
    if (out == NULL) {
        fprintf(stderr, "%s: can't gzopen %s\n", prog, outfile);
        exit(1);
    }
    gz_compress(in, out);

    unlink(file);
}





void file_uncompress(file)
    char  *file;
{
    local char buf[MAX_NAME_LEN];
    char *infile, *outfile;
    FILE  *out;
    gzFile in;
    uInt len = (uInt)strlen(file);

    strcpy(buf, file);

    if (len > SUFFIX_LEN && strcmp(file+len-SUFFIX_LEN, GZ_SUFFIX) == 0) {
        infile = file;
        outfile = buf;
        outfile[len-3] = '\0';
    } else {
        outfile = file;
        infile = buf;
        strcat(infile, GZ_SUFFIX);
    }
    in = gzopen(infile, "rb");
    if (in == NULL) {
        fprintf(stderr, "%s: can't gzopen %s\n", prog, infile);
        exit(1);
    }
    out = fopen(outfile, "wb");
    if (out == NULL) {
        perror(file);
        exit(1);
    }

    gz_uncompress(in, out);

    unlink(infile);
}











int main(argc, argv)
    int argc;
    char *argv[];
{
    int uncompr = 0;
    gzFile file;
    char outmode[20];

    strcpy(outmode, "wb6 ");

    prog = argv[0];
    argc--, argv++;

    while (argc > 0) {
      if (strcmp(*argv, "-d") == 0)
        uncompr = 1;
      else if (strcmp(*argv, "-f") == 0)
        outmode[3] = 'f';
      else if (strcmp(*argv, "-h") == 0)
        outmode[3] = 'h';
      else if (strcmp(*argv, "-r") == 0)
        outmode[3] = 'R';
      else if ((*argv)[0] == '-' && (*argv)[1] >= '1' && (*argv)[1] <= '9' &&
               (*argv)[2] == 0)
        outmode[2] = (*argv)[1];
      else
        break;
      argc--, argv++;
    }
    if (outmode[3] == ' ')
        outmode[3] = 0;
    if (argc == 0) {
        SET_BINARY_MODE(stdin);
        SET_BINARY_MODE(stdout);
        if (uncompr) {
            file = gzdopen(fileno(stdin), "rb");
            if (file == NULL) error("can't gzdopen stdin");
            gz_uncompress(file, stdout);
        } else {
            file = gzdopen(fileno(stdout), outmode);
            if (file == NULL) error("can't gzdopen stdout");
            gz_compress(stdin, file);
        }
    } else {
        do {
            if (uncompr) {
                file_uncompress(*argv);
            } else {
                file_compress(*argv, outmode);
            }
        } while (argv++, --argc);
    }
    return 0;
}
