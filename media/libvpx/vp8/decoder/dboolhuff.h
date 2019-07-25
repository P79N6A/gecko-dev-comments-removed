










#ifndef DBOOLHUFF_H
#define DBOOLHUFF_H
#include "vpx_ports/config.h"
#include "vpx_ports/mem.h"
#include "vpx/vpx_integer.h"

























#define VP8_BOOL_DECODER_SZ       4096
#define VP8_BOOL_DECODER_MASK     (VP8_BOOL_DECODER_SZ-1)
#define VP8_BOOL_DECODER_PTR_MASK (~(uintptr_t)(VP8_BOOL_DECODER_SZ))

struct vp8_dboolhuff_rtcd_vtable;

typedef struct
{
    unsigned int         lowvalue;
    unsigned int         range;
    unsigned int         value;
    int                  count;
    const unsigned char *user_buffer;
    unsigned int         user_buffer_sz;
    unsigned char       *decode_buffer;
    const unsigned char *read_ptr;
    unsigned char       *write_ptr;
#if CONFIG_RUNTIME_CPU_DETECT
    struct vp8_dboolhuff_rtcd_vtable *rtcd;
#endif
} BOOL_DECODER;

#define prototype_dbool_start(sym) int sym(BOOL_DECODER *br, \
    const unsigned char *source, unsigned int source_sz)
#define prototype_dbool_stop(sym) void sym(BOOL_DECODER *bc)
#define prototype_dbool_fill(sym) void sym(BOOL_DECODER *br)
#define prototype_dbool_debool(sym) int sym(BOOL_DECODER *br, int probability)
#define prototype_dbool_devalue(sym) int sym(BOOL_DECODER *br, int bits);

#if ARCH_ARM
#include "arm/dboolhuff_arm.h"
#endif

#ifndef vp8_dbool_start
#define vp8_dbool_start vp8dx_start_decode_c
#endif

#ifndef vp8_dbool_stop
#define vp8_dbool_stop vp8dx_stop_decode_c
#endif

#ifndef vp8_dbool_fill
#define vp8_dbool_fill vp8dx_bool_decoder_fill_c
#endif

#ifndef vp8_dbool_debool
#define vp8_dbool_debool vp8dx_decode_bool_c
#endif

#ifndef vp8_dbool_devalue
#define vp8_dbool_devalue vp8dx_decode_value_c
#endif

extern prototype_dbool_start(vp8_dbool_start);
extern prototype_dbool_stop(vp8_dbool_stop);
extern prototype_dbool_fill(vp8_dbool_fill);
extern prototype_dbool_debool(vp8_dbool_debool);
extern prototype_dbool_devalue(vp8_dbool_devalue);

typedef prototype_dbool_start((*vp8_dbool_start_fn_t));
typedef prototype_dbool_stop((*vp8_dbool_stop_fn_t));
typedef prototype_dbool_fill((*vp8_dbool_fill_fn_t));
typedef prototype_dbool_debool((*vp8_dbool_debool_fn_t));
typedef prototype_dbool_devalue((*vp8_dbool_devalue_fn_t));

typedef struct vp8_dboolhuff_rtcd_vtable {
    vp8_dbool_start_fn_t   start;
    vp8_dbool_stop_fn_t    stop;
    vp8_dbool_fill_fn_t    fill;
    vp8_dbool_debool_fn_t  debool;
    vp8_dbool_devalue_fn_t devalue;
} vp8_dboolhuff_rtcd_vtable_t;








#define DBOOLHUFF_INVOKE(ctx,fn) vp8_dbool_##fn
#define IF_RTCD(x) NULL


static unsigned char *br_ptr_advance(const unsigned char *_ptr,
                                     unsigned int n)
{
    uintptr_t  ptr = (uintptr_t)_ptr;

    ptr += n;
    ptr &= VP8_BOOL_DECODER_PTR_MASK;

    return (void *)ptr;
}

DECLARE_ALIGNED(16, extern const unsigned int, vp8dx_bitreader_norm[256]);




static int vp8dx_start_decode(BOOL_DECODER *br,
        struct vp8_dboolhuff_rtcd_vtable *rtcd,
        const unsigned char *source, unsigned int source_sz) {
#if CONFIG_RUNTIME_CPU_DETECT
    br->rtcd = rtcd;
#endif
    return DBOOLHUFF_INVOKE(rtcd, start)(br, source, source_sz);
}
static void vp8dx_stop_decode(BOOL_DECODER *br) {
    DBOOLHUFF_INVOKE(br->rtcd, stop)(br);
}
static void vp8dx_bool_decoder_fill(BOOL_DECODER *br) {
    DBOOLHUFF_INVOKE(br->rtcd, fill)(br);
}
static int vp8dx_decode_bool(BOOL_DECODER *br, int probability) {
  





    unsigned int bit = 0;
    unsigned int split;
    unsigned int bigsplit;
    register unsigned int range = br->range;
    register unsigned int value = br->value;

    split = 1 + (((range - 1) * probability) >> 8);
    bigsplit = (split << 8);

    range = split;

    if (value >= bigsplit)
    {
        range = br->range - split;
        value = value - bigsplit;
        bit = 1;
    }

    






    {
        int count = br->count;
        register unsigned int shift = vp8dx_bitreader_norm[range];
        range <<= shift;
        value <<= shift;
        count -= shift;

        if (count <= 0)
        {
            value |= (*br->read_ptr) << (-count);
            br->read_ptr = br_ptr_advance(br->read_ptr, 1);
            count += 8 ;
        }

        br->count = count;
    }
    br->value = value;
    br->range = range;
    return bit;
}

static int vp8_decode_value(BOOL_DECODER *br, int bits)
{
  





    int z = 0;
    int bit;

    for (bit = bits - 1; bit >= 0; bit--)
    {
        z |= (vp8dx_decode_bool(br, 0x80) << bit);
    }

    return z;
}
#endif
