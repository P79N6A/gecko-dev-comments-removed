




















#ifdef MAKECRCH
#  include <stdio.h>
#  ifndef DYNAMIC_CRC_TABLE
#    define DYNAMIC_CRC_TABLE
#  endif 
#endif 

#include "zutil.h"      

#define local static


#ifndef NOBYFOUR
#  ifdef STDC           
#    include <limits.h>
#    define BYFOUR
#    if (UINT_MAX == 0xffffffffUL)
       typedef unsigned int u4;
#    else
#      if (ULONG_MAX == 0xffffffffUL)
         typedef unsigned long u4;
#      else
#        if (USHRT_MAX == 0xffffffffUL)
           typedef unsigned short u4;
#        else
#          undef BYFOUR     /* can't find a four-byte integer type! */
#        endif
#      endif
#    endif
#  endif 
#endif 


#ifdef BYFOUR
#  define REV(w) (((w)>>24)+(((w)>>8)&0xff00)+ \
                (((w)&0xff00)<<8)+(((w)&0xff)<<24))
   local unsigned long crc32_little OF((unsigned long,
                        const unsigned char FAR *, unsigned));
   local unsigned long crc32_big OF((unsigned long,
                        const unsigned char FAR *, unsigned));
#  define TBLS 8
#else
#  define TBLS 1
#endif 


local unsigned long gf2_matrix_times OF((unsigned long *mat,
                                         unsigned long vec));
local void gf2_matrix_square OF((unsigned long *square, unsigned long *mat));

#ifdef DYNAMIC_CRC_TABLE

local volatile int crc_table_empty = 1;
local unsigned long FAR crc_table[TBLS][256];
local void make_crc_table OF((void));
#ifdef MAKECRCH
   local void write_table OF((FILE *, const unsigned long FAR *));
#endif 


























local void make_crc_table()
{
    unsigned long c;
    int n, k;
    unsigned long poly;                 
    
    static volatile int first = 1;      
    static const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

    


    if (first) {
        first = 0;

        
        poly = 0UL;
        for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
            poly |= 1UL << (31 - p[n]);

        
        for (n = 0; n < 256; n++) {
            c = (unsigned long)n;
            for (k = 0; k < 8; k++)
                c = c & 1 ? poly ^ (c >> 1) : c >> 1;
            crc_table[0][n] = c;
        }

#ifdef BYFOUR
        

        for (n = 0; n < 256; n++) {
            c = crc_table[0][n];
            crc_table[4][n] = REV(c);
            for (k = 1; k < 4; k++) {
                c = crc_table[0][c & 0xff] ^ (c >> 8);
                crc_table[k][n] = c;
                crc_table[k + 4][n] = REV(c);
            }
        }
#endif 

        crc_table_empty = 0;
    }
    else {      
        
        while (crc_table_empty)
            ;
    }

#ifdef MAKECRCH
    
    {
        FILE *out;

        out = fopen("crc32.h", "w");
        if (out == NULL) return;
        fprintf(out, "/* crc32.h -- tables for rapid CRC calculation\n");
        fprintf(out, " * Generated automatically by crc32.c\n */\n\n");
        fprintf(out, "local const unsigned long FAR ");
        fprintf(out, "crc_table[TBLS][256] =\n{\n  {\n");
        write_table(out, crc_table[0]);
#  ifdef BYFOUR
        fprintf(out, "#ifdef BYFOUR\n");
        for (k = 1; k < 8; k++) {
            fprintf(out, "  },\n  {\n");
            write_table(out, crc_table[k]);
        }
        fprintf(out, "#endif\n");
#  endif 
        fprintf(out, "  }\n};\n");
        fclose(out);
    }
#endif 
}

#ifdef MAKECRCH
local void write_table(out, table)
    FILE *out;
    const unsigned long FAR *table;
{
    int n;

    for (n = 0; n < 256; n++)
        fprintf(out, "%s0x%08lxUL%s", n % 5 ? "" : "    ", table[n],
                n == 255 ? "\n" : (n % 5 == 4 ? ",\n" : ", "));
}
#endif 

#else 



#include "crc32.h"
#endif 




const unsigned long FAR * ZEXPORT get_crc_table()
{
#ifdef DYNAMIC_CRC_TABLE
    if (crc_table_empty)
        make_crc_table();
#endif 
    return (const unsigned long FAR *)crc_table;
}


#define DO1 crc = crc_table[0][((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
#define DO8 DO1; DO1; DO1; DO1; DO1; DO1; DO1; DO1


unsigned long ZEXPORT crc32(crc, buf, len)
    unsigned long crc;
    const unsigned char FAR *buf;
    unsigned len;
{
    if (buf == Z_NULL) return 0UL;

#ifdef DYNAMIC_CRC_TABLE
    if (crc_table_empty)
        make_crc_table();
#endif 

#ifdef BYFOUR
    if (sizeof(void *) == sizeof(ptrdiff_t)) {
        u4 endian;

        endian = 1;
        if (*((unsigned char *)(&endian)))
            return crc32_little(crc, buf, len);
        else
            return crc32_big(crc, buf, len);
    }
#endif 
    crc = crc ^ 0xffffffffUL;
    while (len >= 8) {
        DO8;
        len -= 8;
    }
    if (len) do {
        DO1;
    } while (--len);
    return crc ^ 0xffffffffUL;
}

#ifdef BYFOUR


#define DOLIT4 c ^= *buf4++; \
        c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
            crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4


local unsigned long crc32_little(crc, buf, len)
    unsigned long crc;
    const unsigned char FAR *buf;
    unsigned len;
{
    register u4 c;
    register const u4 FAR *buf4;

    c = (u4)crc;
    c = ~c;
    while (len && ((ptrdiff_t)buf & 3)) {
        c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
        len--;
    }

    buf4 = (const u4 FAR *)(const void FAR *)buf;
    while (len >= 32) {
        DOLIT32;
        len -= 32;
    }
    while (len >= 4) {
        DOLIT4;
        len -= 4;
    }
    buf = (const unsigned char FAR *)buf4;

    if (len) do {
        c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
    } while (--len);
    c = ~c;
    return (unsigned long)c;
}


#define DOBIG4 c ^= *++buf4; \
        c = crc_table[4][c & 0xff] ^ crc_table[5][(c >> 8) & 0xff] ^ \
            crc_table[6][(c >> 16) & 0xff] ^ crc_table[7][c >> 24]
#define DOBIG32 DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4


local unsigned long crc32_big(crc, buf, len)
    unsigned long crc;
    const unsigned char FAR *buf;
    unsigned len;
{
    register u4 c;
    register const u4 FAR *buf4;

    c = REV((u4)crc);
    c = ~c;
    while (len && ((ptrdiff_t)buf & 3)) {
        c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
        len--;
    }

    buf4 = (const u4 FAR *)(const void FAR *)buf;
    buf4--;
    while (len >= 32) {
        DOBIG32;
        len -= 32;
    }
    while (len >= 4) {
        DOBIG4;
        len -= 4;
    }
    buf4++;
    buf = (const unsigned char FAR *)buf4;

    if (len) do {
        c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
    } while (--len);
    c = ~c;
    return (unsigned long)(REV(c));
}

#endif 

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */


local unsigned long gf2_matrix_times(mat, vec)
    unsigned long *mat;
    unsigned long vec;
{
    unsigned long sum;

    sum = 0;
    while (vec) {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}


local void gf2_matrix_square(square, mat)
    unsigned long *square;
    unsigned long *mat;
{
    int n;

    for (n = 0; n < GF2_DIM; n++)
        square[n] = gf2_matrix_times(mat, mat[n]);
}


uLong ZEXPORT crc32_combine(crc1, crc2, len2)
    uLong crc1;
    uLong crc2;
    z_off_t len2;
{
    int n;
    unsigned long row;
    unsigned long even[GF2_DIM];    
    unsigned long odd[GF2_DIM];     

    
    if (len2 == 0)
        return crc1;

    
    odd[0] = 0xedb88320L;           
    row = 1;
    for (n = 1; n < GF2_DIM; n++) {
        odd[n] = row;
        row <<= 1;
    }

    
    gf2_matrix_square(even, odd);

    
    gf2_matrix_square(odd, even);

    

    do {
        
        gf2_matrix_square(even, odd);
        if (len2 & 1)
            crc1 = gf2_matrix_times(even, crc1);
        len2 >>= 1;

        
        if (len2 == 0)
            break;

        
        gf2_matrix_square(odd, even);
        if (len2 & 1)
            crc1 = gf2_matrix_times(odd, crc1);
        len2 >>= 1;

        
    } while (len2 != 0);

    
    crc1 ^= crc2;
    return crc1;
}
