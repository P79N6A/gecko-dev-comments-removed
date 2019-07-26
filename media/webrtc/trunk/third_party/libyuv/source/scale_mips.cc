









#include "libyuv/basic_types.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_MIPS) && \
    defined(__mips_dsp) && (__mips_dsp_rev >= 2)

void ScaleRowDown2_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                              uint8* dst, int dst_width) {
  __asm__ __volatile__(
    ".set push                                     \n"
    ".set noreorder                                \n"

    "srl            $t9, %[dst_width], 4           \n"  
    "beqz           $t9, 2f                        \n"
    " nop                                          \n"

    ".p2align       2                              \n"
  "1:                                              \n"
    "lw             $t0, 0(%[src_ptr])             \n"  
    "lw             $t1, 4(%[src_ptr])             \n"  
    "lw             $t2, 8(%[src_ptr])             \n"  
    "lw             $t3, 12(%[src_ptr])            \n"  
    "lw             $t4, 16(%[src_ptr])            \n"  
    "lw             $t5, 20(%[src_ptr])            \n"  
    "lw             $t6, 24(%[src_ptr])            \n"  
    "lw             $t7, 28(%[src_ptr])            \n"  
    
    "precr.qb.ph    $t8, $t1, $t0                  \n"  
    "precr.qb.ph    $t0, $t3, $t2                  \n"  
    "precr.qb.ph    $t1, $t5, $t4                  \n"  
    "precr.qb.ph    $t2, $t7, $t6                  \n"  
    "addiu          %[src_ptr], %[src_ptr], 32     \n"
    "addiu          $t9, $t9, -1                   \n"
    "sw             $t8, 0(%[dst])                 \n"
    "sw             $t0, 4(%[dst])                 \n"
    "sw             $t1, 8(%[dst])                 \n"
    "sw             $t2, 12(%[dst])                \n"
    "bgtz           $t9, 1b                        \n"
    " addiu         %[dst], %[dst], 16             \n"

  "2:                                              \n"
    "andi           $t9, %[dst_width], 0xf         \n"  
    "beqz           $t9, 3f                        \n"
    " nop                                          \n"

  "21:                                             \n"
    "lbu            $t0, 0(%[src_ptr])             \n"
    "addiu          %[src_ptr], %[src_ptr], 2      \n"
    "addiu          $t9, $t9, -1                   \n"
    "sb             $t0, 0(%[dst])                 \n"
    "bgtz           $t9, 21b                       \n"
    " addiu         %[dst], %[dst], 1              \n"

  "3:                                              \n"
    ".set pop                                      \n"
  : [src_ptr] "+r" (src_ptr),
    [dst] "+r" (dst)
  : [dst_width] "r" (dst_width)
  : "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "t8", "t9"
  );
}

void ScaleRowDown2Box_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                                 uint8* dst, int dst_width) {
  const uint8* t = src_ptr + src_stride;

  __asm__ __volatile__ (
    ".set push                                    \n"
    ".set noreorder                               \n"

    "srl            $t9, %[dst_width], 3          \n"  
    "bltz           $t9, 2f                       \n"
    " nop                                         \n"

    ".p2align       2                             \n"
  "1:                                             \n"
    "lw             $t0, 0(%[src_ptr])            \n"  
    "lw             $t1, 4(%[src_ptr])            \n"  
    "lw             $t2, 8(%[src_ptr])            \n"  
    "lw             $t3, 12(%[src_ptr])           \n"  
    "lw             $t4, 0(%[t])                  \n"  
    "lw             $t5, 4(%[t])                  \n"  
    "lw             $t6, 8(%[t])                  \n"  
    "lw             $t7, 12(%[t])                 \n"  
    "addiu          $t9, $t9, -1                  \n"
    "srl            $t8, $t0, 16                  \n"  
    "ins            $t0, $t4, 16, 16              \n"  
    "ins            $t4, $t8, 0, 16               \n"  
    "raddu.w.qb     $t0, $t0                      \n"  
    "raddu.w.qb     $t4, $t4                      \n"  
    "shra_r.w       $t0, $t0, 2                   \n"  
    "shra_r.w       $t4, $t4, 2                   \n"  
    "srl            $t8, $t1, 16                  \n"  
    "ins            $t1, $t5, 16, 16              \n"  
    "ins            $t5, $t8, 0, 16               \n"  
    "raddu.w.qb     $t1, $t1                      \n"  
    "raddu.w.qb     $t5, $t5                      \n"  
    "shra_r.w       $t1, $t1, 2                   \n"  
    "shra_r.w       $t5, $t5, 2                   \n"  
    "srl            $t8, $t2, 16                  \n"  
    "ins            $t2, $t6, 16, 16              \n"  
    "ins            $t6, $t8, 0, 16               \n"  
    "raddu.w.qb     $t2, $t2                      \n"  
    "raddu.w.qb     $t6, $t6                      \n"  
    "shra_r.w       $t2, $t2, 2                   \n"  
    "shra_r.w       $t6, $t6, 2                   \n"  
    "srl            $t8, $t3, 16                  \n"  
    "ins            $t3, $t7, 16, 16              \n"  
    "ins            $t7, $t8, 0, 16               \n"  
    "raddu.w.qb     $t3, $t3                      \n"  
    "raddu.w.qb     $t7, $t7                      \n"  
    "shra_r.w       $t3, $t3, 2                   \n"  
    "shra_r.w       $t7, $t7, 2                   \n"  
    "addiu          %[src_ptr], %[src_ptr], 16    \n"
    "addiu          %[t], %[t], 16                \n"
    "sb             $t0, 0(%[dst])                \n"
    "sb             $t4, 1(%[dst])                \n"
    "sb             $t1, 2(%[dst])                \n"
    "sb             $t5, 3(%[dst])                \n"
    "sb             $t2, 4(%[dst])                \n"
    "sb             $t6, 5(%[dst])                \n"
    "sb             $t3, 6(%[dst])                \n"
    "sb             $t7, 7(%[dst])                \n"
    "bgtz           $t9, 1b                       \n"
    " addiu         %[dst], %[dst], 8             \n"

  "2:                                             \n"
    "andi           $t9, %[dst_width], 0x7        \n"  
    "beqz           $t9, 3f                       \n"
    " nop                                         \n"

    "21:                                          \n"
    "lwr            $t1, 0(%[src_ptr])            \n"
    "lwl            $t1, 3(%[src_ptr])            \n"
    "lwr            $t2, 0(%[t])                  \n"
    "lwl            $t2, 3(%[t])                  \n"
    "srl            $t8, $t1, 16                  \n"
    "ins            $t1, $t2, 16, 16              \n"
    "ins            $t2, $t8, 0, 16               \n"
    "raddu.w.qb     $t1, $t1                      \n"
    "raddu.w.qb     $t2, $t2                      \n"
    "shra_r.w       $t1, $t1, 2                   \n"
    "shra_r.w       $t2, $t2, 2                   \n"
    "sb             $t1, 0(%[dst])                \n"
    "sb             $t2, 1(%[dst])                \n"
    "addiu          %[src_ptr], %[src_ptr], 4     \n"
    "addiu          $t9, $t9, -2                  \n"
    "addiu          %[t], %[t], 4                 \n"
    "bgtz           $t9, 21b                      \n"
    " addiu         %[dst], %[dst], 2             \n"

  "3:                                             \n"
    ".set pop                                     \n"

  : [src_ptr] "+r" (src_ptr),
    [dst] "+r" (dst), [t] "+r" (t)
  : [dst_width] "r" (dst_width)
  : "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "t8", "t9"
  );
}

void ScaleRowDown4_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                              uint8* dst, int dst_width) {
  __asm__ __volatile__ (
      ".set push                                    \n"
      ".set noreorder                               \n"

      "srl            $t9, %[dst_width], 3          \n"
      "beqz           $t9, 2f                       \n"
      " nop                                         \n"

      ".p2align       2                             \n"
     "1:                                            \n"
      "lw             $t1, 0(%[src_ptr])            \n"  
      "lw             $t2, 4(%[src_ptr])            \n"  
      "lw             $t3, 8(%[src_ptr])            \n"  
      "lw             $t4, 12(%[src_ptr])           \n"  
      "lw             $t5, 16(%[src_ptr])           \n"  
      "lw             $t6, 20(%[src_ptr])           \n"  
      "lw             $t7, 24(%[src_ptr])           \n"  
      "lw             $t8, 28(%[src_ptr])           \n"  
      "precr.qb.ph    $t1, $t2, $t1                 \n"  
      "precr.qb.ph    $t2, $t4, $t3                 \n"  
      "precr.qb.ph    $t5, $t6, $t5                 \n"  
      "precr.qb.ph    $t6, $t8, $t7                 \n"  
      "precr.qb.ph    $t1, $t2, $t1                 \n"  
      "precr.qb.ph    $t5, $t6, $t5                 \n"  
      "addiu          %[src_ptr], %[src_ptr], 32    \n"
      "addiu          $t9, $t9, -1                  \n"
      "sw             $t1, 0(%[dst])                \n"
      "sw             $t5, 4(%[dst])                \n"
      "bgtz           $t9, 1b                       \n"
      " addiu         %[dst], %[dst], 8             \n"

    "2:                                             \n"
      "andi           $t9, %[dst_width], 7          \n"  
      "beqz           $t9, 3f                       \n"
      " nop                                         \n"

    "21:                                            \n"
      "lbu            $t1, 0(%[src_ptr])            \n"
      "addiu          %[src_ptr], %[src_ptr], 4     \n"
      "addiu          $t9, $t9, -1                  \n"
      "sb             $t1, 0(%[dst])                \n"
      "bgtz           $t9, 21b                      \n"
      " addiu         %[dst], %[dst], 1             \n"

    "3:                                             \n"
      ".set pop                                     \n"
      : [src_ptr] "+r" (src_ptr),
        [dst] "+r" (dst)
      : [dst_width] "r" (dst_width)
      : "t1", "t2", "t3", "t4", "t5",
        "t6", "t7", "t8", "t9"
  );
}

void ScaleRowDown4Box_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                                 uint8* dst, int dst_width) {
  intptr_t stride = src_stride;
  const uint8* s1 = src_ptr + stride;
  const uint8* s2 = s1 + stride;
  const uint8* s3 = s2 + stride;

  __asm__ __volatile__ (
      ".set push                                  \n"
      ".set noreorder                             \n"

      "srl           $t9, %[dst_width], 1         \n"
      "andi          $t8, %[dst_width], 1         \n"

      ".p2align      2                            \n"
     "1:                                          \n"
      "lw            $t0, 0(%[src_ptr])           \n"  
      "lw            $t1, 0(%[s1])                \n"  
      "lw            $t2, 0(%[s2])                \n"  
      "lw            $t3, 0(%[s3])                \n"  
      "lw            $t4, 4(%[src_ptr])           \n"  
      "lw            $t5, 4(%[s1])                \n"  
      "lw            $t6, 4(%[s2])                \n"  
      "lw            $t7, 4(%[s3])                \n"  
      "raddu.w.qb    $t0, $t0                     \n"  
      "raddu.w.qb    $t1, $t1                     \n"  
      "raddu.w.qb    $t2, $t2                     \n"  
      "raddu.w.qb    $t3, $t3                     \n"  
      "raddu.w.qb    $t4, $t4                     \n"  
      "raddu.w.qb    $t5, $t5                     \n"  
      "raddu.w.qb    $t6, $t6                     \n"  
      "raddu.w.qb    $t7, $t7                     \n"  
      "add           $t0, $t0, $t1                \n"
      "add           $t1, $t2, $t3                \n"
      "add           $t0, $t0, $t1                \n"
      "add           $t4, $t4, $t5                \n"
      "add           $t6, $t6, $t7                \n"
      "add           $t4, $t4, $t6                \n"
      "shra_r.w      $t0, $t0, 4                  \n"
      "shra_r.w      $t4, $t4, 4                  \n"
      "sb            $t0, 0(%[dst])               \n"
      "sb            $t4, 1(%[dst])               \n"
      "addiu         %[src_ptr], %[src_ptr], 8    \n"
      "addiu         %[s1], %[s1], 8              \n"
      "addiu         %[s2], %[s2], 8              \n"
      "addiu         %[s3], %[s3], 8              \n"
      "addiu         $t9, $t9, -1                 \n"
      "bgtz          $t9, 1b                      \n"
      " addiu        %[dst], %[dst], 2            \n"
      "beqz          $t8, 2f                      \n"
      " nop                                       \n"

      "lw            $t0, 0(%[src_ptr])           \n"  
      "lw            $t1, 0(%[s1])                \n"  
      "lw            $t2, 0(%[s2])                \n"  
      "lw            $t3, 0(%[s3])                \n"  
      "raddu.w.qb    $t0, $t0                     \n"  
      "raddu.w.qb    $t1, $t1                     \n"  
      "raddu.w.qb    $t2, $t2                     \n"  
      "raddu.w.qb    $t3, $t3                     \n"  
      "add           $t0, $t0, $t1                \n"
      "add           $t1, $t2, $t3                \n"
      "add           $t0, $t0, $t1                \n"
      "shra_r.w      $t0, $t0, 4                  \n"
      "sb            $t0, 0(%[dst])               \n"

      "2:                                         \n"
      ".set pop                                   \n"

      : [src_ptr] "+r" (src_ptr),
        [dst] "+r" (dst),
        [s1] "+r" (s1),
        [s2] "+r" (s2),
        [s3] "+r" (s3)
      : [dst_width] "r" (dst_width)
      : "t0", "t1", "t2", "t3", "t4", "t5",
        "t6","t7", "t8", "t9"
  );
}

void ScaleRowDown34_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                               uint8* dst, int dst_width) {
  __asm__ __volatile__ (
      ".set push                                          \n"
      ".set noreorder                                     \n"
      ".p2align        2                                  \n"
    "1:                                                   \n"
      "lw              $t1, 0(%[src_ptr])                 \n"  
      "lw              $t2, 4(%[src_ptr])                 \n"  
      "lw              $t3, 8(%[src_ptr])                 \n"  
      "lw              $t4, 12(%[src_ptr])                \n"  
      "lw              $t5, 16(%[src_ptr])                \n"  
      "lw              $t6, 20(%[src_ptr])                \n"  
      "lw              $t7, 24(%[src_ptr])                \n"  
      "lw              $t8, 28(%[src_ptr])                \n"  
      "precrq.qb.ph    $t0, $t2, $t4                      \n"  
      "precrq.qb.ph    $t9, $t6, $t8                      \n"  
      "addiu           %[dst_width], %[dst_width], -24    \n"
      "ins             $t1, $t1, 8, 16                    \n"  
      "ins             $t4, $t0, 8, 16                    \n"  
      "ins             $t5, $t5, 8, 16                    \n"  
      "ins             $t8, $t9, 8, 16                    \n"  
      "addiu           %[src_ptr], %[src_ptr], 32         \n"
      "packrl.ph       $t0, $t3, $t0                      \n"  
      "packrl.ph       $t9, $t7, $t9                      \n"  
      "prepend         $t1, $t2, 8                        \n"  
      "prepend         $t3, $t4, 24                       \n"  
      "prepend         $t5, $t6, 8                        \n"  
      "prepend         $t7, $t8, 24                       \n"  
      "sw              $t1, 0(%[dst])                     \n"
      "sw              $t0, 4(%[dst])                     \n"
      "sw              $t3, 8(%[dst])                     \n"
      "sw              $t5, 12(%[dst])                    \n"
      "sw              $t9, 16(%[dst])                    \n"
      "sw              $t7, 20(%[dst])                    \n"
      "bnez            %[dst_width], 1b                   \n"
      " addiu          %[dst], %[dst], 24                 \n"
      ".set pop                                           \n"
      : [src_ptr] "+r" (src_ptr),
        [dst] "+r" (dst),
        [dst_width] "+r" (dst_width)
      :
      : "t0", "t1", "t2", "t3", "t4", "t5",
        "t6","t7", "t8", "t9"
  );
}

void ScaleRowDown34_0_Box_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                                     uint8* d, int dst_width) {
  __asm__ __volatile__ (
      ".set push                                         \n"
      ".set noreorder                                    \n"
      "repl.ph           $t3, 3                          \n"  

     ".p2align           2                               \n"
    "1:                                                  \n"
      "lw                $t0, 0(%[src_ptr])              \n"  
      "lwx               $t1, %[src_stride](%[src_ptr])  \n"  
      "rotr              $t2, $t0, 8                     \n"  
      "rotr              $t6, $t1, 8                     \n"  
      "muleu_s.ph.qbl    $t4, $t2, $t3                   \n"  
      "muleu_s.ph.qbl    $t5, $t6, $t3                   \n"  
      "andi              $t0, $t2, 0xFFFF                \n"  
      "andi              $t1, $t6, 0xFFFF                \n"  
      "raddu.w.qb        $t0, $t0                        \n"
      "raddu.w.qb        $t1, $t1                        \n"
      "shra_r.w          $t0, $t0, 1                     \n"
      "shra_r.w          $t1, $t1, 1                     \n"
      "preceu.ph.qbr     $t2, $t2                        \n"  
      "preceu.ph.qbr     $t6, $t6                        \n"  
      "rotr              $t2, $t2, 16                    \n"  
      "rotr              $t6, $t6, 16                    \n"  
      "addu.ph           $t2, $t2, $t4                   \n"
      "addu.ph           $t6, $t6, $t5                   \n"
      "sll               $t5, $t0, 1                     \n"
      "add               $t0, $t5, $t0                   \n"
      "shra_r.ph         $t2, $t2, 2                     \n"
      "shra_r.ph         $t6, $t6, 2                     \n"
      "shll.ph           $t4, $t2, 1                     \n"
      "addq.ph           $t4, $t4, $t2                   \n"
      "addu              $t0, $t0, $t1                   \n"
      "addiu             %[src_ptr], %[src_ptr], 4       \n"
      "shra_r.w          $t0, $t0, 2                     \n"
      "addu.ph           $t6, $t6, $t4                   \n"
      "shra_r.ph         $t6, $t6, 2                     \n"
      "srl               $t1, $t6, 16                    \n"
      "addiu             %[dst_width], %[dst_width], -3  \n"
      "sb                $t1, 0(%[d])                    \n"
      "sb                $t0, 1(%[d])                    \n"
      "sb                $t6, 2(%[d])                    \n"
      "bgtz              %[dst_width], 1b                \n"
      " addiu            %[d], %[d], 3                   \n"
    "3:                                                  \n"
      ".set pop                                          \n"
      : [src_ptr] "+r" (src_ptr),
        [src_stride] "+r" (src_stride),
        [d] "+r" (d),
        [dst_width] "+r" (dst_width)
      :
      : "t0", "t1", "t2", "t3",
        "t4", "t5", "t6"
  );
}

void ScaleRowDown34_1_Box_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                                     uint8* d, int dst_width) {
  __asm__ __volatile__ (
      ".set push                                           \n"
      ".set noreorder                                      \n"
      "repl.ph           $t2, 3                            \n"  

      ".p2align          2                                 \n"
    "1:                                                    \n"
      "lw                $t0, 0(%[src_ptr])                \n"  
      "lwx               $t1, %[src_stride](%[src_ptr])    \n"  
      "rotr              $t4, $t0, 8                       \n"  
      "rotr              $t6, $t1, 8                       \n"  
      "muleu_s.ph.qbl    $t3, $t4, $t2                     \n"  
      "muleu_s.ph.qbl    $t5, $t6, $t2                     \n"  
      "andi              $t0, $t4, 0xFFFF                  \n"  
      "andi              $t1, $t6, 0xFFFF                  \n"  
      "raddu.w.qb        $t0, $t0                          \n"
      "raddu.w.qb        $t1, $t1                          \n"
      "shra_r.w          $t0, $t0, 1                       \n"
      "shra_r.w          $t1, $t1, 1                       \n"
      "preceu.ph.qbr     $t4, $t4                          \n"  
      "preceu.ph.qbr     $t6, $t6                          \n"  
      "rotr              $t4, $t4, 16                      \n"  
      "rotr              $t6, $t6, 16                      \n"  
      "addu.ph           $t4, $t4, $t3                     \n"
      "addu.ph           $t6, $t6, $t5                     \n"
      "shra_r.ph         $t6, $t6, 2                       \n"
      "shra_r.ph         $t4, $t4, 2                       \n"
      "addu.ph           $t6, $t6, $t4                     \n"
      "addiu             %[src_ptr], %[src_ptr], 4         \n"
      "shra_r.ph         $t6, $t6, 1                       \n"
      "addu              $t0, $t0, $t1                     \n"
      "addiu             %[dst_width], %[dst_width], -3    \n"
      "shra_r.w          $t0, $t0, 1                       \n"
      "srl               $t1, $t6, 16                      \n"
      "sb                $t1, 0(%[d])                      \n"
      "sb                $t0, 1(%[d])                      \n"
      "sb                $t6, 2(%[d])                      \n"
      "bgtz              %[dst_width], 1b                  \n"
      " addiu            %[d], %[d], 3                     \n"
    "3:                                                    \n"
      ".set pop                                            \n"
      : [src_ptr] "+r" (src_ptr),
        [src_stride] "+r" (src_stride),
        [d] "+r" (d),
        [dst_width] "+r" (dst_width)
      :
      : "t0", "t1", "t2", "t3",
        "t4", "t5", "t6"
  );
}

void ScaleRowDown38_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                               uint8* dst, int dst_width) {
  __asm__ __volatile__ (
      ".set push                                     \n"
      ".set noreorder                                \n"

      ".p2align   2                                  \n"
    "1:                                              \n"
      "lw         $t0, 0(%[src_ptr])                 \n"  
      "lw         $t1, 4(%[src_ptr])                 \n"  
      "lw         $t2, 8(%[src_ptr])                 \n"  
      "lw         $t3, 12(%[src_ptr])                \n"  
      "lw         $t4, 16(%[src_ptr])                \n"  
      "lw         $t5, 20(%[src_ptr])                \n"  
      "lw         $t6, 24(%[src_ptr])                \n"  
      "lw         $t7, 28(%[src_ptr])                \n"  
      "wsbh       $t0, $t0                           \n"  
      "wsbh       $t6, $t6                           \n"  
      "srl        $t0, $t0, 8                        \n"  
      "srl        $t3, $t3, 16                       \n"  
      "srl        $t5, $t5, 16                       \n"  
      "srl        $t7, $t7, 16                       \n"  
      "ins        $t1, $t2, 24, 8                    \n"  
      "ins        $t6, $t5, 0, 8                     \n"  
      "ins        $t1, $t0, 0, 16                    \n"  
      "ins        $t6, $t7, 24, 8                    \n"  
      "prepend    $t2, $t3, 24                       \n"  
      "ins        $t4, $t4, 16, 8                    \n"  
      "ins        $t4, $t2, 0, 16                    \n"  
      "addiu      %[src_ptr], %[src_ptr], 32         \n"
      "addiu      %[dst_width], %[dst_width], -12    \n"
      "addiu      $t8,%[dst_width], -12              \n"
      "sw         $t1, 0(%[dst])                     \n"
      "sw         $t4, 4(%[dst])                     \n"
      "sw         $t6, 8(%[dst])                     \n"
      "bgez       $t8, 1b                            \n"
      " addiu     %[dst], %[dst], 12                 \n"
      ".set pop                                      \n"
      : [src_ptr] "+r" (src_ptr),
        [dst] "+r" (dst),
        [dst_width] "+r" (dst_width)
      :
      : "t0", "t1", "t2", "t3", "t4",
        "t5", "t6", "t7", "t8"
  );
}

void ScaleRowDown38_2_Box_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                                     uint8* dst_ptr, int dst_width) {
  intptr_t stride = src_stride;
  const uint8* t = src_ptr + stride;
  const int c = 0x2AAA;

  __asm__ __volatile__ (
      ".set push                                         \n"
      ".set noreorder                                    \n"

      ".p2align        2                                 \n"
    "1:                                                  \n"
      "lw              $t0, 0(%[src_ptr])                \n"  
      "lw              $t1, 4(%[src_ptr])                \n"  
      "lw              $t2, 0(%[t])                      \n"  
      "lw              $t3, 4(%[t])                      \n"  
      "rotr            $t1, $t1, 16                      \n"  
      "packrl.ph       $t4, $t1, $t3                     \n"  
      "packrl.ph       $t5, $t3, $t1                     \n"  
      "raddu.w.qb      $t4, $t4                          \n"  
      "raddu.w.qb      $t5, $t5                          \n"  
      "precrq.qb.ph    $t6, $t0, $t2                     \n"  
      "precrq.qb.ph    $t6, $t6, $t6                     \n"  
      "srl             $t4, $t4, 2                       \n"  
      "srl             $t6, $t6, 16                      \n"  
      "raddu.w.qb      $t6, $t6                          \n"  
      "addu            $t6, $t5, $t6                     \n"
      "mul             $t6, $t6, %[c]                    \n"  
      "sll             $t0, $t0, 8                       \n"  
      "sll             $t2, $t2, 8                       \n"  
      "raddu.w.qb      $t0, $t0                          \n"  
      "raddu.w.qb      $t2, $t2                          \n"  
      "addu            $t0, $t0, $t2                     \n"
      "mul             $t0, $t0, %[c]                    \n"  
      "addiu           %[src_ptr], %[src_ptr], 8         \n"
      "addiu           %[t], %[t], 8                     \n"
      "addiu           %[dst_width], %[dst_width], -3    \n"
      "addiu           %[dst_ptr], %[dst_ptr], 3         \n"
      "srl             $t6, $t6, 16                      \n"
      "srl             $t0, $t0, 16                      \n"
      "sb              $t4, -1(%[dst_ptr])               \n"
      "sb              $t6, -2(%[dst_ptr])               \n"
      "bgtz            %[dst_width], 1b                  \n"
      " sb             $t0, -3(%[dst_ptr])               \n"
      ".set pop                                          \n"
      : [src_ptr] "+r" (src_ptr),
        [dst_ptr] "+r" (dst_ptr),
        [t] "+r" (t),
        [dst_width] "+r" (dst_width)
      : [c] "r" (c)
      : "t0", "t1", "t2", "t3", "t4", "t5", "t6"
  );
}

void ScaleRowDown38_3_Box_MIPS_DSPR2(const uint8* src_ptr,
                                     ptrdiff_t src_stride,
                                     uint8* dst_ptr, int dst_width) {
  intptr_t stride = src_stride;
  const uint8* s1 = src_ptr + stride;
  stride += stride;
  const uint8* s2 = src_ptr + stride;
  const int c1 = 0x1C71;
  const int c2 = 0x2AAA;

  __asm__ __volatile__ (
      ".set push                                         \n"
      ".set noreorder                                    \n"

      ".p2align        2                                 \n"
    "1:                                                  \n"
      "lw              $t0, 0(%[src_ptr])                \n"  
      "lw              $t1, 4(%[src_ptr])                \n"  
      "lw              $t2, 0(%[s1])                     \n"  
      "lw              $t3, 4(%[s1])                     \n"  
      "lw              $t4, 0(%[s2])                     \n"  
      "lw              $t5, 4(%[s2])                     \n"  
      "rotr            $t1, $t1, 16                      \n"  
      "packrl.ph       $t6, $t1, $t3                     \n"  
      "raddu.w.qb      $t6, $t6                          \n"  
      "packrl.ph       $t7, $t3, $t1                     \n"  
      "raddu.w.qb      $t7, $t7                          \n"  
      "sll             $t8, $t5, 16                      \n"  
      "raddu.w.qb      $t8, $t8                          \n"  
      "addu            $t7, $t7, $t8                     \n"
      "srl             $t8, $t5, 16                      \n"  
      "raddu.w.qb      $t8, $t8                          \n"  
      "addu            $t6, $t6, $t8                     \n"
      "mul             $t6, $t6, %[c2]                   \n"  
      "precrq.qb.ph    $t8, $t0, $t2                     \n"  
      "precrq.qb.ph    $t8, $t8, $t4                     \n"  
      "srl             $t8, $t8, 8                       \n"  
      "raddu.w.qb      $t8, $t8                          \n"  
      "addu            $t7, $t7, $t8                     \n"
      "mul             $t7, $t7, %[c1]                   \n"  
      "sll             $t0, $t0, 8                       \n"  
      "sll             $t2, $t2, 8                       \n"  
      "sll             $t4, $t4, 8                       \n"  
      "raddu.w.qb      $t0, $t0                          \n"
      "raddu.w.qb      $t2, $t2                          \n"
      "raddu.w.qb      $t4, $t4                          \n"
      "addu            $t0, $t0, $t2                     \n"
      "addu            $t0, $t0, $t4                     \n"
      "mul             $t0, $t0, %[c1]                   \n"  
      "addiu           %[src_ptr], %[src_ptr], 8         \n"
      "addiu           %[s1], %[s1], 8                   \n"
      "addiu           %[s2], %[s2], 8                   \n"
      "addiu           %[dst_width], %[dst_width], -3    \n"
      "addiu           %[dst_ptr], %[dst_ptr], 3         \n"
      "srl             $t6, $t6, 16                      \n"
      "srl             $t7, $t7, 16                      \n"
      "srl             $t0, $t0, 16                      \n"
      "sb              $t6, -1(%[dst_ptr])               \n"
      "sb              $t7, -2(%[dst_ptr])               \n"
      "bgtz            %[dst_width], 1b                  \n"
      " sb             $t0, -3(%[dst_ptr])               \n"
      ".set pop                                          \n"
      : [src_ptr] "+r" (src_ptr),
        [dst_ptr] "+r" (dst_ptr),
        [s1] "+r" (s1),
        [s2] "+r" (s2),
        [dst_width] "+r" (dst_width)
      : [c1] "r" (c1), [c2] "r" (c2)
      : "t0", "t1", "t2", "t3", "t4",
        "t5", "t6", "t7", "t8"
  );
}

#endif  

#ifdef __cplusplus
}  
}  
#endif

