



























#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include "linker_format.h"
#include "linker_debug.h"




#define xxUNIT_TESTS




typedef struct {
    void *opaque;
    void (*send)(void *opaque, const char *data, int len);
} Out;

static void
out_send(Out *o, const void *data, size_t len)
{
    o->send(o->opaque, data, (int)len);
}

static void
out_send_repeat(Out *o, char ch, int count)
{
    char pad[8];
    const int padSize = (int)sizeof(pad);

    memset(pad, ch, sizeof(pad));
    while (count > 0) {
        int avail = count;
        if (avail > padSize) {
            avail = padSize;
        }
        o->send(o->opaque, pad, avail);
        count -= avail;
    }
}


static void
out_vformat(Out *o, const char *format, va_list args);




typedef struct {
    Out out[1];
    char *buffer;
    char *pos;
    char *end;
    int total;
} BufOut;

static void
buf_out_send(void *opaque, const char *data, int len)
{
    BufOut *bo = opaque;

    if (len < 0)
        len = strlen(data);

    bo->total += len;

    while (len > 0) {
        int avail = bo->end - bo->pos;
        if (avail == 0)
            break;
        if (avail > len)
            avail = len;
        memcpy(bo->pos, data, avail);
        bo->pos += avail;
        bo->pos[0] = '\0';
        len -= avail;
    }
}

static Out*
buf_out_init(BufOut *bo, char *buffer, size_t size)
{
    if (size == 0)
        return NULL;

    bo->out->opaque = bo;
    bo->out->send   = buf_out_send;
    bo->buffer      = buffer;
    bo->end         = buffer + size - 1;
    bo->pos         = bo->buffer;
    bo->pos[0]      = '\0';
    bo->total       = 0;

    return bo->out;
}

static int
buf_out_length(BufOut *bo)
{
    return bo->total;
}

static int
vformat_buffer(char *buff, size_t buffsize, const char *format, va_list args)
{
    BufOut bo;
    Out *out;

    out = buf_out_init(&bo, buff, buffsize);
    if (out == NULL)
        return 0;

    out_vformat(out, format, args);

    return buf_out_length(&bo);
}

int
format_buffer(char *buff, size_t buffsize, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vformat_buffer(buff, buffsize, format, args);
    va_end(args);

    return ret;
}







int
vsnprintf(char *buff, size_t bufsize, const char *format, va_list args)
{
    return format_buffer(buff, bufsize, format, args);
}

#if LINKER_DEBUG

#if !LINKER_DEBUG_TO_LOG




typedef struct {
    Out out[1];
    int fd;
    int total;
} FdOut;

static void
fd_out_send(void *opaque, const char *data, int len)
{
    FdOut *fdo = opaque;

    if (len < 0)
        len = strlen(data);

    while (len > 0) {
        int ret = write(fdo->fd, data, len);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        data += ret;
        len -= ret;
        fdo->total += ret;
    }
}

static Out*
fd_out_init(FdOut *fdo, int  fd)
{
    fdo->out->opaque = fdo;
    fdo->out->send = fd_out_send;
    fdo->fd = fd;
    fdo->total = 0;

    return fdo->out;
}

static int
fd_out_length(FdOut *fdo)
{
    return fdo->total;
}


int
format_fd(int fd, const char *format, ...)
{
    FdOut fdo;
    Out* out;
    va_list args;

    out = fd_out_init(&fdo, fd);
    if (out == NULL)
        return 0;

    va_start(args, format);
    out_vformat(out, format, args);
    va_end(args);

    return fd_out_length(&fdo);
}

#else 










#define  CUSTOM_LOG_VPRINT  1

#if CUSTOM_LOG_VPRINT

#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>

static int log_vprint(int prio, const char *tag, const char *fmt, va_list  args)
{
    char buf[1024];
    int result;
    static int log_fd = -1;

    result = vformat_buffer(buf, sizeof buf, fmt, args);

    if (log_fd < 0) {
        log_fd = open("/dev/log/main", O_WRONLY);
        if (log_fd < 0)
            return result;
    }

    {
        ssize_t ret;
        struct iovec vec[3];

        vec[0].iov_base = (unsigned char *) &prio;
        vec[0].iov_len = 1;
        vec[1].iov_base = (void *) tag;
        vec[1].iov_len = strlen(tag) + 1;
        vec[2].iov_base = (void *) buf;
        vec[2].iov_len = strlen(buf) + 1;

        do {
            ret = writev(log_fd, vec, 3);
        } while ((ret < 0) && (errno == EINTR));
    }
    return result;
}

#define  __libc_android_log_vprint  log_vprint

#else 

extern int __libc_android_log_vprint(int  prio, const char* tag, const char*  format, va_list ap);

#endif 

int
format_log(int prio, const char *tag, const char *format, ...)
{
    int ret;
    va_list  args;
    va_start(args, format);
    ret = __libc_android_log_vprint(prio, tag, format, args);
    va_end(args);
    return ret;
}

#endif 

#endif 










static unsigned
parse_decimal(const char *format, int *ppos)
{
    const char* p = format + *ppos;
    unsigned result = 0;

    for (;;) {
        int ch = *p;
        unsigned d = (unsigned)(ch - '0');

        if (d >= 10U)
            break;

        result = result*10 + d;
        p++;
    }
    *ppos = p - format;
    return result;
}





static void
format_number(char *buffer, size_t bufsize, uint64_t value, int base, const char *digits)
{
    char *pos = buffer;
    char *end = buffer + bufsize - 1;

    
    while (value) {
        unsigned d = value % base;
        value /= base;
        if (pos < end) {
            *pos++ = digits[d];
        }
    }

    
    if (pos == buffer) {
        if (pos < end) {
            *pos++ = '0';
        }
    }
    pos[0] = '\0';

    
    end = pos - 1;
    pos = buffer;
    while (pos < end) {
        int ch = pos[0];
        pos[0] = end[0];
        end[0] = (char) ch;
        pos++;
        end--;
    }
}


static void
format_integer(char *buffer, size_t buffsize, uint64_t value, int base, int isSigned)
{
    if (isSigned && (int64_t)value < 0) {
        buffer[0] = '-';
        buffer += 1;
        buffsize -= 1;
        value = (uint64_t)(-(int64_t)value);
    }

    format_number(buffer, buffsize, value, base, "0123456789");
}


static void
format_octal(char *buffer, size_t buffsize, uint64_t value, int isSigned)
{
    format_integer(buffer, buffsize, value, 8, isSigned);
}


static void
format_decimal(char *buffer, size_t buffsize, uint64_t value, int isSigned)
{
    format_integer(buffer, buffsize, value, 10, isSigned);
}



static void
format_hex(char *buffer, size_t buffsize, uint64_t value, int isCap)
{
    const char *digits = isCap ? "0123456789ABCDEF" : "0123456789abcdef";

    format_number(buffer, buffsize, value, 16, digits);
}



static void
out_vformat(Out *o, const char *format, va_list args)
{
    int nn = 0, mm;
    int padZero = 0;
    int padLeft = 0;
    char sign = '\0';
    int width = -1;
    int prec  = -1;
    size_t bytelen = sizeof(int);
    const char*  str;
    int slen;
    char buffer[32];  

    for (;;) {
        char  c;

        
        
        mm = nn;
        do {
            c = format[mm];
            if (c == '\0' || c == '%')
                break;
            mm++;
        } while (1);

        if (mm > nn) {
            out_send(o, format+nn, mm-nn);
            nn = mm;
        }

        
        if (c == '\0')
            break;

        
        nn++;  

        
        for (;;) {
            c = format[nn++];
            if (c == '\0') {  
                c = '%';
                out_send(o, &c, 1);
                return;
            }
            else if (c == '0') {
                padZero = 1;
                continue;
            }
            else if (c == '-') {
                padLeft = 1;
                continue;
            }
            else if (c == ' ' || c == '+') {
                sign = c;
                continue;
            }
            break;
        }

        
        if ((c >= '0' && c <= '9')) {
            nn --;
            width = (int)parse_decimal(format, &nn);
            c = format[nn++];
        }

        
        if (c == '.') {
            prec = (int)parse_decimal(format, &nn);
            c = format[nn++];
        }

        
        switch (c) {
        case 'h':
            bytelen = sizeof(short);
            if (format[nn] == 'h') {
                bytelen = sizeof(char);
                nn += 1;
            }
            c = format[nn++];
            break;
        case 'l':
            bytelen = sizeof(long);
            if (format[nn] == 'l') {
                bytelen = sizeof(long long);
                nn += 1;
            }
            c = format[nn++];
            break;
        case 'z':
            bytelen = sizeof(size_t);
            c = format[nn++];
            break;
        case 't':
            bytelen = sizeof(ptrdiff_t);
            c = format[nn++];
            break;
        case 'p':
            bytelen = sizeof(void*);
            c = format[nn++];
        default:
            ;
        }

        
        if (c == 's') {
            
            str = va_arg(args, const char*);
        } else if (c == 'c') {
            
            
            buffer[0] = (char) va_arg(args, int);
            buffer[1] = '\0';
            str = buffer;
        } else if (c == 'p') {
            uint64_t  value = (uint64_t)(ptrdiff_t) va_arg(args, void*);
            buffer[0] = '0';
            buffer[1] = 'x';
            format_hex(buffer + 2, sizeof buffer-2, value, 0);
            str = buffer;
        } else {
            
            uint64_t value;
            int isSigned = (c == 'd' || c == 'i' || c == 'o');

            


            switch (bytelen) {
            case 1: value = (uint8_t)  va_arg(args, int); break;
            case 2: value = (uint16_t) va_arg(args, int); break;
            case 4: value = va_arg(args, uint32_t); break;
            case 8: value = va_arg(args, uint64_t); break;
            default: return;  
            }

            
            if (isSigned) {
                int shift = 64 - 8*bytelen;
                value = (uint64_t)(((int64_t)(value << shift)) >> shift);
            }

            
            switch (c) {
            case 'i': case 'd':
                format_integer(buffer, sizeof buffer, value, 10, isSigned);
                break;
            case 'o':
                format_integer(buffer, sizeof buffer, value, 8, isSigned);
                break;
            case 'x': case 'X':
                format_hex(buffer, sizeof buffer, value, (c == 'X'));
                break;
            default:
                buffer[0] = '\0';
            }
            
            str = buffer;
        }

        


        slen = strlen(str);

        if (slen < width && !padLeft) {
            char padChar = padZero ? '0' : ' ';
            out_send_repeat(o, padChar, width - slen);
        }

        out_send(o, str, slen);

        if (slen < width && padLeft) {
            char padChar = padZero ? '0' : ' ';
            out_send_repeat(o, padChar, width - slen);
        }
    }
}


#ifdef UNIT_TESTS

#include <stdio.h>

static int   gFails = 0;

#define  MARGIN  40

#define  UTEST_CHECK(condition,message) \
    printf("Checking %-*s: ", MARGIN, message); fflush(stdout); \
    if (!(condition)) { \
        printf("KO\n"); \
        gFails += 1; \
    } else { \
        printf("ok\n"); \
    }

static void
utest_BufOut(void)
{
    char buffer[16];
    BufOut bo[1];
    Out* out;
    int ret;

    buffer[0] = '1';
    out = buf_out_init(bo, buffer, sizeof buffer);
    UTEST_CHECK(buffer[0] == '\0', "buf_out_init clears initial byte");
    out_send(out, "abc", 3);
    UTEST_CHECK(!memcmp(buffer, "abc", 4), "out_send() works with BufOut");
    out_send_repeat(out, 'X', 4);
    UTEST_CHECK(!memcmp(buffer, "abcXXXX", 8), "out_send_repeat() works with BufOut");
    buffer[sizeof buffer-1] = 'x';
    out_send_repeat(out, 'Y', 2*sizeof(buffer));
    UTEST_CHECK(buffer[sizeof buffer-1] == '\0', "overflows always zero-terminates");

    out = buf_out_init(bo, buffer, sizeof buffer);
    out_send_repeat(out, 'X', 2*sizeof(buffer));
    ret = buf_out_length(bo);
    UTEST_CHECK(ret == 2*sizeof(buffer), "correct size returned on overflow");
}

static void
utest_expect(const char*  result, const char*  format, ...)
{
    va_list args;
    BufOut bo[1];
    char buffer[256];
    Out* out = buf_out_init(bo, buffer, sizeof buffer);

    printf("Checking %-*s: ", MARGIN, format); fflush(stdout);
    va_start(args, format);
    out_vformat(out, format, args);
    va_end(args);

    if (strcmp(result, buffer)) {
        printf("KO. got '%s' expecting '%s'\n", buffer, result);
        gFails += 1;
    } else {
        printf("ok. got '%s'\n", result);
    }
}

int  main(void)
{
    utest_BufOut();
    utest_expect("", "");
    utest_expect("a", "a");
    utest_expect("01234", "01234", "");
    utest_expect("01234", "%s", "01234");
    utest_expect("aabbcc", "aa%scc", "bb");
    utest_expect("a", "%c", 'a');
    utest_expect("1234", "%d", 1234);
    utest_expect("-8123", "%d", -8123);
    utest_expect("16", "%hd", 0x7fff0010);
    utest_expect("16", "%hhd", 0x7fffff10);
    utest_expect("68719476736", "%lld", 0x1000000000);
    utest_expect("70000", "%ld", 70000);
    utest_expect("0xb0001234", "%p", (void*)0xb0001234);
    utest_expect("12ab", "%x", 0x12ab);
    utest_expect("12AB", "%X", 0x12ab);
    utest_expect("00123456", "%08x", 0x123456);
    utest_expect("01234", "0%d", 1234);
    utest_expect(" 1234", "%5d", 1234);
    utest_expect("01234", "%05d", 1234);
    utest_expect("    1234", "%8d", 1234);
    utest_expect("1234    ", "%-8d", 1234);
    utest_expect("abcdef     ", "%-11s", "abcdef");
    utest_expect("something:1234", "%s:%d", "something", 1234);
    return gFails != 0;
}

#endif 
