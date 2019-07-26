















#include <utils/Unicode.h>

#include <stddef.h>

#ifdef HAVE_WINSOCK
# undef  nhtol
# undef  htonl
# undef  nhtos
# undef  htons

# ifdef HAVE_LITTLE_ENDIAN
#  define ntohl(x)    ( ((x) << 24) | (((x) >> 24) & 255) | (((x) << 8) & 0xff0000) | (((x) >> 8) & 0xff00) )
#  define htonl(x)    ntohl(x)
#  define ntohs(x)    ( (((x) << 8) & 0xff00) | (((x) >> 8) & 255) )
#  define htons(x)    ntohs(x)
# else
#  define ntohl(x)    (x)
#  define htonl(x)    (x)
#  define ntohs(x)    (x)
#  define htons(x)    (x)
# endif
#else
# include <netinet/in.h>
#endif

extern "C" {

static const char32_t kByteMask = 0x000000BF;
static const char32_t kByteMark = 0x00000080;



static const char32_t kUnicodeSurrogateHighStart  = 0x0000D800;
static const char32_t kUnicodeSurrogateHighEnd    = 0x0000DBFF;
static const char32_t kUnicodeSurrogateLowStart   = 0x0000DC00;
static const char32_t kUnicodeSurrogateLowEnd     = 0x0000DFFF;
static const char32_t kUnicodeSurrogateStart      = kUnicodeSurrogateHighStart;
static const char32_t kUnicodeSurrogateEnd        = kUnicodeSurrogateLowEnd;
static const char32_t kUnicodeMaxCodepoint        = 0x0010FFFF;











static const char32_t kFirstByteMark[] = {
    0x00000000, 0x00000000, 0x000000C0, 0x000000E0, 0x000000F0
};









static inline size_t utf32_codepoint_utf8_length(char32_t srcChar)
{
    
    if (srcChar < 0x00000080) {
        return 1;
    } else if (srcChar < 0x00000800) {
        return 2;
    } else if (srcChar < 0x00010000) {
        if ((srcChar < kUnicodeSurrogateStart) || (srcChar > kUnicodeSurrogateEnd)) {
            return 3;
        } else {
            
            return 0;
        }
    }
    
    else if (srcChar <= kUnicodeMaxCodepoint) {
        return 4;
    } else {
        
        return 0;
    }
}



static inline void utf32_codepoint_to_utf8(uint8_t* dstP, char32_t srcChar, size_t bytes)
{
    dstP += bytes;
    switch (bytes)
    {   
        case 4: *--dstP = (uint8_t)((srcChar | kByteMark) & kByteMask); srcChar >>= 6;
        case 3: *--dstP = (uint8_t)((srcChar | kByteMark) & kByteMask); srcChar >>= 6;
        case 2: *--dstP = (uint8_t)((srcChar | kByteMark) & kByteMask); srcChar >>= 6;
        case 1: *--dstP = (uint8_t)(srcChar | kFirstByteMark[bytes]);
    }
}

size_t strlen32(const char32_t *s)
{
  const char32_t *ss = s;
  while ( *ss )
    ss++;
  return ss-s;
}

size_t strnlen32(const char32_t *s, size_t maxlen)
{
  const char32_t *ss = s;
  while ((maxlen > 0) && *ss) {
    ss++;
    maxlen--;
  }
  return ss-s;
}

static inline int32_t utf32_at_internal(const char* cur, size_t *num_read)
{
    const char first_char = *cur;
    if ((first_char & 0x80) == 0) { 
        *num_read = 1;
        return *cur;
    }
    cur++;
    char32_t mask, to_ignore_mask;
    size_t num_to_read = 0;
    char32_t utf32 = first_char;
    for (num_to_read = 1, mask = 0x40, to_ignore_mask = 0xFFFFFF80;
         (first_char & mask);
         num_to_read++, to_ignore_mask |= mask, mask >>= 1) {
        
        utf32 = (utf32 << 6) + (*cur++ & 0x3F);
    }
    to_ignore_mask |= mask;
    utf32 &= ~(to_ignore_mask << (6 * (num_to_read - 1)));

    *num_read = num_to_read;
    return static_cast<int32_t>(utf32);
}

int32_t utf32_from_utf8_at(const char *src, size_t src_len, size_t index, size_t *next_index)
{
    if (index >= src_len) {
        return -1;
    }
    size_t dummy_index;
    if (next_index == NULL) {
        next_index = &dummy_index;
    }
    size_t num_read;
    int32_t ret = utf32_at_internal(src + index, &num_read);
    if (ret >= 0) {
        *next_index = index + num_read;
    }

    return ret;
}

ssize_t utf32_to_utf8_length(const char32_t *src, size_t src_len)
{
    if (src == NULL || src_len == 0) {
        return -1;
    }

    size_t ret = 0;
    const char32_t *end = src + src_len;
    while (src < end) {
        ret += utf32_codepoint_utf8_length(*src++);
    }
    return ret;
}

void utf32_to_utf8(const char32_t* src, size_t src_len, char* dst)
{
    if (src == NULL || src_len == 0 || dst == NULL) {
        return;
    }

    const char32_t *cur_utf32 = src;
    const char32_t *end_utf32 = src + src_len;
    char *cur = dst;
    while (cur_utf32 < end_utf32) {
        size_t len = utf32_codepoint_utf8_length(*cur_utf32);
        utf32_codepoint_to_utf8((uint8_t *)cur, *cur_utf32++, len);
        cur += len;
    }
    *cur = '\0';
}





int strcmp16(const char16_t *s1, const char16_t *s2)
{
  char16_t ch;
  int d = 0;

  while ( 1 ) {
    d = (int)(ch = *s1++) - (int)*s2++;
    if ( d || !ch )
      break;
  }

  return d;
}

int strncmp16(const char16_t *s1, const char16_t *s2, size_t n)
{
  char16_t ch;
  int d = 0;

  while ( n-- ) {
    d = (int)(ch = *s1++) - (int)*s2++;
    if ( d || !ch )
      break;
  }

  return d;
}

char16_t *strcpy16(char16_t *dst, const char16_t *src)
{
  char16_t *q = dst;
  const char16_t *p = src;
  char16_t ch;

  do {
    *q++ = ch = *p++;
  } while ( ch );

  return dst;
}

size_t strlen16(const char16_t *s)
{
  const char16_t *ss = s;
  while ( *ss )
    ss++;
  return ss-s;
}


char16_t *strncpy16(char16_t *dst, const char16_t *src, size_t n)
{
  char16_t *q = dst;
  const char16_t *p = src;
  char ch;

  while (n) {
    n--;
    *q++ = ch = *p++;
    if ( !ch )
      break;
  }

  *q = 0;

  return dst;
}

size_t strnlen16(const char16_t *s, size_t maxlen)
{
  const char16_t *ss = s;

  

  while ((maxlen > 0) && *ss) {
    ss++;
    maxlen--;
  }
  return ss-s;
}

int strzcmp16(const char16_t *s1, size_t n1, const char16_t *s2, size_t n2)
{
    const char16_t* e1 = s1+n1;
    const char16_t* e2 = s2+n2;

    while (s1 < e1 && s2 < e2) {
        const int d = (int)*s1++ - (int)*s2++;
        if (d) {
            return d;
        }
    }

    return n1 < n2
        ? (0 - (int)*s2)
        : (n1 > n2
           ? ((int)*s1 - 0)
           : 0);
}

int strzcmp16_h_n(const char16_t *s1H, size_t n1, const char16_t *s2N, size_t n2)
{
    const char16_t* e1 = s1H+n1;
    const char16_t* e2 = s2N+n2;

    while (s1H < e1 && s2N < e2) {
        const char16_t c2 = ntohs(*s2N);
        const int d = (int)*s1H++ - (int)c2;
        s2N++;
        if (d) {
            return d;
        }
    }

    return n1 < n2
        ? (0 - (int)ntohs(*s2N))
        : (n1 > n2
           ? ((int)*s1H - 0)
           : 0);
}

void utf16_to_utf8(const char16_t* src, size_t src_len, char* dst)
{
    if (src == NULL || src_len == 0 || dst == NULL) {
        return;
    }

    const char16_t* cur_utf16 = src;
    const char16_t* const end_utf16 = src + src_len;
    char *cur = dst;
    while (cur_utf16 < end_utf16) {
        char32_t utf32;
        
        if ((*cur_utf16 & 0xFC00) == 0xD800) {
            utf32 = (*cur_utf16++ - 0xD800) << 10;
            utf32 |= *cur_utf16++ - 0xDC00;
            utf32 += 0x10000;
        } else {
            utf32 = (char32_t) *cur_utf16++;
        }
        const size_t len = utf32_codepoint_utf8_length(utf32);
        utf32_codepoint_to_utf8((uint8_t*)cur, utf32, len);
        cur += len;
    }
    *cur = '\0';
}





ssize_t utf8_length(const char *src)
{
    const char *cur = src;
    size_t ret = 0;
    while (*cur != '\0') {
        const char first_char = *cur++;
        if ((first_char & 0x80) == 0) { 
            ret += 1;
            continue;
        }
        
        
        if ((first_char & 0x40) == 0) {
            return -1;
        }

        int32_t mask, to_ignore_mask;
        size_t num_to_read = 0;
        char32_t utf32 = 0;
        for (num_to_read = 1, mask = 0x40, to_ignore_mask = 0x80;
             num_to_read < 5 && (first_char & mask);
             num_to_read++, to_ignore_mask |= mask, mask >>= 1) {
            if ((*cur & 0xC0) != 0x80) { 
                return -1;
            }
            
            utf32 = (utf32 << 6) + (*cur++ & 0x3F);
        }
        
        if (num_to_read == 5) {
            return -1;
        }
        to_ignore_mask |= mask;
        utf32 |= ((~to_ignore_mask) & first_char) << (6 * (num_to_read - 1));
        if (utf32 > kUnicodeMaxCodepoint) {
            return -1;
        }

        ret += num_to_read;
    }
    return ret;
}

ssize_t utf16_to_utf8_length(const char16_t *src, size_t src_len)
{
    if (src == NULL || src_len == 0) {
        return -1;
    }

    size_t ret = 0;
    const char16_t* const end = src + src_len;
    while (src < end) {
        if ((*src & 0xFC00) == 0xD800 && (src + 1) < end
                && (*++src & 0xFC00) == 0xDC00) {
            
            ret += 4;
            src++;
        } else {
            ret += utf32_codepoint_utf8_length((char32_t) *src++);
        }
    }
    return ret;
}










static inline size_t utf8_codepoint_len(uint8_t ch)
{
    return ((0xe5000000 >> ((ch >> 3) & 0x1e)) & 3) + 1;
}

static inline void utf8_shift_and_mask(uint32_t* codePoint, const uint8_t byte)
{
    *codePoint <<= 6;
    *codePoint |= 0x3F & byte;
}

size_t utf8_to_utf32_length(const char *src, size_t src_len)
{
    if (src == NULL || src_len == 0) {
        return 0;
    }
    size_t ret = 0;
    const char* cur;
    const char* end;
    size_t num_to_skip;
    for (cur = src, end = src + src_len, num_to_skip = 1;
         cur < end;
         cur += num_to_skip, ret++) {
        const char first_char = *cur;
        num_to_skip = 1;
        if ((first_char & 0x80) == 0) {  
            continue;
        }
        int32_t mask;

        for (mask = 0x40; (first_char & mask); num_to_skip++, mask >>= 1) {
        }
    }
    return ret;
}

void utf8_to_utf32(const char* src, size_t src_len, char32_t* dst)
{
    if (src == NULL || src_len == 0 || dst == NULL) {
        return;
    }

    const char* cur = src;
    const char* const end = src + src_len;
    char32_t* cur_utf32 = dst;
    while (cur < end) {
        size_t num_read;
        *cur_utf32++ = static_cast<char32_t>(utf32_at_internal(cur, &num_read));
        cur += num_read;
    }
    *cur_utf32 = 0;
}

static inline uint32_t utf8_to_utf32_codepoint(const uint8_t *src, size_t length)
{
    uint32_t unicode;

    switch (length)
    {
        case 1:
            return src[0];
        case 2:
            unicode = src[0] & 0x1f;
            utf8_shift_and_mask(&unicode, src[1]);
            return unicode;
        case 3:
            unicode = src[0] & 0x0f;
            utf8_shift_and_mask(&unicode, src[1]);
            utf8_shift_and_mask(&unicode, src[2]);
            return unicode;
        case 4:
            unicode = src[0] & 0x07;
            utf8_shift_and_mask(&unicode, src[1]);
            utf8_shift_and_mask(&unicode, src[2]);
            utf8_shift_and_mask(&unicode, src[3]);
            return unicode;
        default:
            return 0xffff;
    }

    
}

ssize_t utf8_to_utf16_length(const uint8_t* u8str, size_t u8len)
{
    const uint8_t* const u8end = u8str + u8len;
    const uint8_t* u8cur = u8str;

    
    size_t u16measuredLen = 0;
    while (u8cur < u8end) {
        u16measuredLen++;
        int u8charLen = utf8_codepoint_len(*u8cur);
        uint32_t codepoint = utf8_to_utf32_codepoint(u8cur, u8charLen);
        if (codepoint > 0xFFFF) u16measuredLen++; 
        u8cur += u8charLen;
    }

    



    if (u8cur != u8end) {
        return -1;
    }

    return u16measuredLen;
}

char16_t* utf8_to_utf16_no_null_terminator(const uint8_t* u8str, size_t u8len, char16_t* u16str)
{
    const uint8_t* const u8end = u8str + u8len;
    const uint8_t* u8cur = u8str;
    char16_t* u16cur = u16str;

    while (u8cur < u8end) {
        size_t u8len = utf8_codepoint_len(*u8cur);
        uint32_t codepoint = utf8_to_utf32_codepoint(u8cur, u8len);

        
        if (codepoint <= 0xFFFF) {
            
            *u16cur++ = (char16_t) codepoint;
        } else {
            
            codepoint = codepoint - 0x10000;
            *u16cur++ = (char16_t) ((codepoint >> 10) + 0xD800);
            *u16cur++ = (char16_t) ((codepoint & 0x3FF) + 0xDC00);
        }

        u8cur += u8len;
    }
    return u16cur;
}

void utf8_to_utf16(const uint8_t* u8str, size_t u8len, char16_t* u16str) {
    char16_t* end = utf8_to_utf16_no_null_terminator(u8str, u8len, u16str);
    *end = 0;
}

char16_t* utf8_to_utf16_n(const uint8_t* src, size_t srcLen, char16_t* dst, size_t dstLen) {
    const uint8_t* const u8end = src + srcLen;
    const uint8_t* u8cur = src;
    const uint16_t* const u16end = (const uint16_t* const) dst + dstLen;
    uint16_t* u16cur = (uint16_t*) dst;

    while (u8cur < u8end && u16cur < u16end) {
        size_t u8len = utf8_codepoint_len(*u8cur);
        uint32_t codepoint = utf8_to_utf32_codepoint(u8cur, u8len);

        
        if (codepoint <= 0xFFFF) {
            
            *u16cur++ = (char16_t) codepoint;
        } else {
            
            codepoint = codepoint - 0x10000;
            *u16cur++ = (char16_t) ((codepoint >> 10) + 0xD800);
            if (u16cur >= u16end) {
                
                return (char16_t*) u16cur-1;
            }
            *u16cur++ = (char16_t) ((codepoint & 0x3FF) + 0xDC00);
        }

        u8cur += u8len;
    }
    return (char16_t*) u16cur;
}

}
