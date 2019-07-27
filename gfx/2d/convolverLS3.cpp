



























#include "convolver.h"
#include <algorithm>
#include "skia/SkTypes.h"

#if defined(_MIPS_ARCH_LOONGSON3A)

#include "MMIHelpers.h"

namespace skia {



void ConvolveHorizontally_LS3(const unsigned char* src_data,
                               int begin, int end,
                               const ConvolutionFilter1D& filter,
                               unsigned char* out_row) {
  int tmp, filter_offset, filter_length;
  double zero, mask[3], shuf_50, shuf_fa;

  asm volatile (
    ".set push \n\t"
    ".set arch=loongson3a \n\t"
    "xor %[zero], %[zero], %[zero] \n\t"
    
    
    "li %[tmp], 1 \n\t"
    "dsll32 %[tmp], 0x10 \n\t"
    "daddiu %[tmp], -1 \n\t"
    "dmtc1 %[tmp], %[mask2] \n\t"
    "dsrl %[tmp], 0x10 \n\t"
    "mtc1 %[tmp], %[mask1] \n\t"
    "dsrl %[tmp], 0x10 \n\t"
    "mtc1 %[tmp], %[mask0] \n\t"
    "ori %[tmp], $0, 0x50 \n\t"
    "mtc1 %[tmp], %[shuf_50] \n\t"
    "ori %[tmp], $0, 0xfa \n\t"
    "mtc1 %[tmp], %[shuf_fa] \n\t"
    ".set pop \n\t"
    :[zero]"=f"(zero), [mask0]"=f"(mask[0]),
     [mask1]"=f"(mask[1]), [mask2]"=f"(mask[2]),
     [shuf_50]"=f"(shuf_50), [shuf_fa]"=f"(shuf_fa),
     [tmp]"=&r"(tmp)
  );

  
  for (int out_x = begin; out_x < end; out_x++) {
    const ConvolutionFilter1D::Fixed* filter_values =
        filter.FilterForValue(out_x, &filter_offset, &filter_length);
    double accumh, accuml;
    
    
    const void *row_to_filter =
        reinterpret_cast<const void*>(&src_data[filter_offset << 2]);

    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      _mm_xor(accum, accum, accum)
      ".set pop \n\t"
      :[accumh]"=f"(accumh), [accuml]"=f"(accuml)
    );

    
    for (int filter_x = 0; filter_x < filter_length >> 2; filter_x++) {
      double src16h, src16l, mul_hih, mul_hil, mul_loh, mul_lol;
      double coeffh, coeffl, src8h, src8l, th, tl, coeff16h, coeff16l;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        
        
        "ldc1 %[coeffl], (%[fval]) \n\t"
        "xor %[coeffh], %[coeffh], %[coeffh] \n\t"
        
        _mm_pshuflh(coeff16, coeff, shuf_50)
        
        _mm_punpcklhw(coeff16, coeff16, coeff16)
        
        
        
        "gsldlc1 %[src8h], 0xf(%[rtf]) \n\t"
        "gsldrc1 %[src8h], 0x8(%[rtf]) \n\t"
        "gsldlc1 %[src8l], 0x7(%[rtf]) \n\t"
        "gsldrc1 %[src8l], 0x0(%[rtf]) \n\t"
        
        _mm_punpcklbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        
        _mm_punpckhhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        
        
        
        
        _mm_pshuflh(coeff16, coeff, shuf_fa)
        
        _mm_punpcklhw(coeff16, coeff16, coeff16)
        
        _mm_punpckhbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        
        _mm_punpckhhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        ".set pop \n\t"
        :[th]"=&f"(th), [tl]"=&f"(tl),
         [src8h]"=&f"(src8h), [src8l]"=&f"(src8l),
         [accumh]"+f"(accumh), [accuml]"+f"(accuml),
         [src16h]"=&f"(src16h), [src16l]"=&f"(src16l),
         [coeffh]"=&f"(coeffh), [coeffl]"=&f"(coeffl),
         [coeff16h]"=&f"(coeff16h), [coeff16l]"=&f"(coeff16l),
         [mul_hih]"=&f"(mul_hih), [mul_hil]"=&f"(mul_hil),
         [mul_loh]"=&f"(mul_loh), [mul_lol]"=&f"(mul_lol)
        :[zeroh]"f"(zero), [zerol]"f"(zero),
         [shuf_50]"f"(shuf_50), [shuf_fa]"f"(shuf_fa),
         [fval]"r"(filter_values), [rtf]"r"(row_to_filter)
      );

      
      row_to_filter += 16;
      filter_values += 4;
    }

    
    
    
    
    int r = filter_length & 3;
    if (r) {
      double coeffh, coeffl, th, tl, coeff16h, coeff16l;
      double src8h, src8l, src16h, src16l, mul_hih, mul_hil, mul_loh, mul_lol;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        "ldc1 %[coeffl], (%[fval]) \n\t"
        "xor %[coeffh], %[coeffh], %[coeffh] \n\t"
        
        "and %[coeffl], %[coeffl], %[mask] \n\t"
        _mm_pshuflh(coeff16, coeff, shuf_50)
        _mm_punpcklhw(coeff16, coeff16, coeff16)
        "gsldlc1 %[src8h], 0xf(%[rtf]) \n\t"
        "gsldrc1 %[src8h], 0x8(%[rtf]) \n\t"
        "gsldlc1 %[src8l], 0x7(%[rtf]) \n\t"
        "gsldrc1 %[src8l], 0x0(%[rtf]) \n\t"
        _mm_punpcklbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        _mm_punpckhhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        _mm_punpckhbh(src16, src8, zero)
        _mm_pshuflh(coeff16, coeff, shuf_fa)
        _mm_punpcklhw(coeff16, coeff16, coeff16)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum, accum, t)
        ".set pop \n\t"
        :[th]"=&f"(th), [tl]"=&f"(tl),
         [src8h]"=&f"(src8h), [src8l]"=&f"(src8l),
         [accumh]"+f"(accumh), [accuml]"+f"(accuml),
         [src16h]"=&f"(src16h), [src16l]"=&f"(src16l),
         [coeffh]"=&f"(coeffh), [coeffl]"=&f"(coeffl),
         [coeff16h]"=&f"(coeff16h), [coeff16l]"=&f"(coeff16l),
         [mul_hih]"=&f"(mul_hih), [mul_hil]"=&f"(mul_hil),
         [mul_loh]"=&f"(mul_loh), [mul_lol]"=&f"(mul_lol)
        :[fval]"r"(filter_values), [rtf]"r"(row_to_filter),
         [zeroh]"f"(zero), [zerol]"f"(zero), [mask]"f"(mask[r-1]),
         [shuf_50]"f"(shuf_50), [shuf_fa]"f"(shuf_fa)
      );
    }

    double t, sra;
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      "ori %[tmp], $0, %[sk_sra] \n\t"
      "mtc1 %[tmp], %[sra] \n\t"
      
      _mm_psraw(accum, accum, sra)
      
      _mm_packsswh(accum, accum, zero, t)
      
      _mm_packushb(accum, accum, zero, t)
      
      "swc1 %[accuml], (%[out_row]) \n\t"
      ".set pop \n\t"
      :[sra]"=&f"(sra), [t]"=&f"(t), [tmp]"=&r"(tmp),
       [accumh]"+f"(accumh), [accuml]"+f"(accuml)
      :[sk_sra]"i"(ConvolutionFilter1D::kShiftBits),
       [out_row]"r"(out_row), [zeroh]"f"(zero), [zerol]"f"(zero)
      :"memory"
    );

    out_row += 4;
  }
}





void ConvolveHorizontally4_LS3(const unsigned char* src_data[4],
                                int begin, int end,
                                const ConvolutionFilter1D& filter,
                                unsigned char* out_row[4]) {
  int tmp, filter_offset, filter_length;
  double zero, mask[3], shuf_50, shuf_fa;

  asm volatile (
    ".set push \n\t"
    ".set arch=loongson3a \n\t"
    "xor %[zero], %[zero], %[zero] \n\t"
    
    
    "li %[tmp], 1 \n\t"
    "dsll32 %[tmp], 0x10 \n\t"
    "daddiu %[tmp], -1 \n\t"
    "dmtc1 %[tmp], %[mask2] \n\t"
    "dsrl %[tmp], 0x10 \n\t"
    "mtc1 %[tmp], %[mask1] \n\t"
    "dsrl %[tmp], 0x10 \n\t"
    "mtc1 %[tmp], %[mask0] \n\t"
    "ori %[tmp], $0, 0x50 \n\t"
    "mtc1 %[tmp], %[shuf_50] \n\t"
    "ori %[tmp], $0, 0xfa \n\t"
    "mtc1 %[tmp], %[shuf_fa] \n\t"
    ".set pop \n\t"
    :[zero]"=f"(zero), [mask0]"=f"(mask[0]),
     [mask1]"=f"(mask[1]), [mask2]"=f"(mask[2]),
     [shuf_50]"=f"(shuf_50), [shuf_fa]"=f"(shuf_fa),
     [tmp]"=&r"(tmp)
  );

  
  for (int out_x = begin; out_x < end; out_x++) {
    const ConvolutionFilter1D::Fixed* filter_values =
        filter.FilterForValue(out_x, &filter_offset, &filter_length);
    double accum0h, accum0l, accum1h, accum1l;
    double accum2h, accum2l, accum3h, accum3l;

    
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      _mm_xor(accum0, accum0, accum0)
      _mm_xor(accum1, accum1, accum1)
      _mm_xor(accum2, accum2, accum2)
      _mm_xor(accum3, accum3, accum3)
      ".set pop \n\t"
      :[accum0h]"=f"(accum0h), [accum0l]"=f"(accum0l),
       [accum1h]"=f"(accum1h), [accum1l]"=f"(accum1l),
       [accum2h]"=f"(accum2h), [accum2l]"=f"(accum2l),
       [accum3h]"=f"(accum3h), [accum3l]"=f"(accum3l)
    );

    int start = (filter_offset<<2);
    
    for (int filter_x = 0; filter_x < (filter_length >> 2); filter_x++) {
      double src8h, src8l, src16h, src16l;
      double mul_hih, mul_hil, mul_loh, mul_lol, th, tl;
      double coeffh, coeffl, coeff16loh, coeff16lol, coeff16hih, coeff16hil;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        
        "ldc1 %[coeffl], (%[fval]) \n\t"
        "xor %[coeffh], %[coeffh], %[coeffh] \n\t"
        
        _mm_pshuflh(coeff16lo, coeff, shuf_50)
        
        _mm_punpcklhw(coeff16lo, coeff16lo, coeff16lo)
        
        _mm_pshuflh(coeff16hi, coeff, shuf_fa)
        
        _mm_punpcklhw(coeff16hi, coeff16hi, coeff16hi)
        ".set pop \n\t"
        :[coeffh]"=&f"(coeffh), [coeffl]"=&f"(coeffl),
         [coeff16loh]"=&f"(coeff16loh), [coeff16lol]"=&f"(coeff16lol),
         [coeff16hih]"=&f"(coeff16hih), [coeff16hil]"=&f"(coeff16hil)
        :[fval]"r"(filter_values), [shuf_50]"f"(shuf_50), [shuf_fa]"f"(shuf_fa)
      );

#define ITERATION(_src, _accumh, _accuml)                              \
      asm volatile (                                                   \
        ".set push \n\t"                                               \
        ".set arch=loongson3a \n\t"                                    \
        "gsldlc1 %[src8h], 0xf(%[src]) \n\t"                           \
        "gsldrc1 %[src8h], 0x8(%[src]) \n\t"                           \
        "gsldlc1 %[src8l], 0x7(%[src]) \n\t"                           \
        "gsldrc1 %[src8l], 0x0(%[src]) \n\t"                           \
        _mm_punpcklbh(src16, src8, zero)                               \
        _mm_pmulhh(mul_hi, src16, coeff16lo)                           \
        _mm_pmullh(mul_lo, src16, coeff16lo)                           \
        _mm_punpcklhw(t, mul_lo, mul_hi)                               \
        _mm_paddw(accum, accum, t)                                     \
        _mm_punpckhhw(t, mul_lo, mul_hi)                               \
        _mm_paddw(accum, accum, t)                                     \
        _mm_punpckhbh(src16, src8, zero)                               \
        _mm_pmulhh(mul_hi, src16, coeff16hi)                           \
        _mm_pmullh(mul_lo, src16, coeff16hi)                           \
        _mm_punpcklhw(t, mul_lo, mul_hi)                               \
        _mm_paddw(accum, accum, t)                                     \
        _mm_punpckhhw(t, mul_lo, mul_hi)                               \
        _mm_paddw(accum, accum, t)                                     \
        ".set pop \n\t"                                                \
        :[th]"=&f"(th), [tl]"=&f"(tl),                                 \
         [src8h]"=&f"(src8h), [src8l]"=&f"(src8l),                     \
         [src16h]"=&f"(src16h), [src16l]"=&f"(src16l),                 \
         [mul_hih]"=&f"(mul_hih), [mul_hil]"=&f"(mul_hil),             \
         [mul_loh]"=&f"(mul_loh), [mul_lol]"=&f"(mul_lol),             \
         [accumh]"+f"(_accumh), [accuml]"+f"(_accuml)                  \
        :[zeroh]"f"(zero), [zerol]"f"(zero), [src]"r"(_src),           \
         [coeff16loh]"f"(coeff16loh), [coeff16lol]"f"(coeff16lol),     \
         [coeff16hih]"f"(coeff16hih), [coeff16hil]"f"(coeff16hil)      \
      );

      ITERATION(src_data[0] + start, accum0h, accum0l);
      ITERATION(src_data[1] + start, accum1h, accum1l);
      ITERATION(src_data[2] + start, accum2h, accum2l);
      ITERATION(src_data[3] + start, accum3h, accum3l);

      start += 16;
      filter_values += 4;
    }

    int r = filter_length & 3;
    if (r) {
      double src8h, src8l, src16h, src16l;
      double mul_hih, mul_hil, mul_loh, mul_lol, th, tl;
      double coeffh, coeffl, coeff16loh, coeff16lol, coeff16hih, coeff16hil;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        "ldc1 %[coeffl], (%[fval]) \n\t"
        "xor %[coeffh], %[coeffh], %[coeffh] \n\t"
        
        "and %[coeffl], %[coeffl], %[mask] \n\t"
        _mm_pshuflh(coeff16lo, coeff, shuf_50)
        
        _mm_punpcklhw(coeff16lo, coeff16lo, coeff16lo)
        _mm_pshuflh(coeff16hi, coeff, shuf_fa)
        _mm_punpcklhw(coeff16hi, coeff16hi, coeff16hi)
        ".set pop \n\t"
        :[coeffh]"=&f"(coeffh), [coeffl]"=&f"(coeffl),
         [coeff16loh]"=&f"(coeff16loh), [coeff16lol]"=&f"(coeff16lol),
         [coeff16hih]"=&f"(coeff16hih), [coeff16hil]"=&f"(coeff16hil)
        :[fval]"r"(filter_values), [mask]"f"(mask[r-1]),
         [shuf_50]"f"(shuf_50), [shuf_fa]"f"(shuf_fa)
      );

      ITERATION(src_data[0] + start, accum0h, accum0l);
      ITERATION(src_data[1] + start, accum1h, accum1l);
      ITERATION(src_data[2] + start, accum2h, accum2l);
      ITERATION(src_data[3] + start, accum3h, accum3l);
    }

    double t, sra;
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      "ori %[tmp], $0, %[sk_sra] \n\t"
      "mtc1 %[tmp], %[sra] \n\t"
      _mm_psraw(accum0, accum0, sra)
      _mm_packsswh(accum0, accum0, zero, t)
      _mm_packushb(accum0, accum0, zero, t)
      _mm_psraw(accum1, accum1, sra)
      _mm_packsswh(accum1, accum1, zero, t)
      _mm_packushb(accum1, accum1, zero, t)
      _mm_psraw(accum2, accum2, sra)
      _mm_packsswh(accum2, accum2, zero, t)
      _mm_packushb(accum2, accum2, zero, t)
      _mm_psraw(accum3, accum3, sra)
      _mm_packsswh(accum3, accum3, zero, t)
      _mm_packushb(accum3, accum3, zero, t)
      "swc1 %[accum0l], (%[out_row0]) \n\t"
      "swc1 %[accum1l], (%[out_row1]) \n\t"
      "swc1 %[accum2l], (%[out_row2]) \n\t"
      "swc1 %[accum3l], (%[out_row3]) \n\t"
      ".set pop \n\t"
      :[accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
       [accum1h]"+f"(accum1h), [accum1l]"+f"(accum1l),
       [accum2h]"+f"(accum2h), [accum2l]"+f"(accum2l),
       [accum3h]"+f"(accum3h), [accum3l]"+f"(accum3l),
       [sra]"=&f"(sra), [t]"=&f"(t), [tmp]"=&r"(tmp)
      :[zeroh]"f"(zero), [zerol]"f"(zero),
       [out_row0]"r"(out_row[0]), [out_row1]"r"(out_row[1]),
       [out_row2]"r"(out_row[2]), [out_row3]"r"(out_row[3]),
       [sk_sra]"i"(ConvolutionFilter1D::kShiftBits)
      :"memory"
    );

    out_row[0] += 4;
    out_row[1] += 4;
    out_row[2] += 4;
    out_row[3] += 4;
  }
}







template<bool has_alpha>
void ConvolveVertically_LS3_impl(const ConvolutionFilter1D::Fixed* filter_values,
                                  int filter_length,
                                  unsigned char* const* source_data_rows,
                                  int begin, int end,
                                  unsigned char* out_row) {
  uint64_t tmp;
  double zero, sra, coeff16h, coeff16l;
  double accum0h, accum0l, accum1h, accum1l;
  double accum2h, accum2l, accum3h, accum3l;
  const void *src;
  int out_x;

  asm volatile (
    ".set push \n\t"
    ".set arch=loongson3a \n\t"
    "xor %[zero], %[zero], %[zero] \n\t"
    "ori %[tmp], $0, %[sk_sra] \n\t"
    "mtc1 %[tmp], %[sra] \n\t"
    ".set pop \n\t"
    :[zero]"=f"(zero), [sra]"=f"(sra), [tmp]"=&r"(tmp)
    :[sk_sra]"i"(ConvolutionFilter1D::kShiftBits)
  );

  
  for (out_x = begin; out_x + 3 < end; out_x += 4) {
    
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      _mm_xor(accum0, accum0, accum0)
      _mm_xor(accum1, accum1, accum1)
      _mm_xor(accum2, accum2, accum2)
      _mm_xor(accum3, accum3, accum3)
      ".set pop \n\t"
      :[accum0h]"=f"(accum0h), [accum0l]"=f"(accum0l),
       [accum1h]"=f"(accum1h), [accum1l]"=f"(accum1l),
       [accum2h]"=f"(accum2h), [accum2l]"=f"(accum2l),
       [accum3h]"=f"(accum3h), [accum3l]"=f"(accum3l)
    );

    
    for (int filter_y = 0; filter_y < filter_length; filter_y++) {
      double src8h, src8l, src16h, src16l;
      double mul_hih, mul_hil, mul_loh, mul_lol, th, tl;

      src = reinterpret_cast<const void*>(
          &source_data_rows[filter_y][out_x << 2]);

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        
        
        "mtc1 %[fval], %[coeff16l] \n\t"
        "pshufh %[coeff16l], %[coeff16l], %[zerol] \n\t"
        "mov.d %[coeff16h], %[coeff16l] \n\t"
        
        
        "gsldlc1 %[src8h], 0xf(%[src]) \n\t"
        "gsldrc1 %[src8h], 0x8(%[src]) \n\t"
        "gsldlc1 %[src8l], 0x7(%[src]) \n\t"
        "gsldrc1 %[src8l], 0x0(%[src]) \n\t"
        
        
        
        _mm_punpcklbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum0, accum0, t)
        
        _mm_punpckhhw(t, mul_lo, mul_hi)
        _mm_paddw(accum1, accum1, t)
        
        
        
        _mm_punpckhbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        ".set pop \n\t"
        :[th]"=&f"(th), [tl]"=&f"(tl),
         [src8h]"=&f"(src8h), [src8l]"=&f"(src8l),
         [src16h]"=&f"(src16h), [src16l]"=&f"(src16l),
         [mul_hih]"=&f"(mul_hih), [mul_hil]"=&f"(mul_hil),
         [mul_loh]"=&f"(mul_loh), [mul_lol]"=&f"(mul_lol),
         [accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
         [accum1h]"+f"(accum1h), [accum1l]"+f"(accum1l),
         [coeff16h]"=&f"(coeff16h), [coeff16l]"=&f"(coeff16l)
        :[zeroh]"f"(zero), [zerol]"f"(zero),
         [fval]"r"(filter_values[filter_y]),
         [src]"r"(src)
      );

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum2, accum2, t)
        
        _mm_punpckhhw(t, mul_lo, mul_hi)
        _mm_paddw(accum3, accum3, t)
        ".set pop \n\t"
        :[th]"=&f"(th), [tl]"=&f"(tl),
         [mul_hih]"+f"(mul_hih), [mul_hil]"+f"(mul_hil),
         [mul_loh]"+f"(mul_loh), [mul_lol]"+f"(mul_lol),
         [accum2h]"+f"(accum2h), [accum2l]"+f"(accum2l),
         [accum3h]"+f"(accum3h), [accum3l]"+f"(accum3l)
      );
    }

    double t;
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      
      _mm_psraw(accum0, accum0, sra)
      _mm_psraw(accum1, accum1, sra)
      _mm_psraw(accum2, accum2, sra)
      _mm_psraw(accum3, accum3, sra)
      
      
      _mm_packsswh(accum0, accum0, accum1, t)
      
      _mm_packsswh(accum2, accum2, accum3, t)
      
      
      _mm_packushb(accum0, accum0, accum2, t)
      ".set pop \n\t"
      :[accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
       [accum1h]"+f"(accum1h), [accum1l]"+f"(accum1l),
       [accum2h]"+f"(accum2h), [accum2l]"+f"(accum2l),
       [accum3h]"+f"(accum3h), [accum3l]"+f"(accum3l),
       [t]"=&f"(t)
      :[sra]"f"(sra)
    );

    if (has_alpha) {
      double ah, al, bh, bl, srl8, srl16, sll24;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        "li %[tmp], 8 \n\t"
        "mtc1 %[tmp], %[srl8] \n\t"
        "li %[tmp], 16 \n\t"
        "mtc1 %[tmp], %[srl16] \n\t"
        "li %[tmp], 24 \n\t"
        "mtc1 %[tmp], %[sll24] \n\t"
        
        
        _mm_psraw(a, accum0, srl8)
        
        _mm_pmaxub(b, a, accum0) 
        
        _mm_psrlw(a, accum0, srl16)
        
        _mm_pmaxub(b, a, b) 
        
        _mm_psllw(b, b, sll24)
        
        
        _mm_pmaxub(accum0, b, accum0)
        ".set pop \n\t"
        :[accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
         [tmp]"=&r"(tmp), [ah]"=&f"(ah), [al]"=&f"(al),
         [bh]"=&f"(bh), [bl]"=&f"(bl), [srl8]"=&f"(srl8),
         [srl16]"=&f"(srl16), [sll24]"=&f"(sll24)
      );
    } else {
      double maskh, maskl;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        
        "li %[tmp], 0xff000000 \n\t"
        "mtc1 %[tmp], %[maskl] \n\t"
        "punpcklwd %[maskl], %[maskl], %[maskl] \n\t"
        "mov.d %[maskh], %[maskl] \n\t"
        _mm_or(accum0, accum0, mask)
      ".set pop \n\t"
      :[maskh]"=&f"(maskh), [maskl]"=&f"(maskl),
       [accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
       [tmp]"=&r"(tmp)
      );
    }

    
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      "gssdlc1 %[accum0h], 0xf(%[out_row]) \n\t"
      "gssdrc1 %[accum0h], 0x8(%[out_row]) \n\t"
      "gssdlc1 %[accum0l], 0x7(%[out_row]) \n\t"
      "gssdrc1 %[accum0l], 0x0(%[out_row]) \n\t"
      ".set pop \n\t"
      ::[accum0h]"f"(accum0h), [accum0l]"f"(accum0l),
        [out_row]"r"(out_row)
      :"memory"
    );
    out_row += 16;
  }

  
  
  int r = end - out_x;
  if (r > 0) {
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      _mm_xor(accum0, accum0, accum0)
      _mm_xor(accum1, accum1, accum1)
      _mm_xor(accum2, accum2, accum2)
      ".set pop \n\t"
      :[accum0h]"=&f"(accum0h), [accum0l]"=&f"(accum0l),
       [accum1h]"=&f"(accum1h), [accum1l]"=&f"(accum1l),
       [accum2h]"=&f"(accum2h), [accum2l]"=&f"(accum2l)
    );
    for (int filter_y = 0; filter_y < filter_length; ++filter_y) {
      double src8h, src8l, src16h, src16l;
      double th, tl, mul_hih, mul_hil, mul_loh, mul_lol;
      src = reinterpret_cast<const void*>(
          &source_data_rows[filter_y][out_x<<2]);

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        "mtc1 %[fval], %[coeff16l] \n\t"
        "pshufh %[coeff16l], %[coeff16l], %[zerol] \n\t"
        "mov.d %[coeff16h], %[coeff16l] \n\t"
        
        "gsldlc1 %[src8h], 0xf(%[src]) \n\t"
        "gsldrc1 %[src8h], 0x8(%[src]) \n\t"
        "gsldlc1 %[src8l], 0x7(%[src]) \n\t"
        "gsldrc1 %[src8l], 0x0(%[src]) \n\t"
        
        _mm_punpcklbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum0, accum0, t)
        
        _mm_punpckhhw(t, mul_lo, mul_hi)
        _mm_paddw(accum1, accum1, t)
        
        _mm_punpckhbh(src16, src8, zero)
        _mm_pmulhh(mul_hi, src16, coeff16)
        _mm_pmullh(mul_lo, src16, coeff16)
        
        _mm_punpcklhw(t, mul_lo, mul_hi)
        _mm_paddw(accum2, accum2, t)
        ".set pop \n\t"
        :[th]"=&f"(th), [tl]"=&f"(tl),
         [src8h]"=&f"(src8h), [src8l]"=&f"(src8l),
         [src16h]"=&f"(src16h), [src16l]"=&f"(src16l),
         [mul_hih]"=&f"(mul_hih), [mul_hil]"=&f"(mul_hil),
         [mul_loh]"=&f"(mul_loh), [mul_lol]"=&f"(mul_lol),
         [accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
         [accum1h]"+f"(accum1h), [accum1l]"+f"(accum1l),
         [accum2h]"+f"(accum2h), [accum2l]"+f"(accum2l),
         [coeff16h]"=&f"(coeff16h), [coeff16l]"=&f"(coeff16l)
        :[zeroh]"f"(zero), [zerol]"f"(zero),
         [fval]"r"(filter_values[filter_y]),
         [src]"r"(src)
      );
    }

    double t;
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      _mm_psraw(accum0, accum0, sra)
      _mm_psraw(accum1, accum1, sra)
      _mm_psraw(accum2, accum2, sra)
      
      _mm_packsswh(accum0, accum0, accum1, t)
      
      _mm_packsswh(accum2, accum2, zero, t)
      
      _mm_packushb(accum0, accum0, accum2, t)
      ".set pop \n\t"
      :[accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
       [accum1h]"+f"(accum1h), [accum1l]"+f"(accum1l),
       [accum2h]"+f"(accum2h), [accum2l]"+f"(accum2l),
       [t]"=&f"(t)
      :[zeroh]"f"(zero), [zerol]"f"(zero), [sra]"f"(sra)
    );
    if (has_alpha) {
      double ah, al, bh, bl, srl8, srl16, sll24;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        "li %[tmp], 8 \n\t"
        "mtc1 %[tmp], %[srl8] \n\t"
        "li %[tmp], 16 \n\t"
        "mtc1 %[tmp], %[srl16] \n\t"
        "li %[tmp], 24 \n\t"
        "mtc1 %[tmp], %[sll24] \n\t"
        
        _mm_psrlw(a, accum0, srl8)
        
        _mm_pmaxub(b, a, accum0) 
        
        _mm_psrlw(a, accum0, srl16)
        
        _mm_pmaxub(b, a, b) 
        
        _mm_psllw(b, b, sll24)
        _mm_pmaxub(accum0, b, accum0)
        ".set pop \n\t"
        :[ah]"=&f"(ah), [al]"=&f"(al), [bh]"=&f"(bh), [bl]"=&f"(bl),
         [accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l), [tmp]"=&r"(tmp),
         [srl8]"=&f"(srl8), [srl16]"=&f"(srl16), [sll24]"=&f"(sll24)
      );
    } else {
      double maskh, maskl;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        
        "li %[tmp], 0xff000000 \n\t"
        "mtc1 %[tmp], %[maskl] \n\t"
        "punpcklwd %[maskl], %[maskl], %[maskl] \n\t"
        "mov.d %[maskh], %[maskl] \n\t"
        _mm_or(accum0, accum0, mask)
        ".set pop \n\t"
        :[maskh]"=&f"(maskh), [maskl]"=&f"(maskl),
         [accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l),
         [tmp]"=&r"(tmp)
      );
    }

    double s4, s64;
    asm volatile (
      ".set push \n\t"
      ".set arch=loongson3a \n\t"
      "li %[tmp], 4 \n\t"
      "mtc1 %[tmp], %[s4] \n\t"
      "li %[tmp], 64 \n\t"
      "mtc1 %[tmp], %[s64] \n\t"
      ".set pop \n\t"
      :[s4]"=f"(s4), [s64]"=f"(s64),
       [tmp]"=&r"(tmp)
    );
    for (; out_x < end; out_x++) {
      double t;

      asm volatile (
        ".set push \n\t"
        ".set arch=loongson3a \n\t"
        "swc1 %[accum0l], (%[out_row]) \n\t"
        _mm_psrlq(accum0, accum0, s4, s64, t)
        ".set pop \n\t"
        :[t]"=&f"(t),
         [accum0h]"+f"(accum0h), [accum0l]"+f"(accum0l)
        :[out_row]"r"(out_row), [s4]"f"(s4), [s64]"f"(s64)
        :"memory"
      );
      out_row += 4;
    }
  }
}

void ConvolveVertically_LS3(const ConvolutionFilter1D::Fixed* filter_values,
                             int filter_length,
                             unsigned char* const* source_data_rows,
                             int begin, int end,
                             unsigned char* out_row, bool has_alpha) {
  if (has_alpha) {
    ConvolveVertically_LS3_impl<true>(filter_values, filter_length,
                                       source_data_rows, begin, end, out_row);
  } else {
    ConvolveVertically_LS3_impl<false>(filter_values, filter_length,
                                       source_data_rows, begin, end, out_row);
  }
}

}  

#endif 
