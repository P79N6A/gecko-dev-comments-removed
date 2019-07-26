









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_MIPS) && defined(__mips__)

#include <sgidefs.h>

#if (_MIPS_ISA == _MIPS_ISA_MIPS4) || (_MIPS_ISA == _MIPS_ISA_MIPS5)
#define HAS_MIPS_PREFETCH 1
#endif

#ifdef HAS_COPYROW_MIPS
void CopyRow_MIPS(const uint8* src, uint8* dst, int count) {
  __asm__ __volatile__ (
    ".set      noreorder                         \n"
    ".set      noat                              \n"
    "slti      $at, %[count], 8                  \n"
    "bne       $at ,$zero, $last8                \n"
    "xor       $t8, %[src], %[dst]               \n"
    "andi      $t8, $t8, 0x3                     \n"

    "bne       $t8, $zero, unaligned             \n"
    "negu      $a3, %[dst]                       \n"
    
    "andi      $a3, $a3, 0x3                     \n"
    "beq       $a3, $zero, $chk16w               \n"
    
    "subu     %[count], %[count], $a3            \n"

    "lwr       $t8, 0(%[src])                    \n"
    "addu      %[src], %[src], $a3               \n"
    "swr       $t8, 0(%[dst])                    \n"
    "addu      %[dst], %[dst], $a3               \n"

    
    "$chk16w:                                    \n"
    "andi      $t8, %[count], 0x3f               \n"  
    
    "beq       %[count], $t8, chk8w              \n"
    
    "subu      $a3, %[count], $t8                \n"  
    
    "addu      $a3, %[dst], $a3                  \n"
    
    "addu      $t0, %[dst], %[count]             \n"
    

    
    
    
    
    
    "subu      $t9, $t0, 160                     \n"
#ifdef HAS_MIPS_PREFETCH
    
    "pref      0, 0(%[src])                      \n"  
    "pref      0, 32(%[src])                     \n"  
    "pref      0, 64(%[src])                     \n"
    "pref      30, 32(%[dst])                    \n"
#endif
    
    "sgtu      $v1, %[dst], $t9                  \n"
    "bgtz      $v1, $loop16w                     \n"
    "nop                                         \n"
    
#ifdef HAS_MIPS_PREFETCH
    "pref      30, 64(%[dst])                    \n"
#endif
    "$loop16w:                                    \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 96(%[src])                     \n"
#endif
    "lw        $t0, 0(%[src])                    \n"
    "bgtz      $v1, $skip_pref30_96              \n"  
    "lw        $t1, 4(%[src])                    \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      30, 96(%[dst])                    \n"  
#endif
    "$skip_pref30_96:                            \n"
    "lw        $t2, 8(%[src])                    \n"
    "lw        $t3, 12(%[src])                   \n"
    "lw        $t4, 16(%[src])                   \n"
    "lw        $t5, 20(%[src])                   \n"
    "lw        $t6, 24(%[src])                   \n"
    "lw        $t7, 28(%[src])                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 128(%[src])                    \n"
#endif
    
    "sw        $t0, 0(%[dst])                    \n"
    "sw        $t1, 4(%[dst])                    \n"
    "sw        $t2, 8(%[dst])                    \n"
    "sw        $t3, 12(%[dst])                   \n"
    "sw        $t4, 16(%[dst])                   \n"
    "sw        $t5, 20(%[dst])                   \n"
    "sw        $t6, 24(%[dst])                   \n"
    "sw        $t7, 28(%[dst])                   \n"
    "lw        $t0, 32(%[src])                   \n"
    "bgtz      $v1, $skip_pref30_128             \n"  
    "lw        $t1, 36(%[src])                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      30, 128(%[dst])                   \n"  
#endif
    "$skip_pref30_128:                           \n"
    "lw        $t2, 40(%[src])                   \n"
    "lw        $t3, 44(%[src])                   \n"
    "lw        $t4, 48(%[src])                   \n"
    "lw        $t5, 52(%[src])                   \n"
    "lw        $t6, 56(%[src])                   \n"
    "lw        $t7, 60(%[src])                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 160(%[src])                    \n"
#endif
    
    "sw        $t0, 32(%[dst])                   \n"
    "sw        $t1, 36(%[dst])                   \n"
    "sw        $t2, 40(%[dst])                   \n"
    "sw        $t3, 44(%[dst])                   \n"
    "sw        $t4, 48(%[dst])                   \n"
    "sw        $t5, 52(%[dst])                   \n"
    "sw        $t6, 56(%[dst])                   \n"
    "sw        $t7, 60(%[dst])                   \n"

    "addiu     %[dst], %[dst], 64                \n"  
    "sgtu      $v1, %[dst], $t9                  \n"
    "bne       %[dst], $a3, $loop16w             \n"
    " addiu    %[src], %[src], 64                \n"  
    "move      %[count], $t8                     \n"

    

    "chk8w:                                      \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 0x0(%[src])                    \n"
#endif
    "andi      $t8, %[count], 0x1f               \n"  
    
    "beq       %[count], $t8, chk1w              \n"
    
    " nop                                        \n"

    "lw        $t0, 0(%[src])                    \n"
    "lw        $t1, 4(%[src])                    \n"
    "lw        $t2, 8(%[src])                    \n"
    "lw        $t3, 12(%[src])                   \n"
    "lw        $t4, 16(%[src])                   \n"
    "lw        $t5, 20(%[src])                   \n"
    "lw        $t6, 24(%[src])                   \n"
    "lw        $t7, 28(%[src])                   \n"
    "addiu     %[src], %[src], 32                \n"

    "sw        $t0, 0(%[dst])                    \n"
    "sw        $t1, 4(%[dst])                    \n"
    "sw        $t2, 8(%[dst])                    \n"
    "sw        $t3, 12(%[dst])                   \n"
    "sw        $t4, 16(%[dst])                   \n"
    "sw        $t5, 20(%[dst])                   \n"
    "sw        $t6, 24(%[dst])                   \n"
    "sw        $t7, 28(%[dst])                   \n"
    "addiu     %[dst], %[dst], 32                \n"

    "chk1w:                                      \n"
    "andi      %[count], $t8, 0x3                \n"
    
    "beq       %[count], $t8, $last8             \n"
    " subu     $a3, $t8, %[count]                \n"
    
    "addu      $a3, %[dst], $a3                  \n"
    
    
    "$wordCopy_loop:                             \n"
    "lw        $t3, 0(%[src])                    \n"
    
    "addiu     %[src], %[src],4                  \n"
    "addiu     %[dst], %[dst],4                  \n"
    "bne       %[dst], $a3,$wordCopy_loop        \n"
    " sw       $t3, -4(%[dst])                   \n"

    
    "$last8:                                     \n"
    "blez      %[count], leave                   \n"
    " addu     $a3, %[dst], %[count]             \n"  
    "$last8loop:                                 \n"
    "lb        $v1, 0(%[src])                    \n"
    "addiu     %[src], %[src], 1                 \n"
    "addiu     %[dst], %[dst], 1                 \n"
    "bne       %[dst], $a3, $last8loop           \n"
    " sb       $v1, -1(%[dst])                   \n"

    "leave:                                      \n"
    "  j       $ra                               \n"
    "  nop                                       \n"

    
    
    

    "unaligned:                                  \n"
    
    "andi      $a3, $a3, 0x3                     \n"  
    "beqz      $a3, $ua_chk16w                   \n"
    " subu     %[count], %[count], $a3           \n"
    
    "lwr       $v1, 0(%[src])                    \n"
    "lwl       $v1, 3(%[src])                    \n"
    "addu      %[src], %[src], $a3               \n"  
    "swr       $v1, 0(%[dst])                    \n"
    "addu      %[dst], %[dst], $a3               \n"
    
    "$ua_chk16w:                                 \n"
    "andi      $t8, %[count], 0x3f               \n"  
    
    "beq       %[count], $t8, ua_chk8w           \n"
    
    
    "subu      $a3, %[count], $t8                \n"  
    
    "addu      $a3, %[dst], $a3                  \n"
    
    "addu      $t0, %[dst], %[count]             \n"  
    "subu      $t9, $t0, 160                     \n"
    
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 0(%[src])                      \n"  
    "pref      0, 32(%[src])                     \n"  
    "pref      0, 64(%[src])                     \n"
    "pref      30, 32(%[dst])                    \n"
#endif
    
    
    "sgtu      $v1, %[dst], $t9                  \n"
    "bgtz      $v1, $ua_loop16w                  \n"
    
    " nop                                        \n"
    
#ifdef HAS_MIPS_PREFETCH
    "pref      30, 64(%[dst])                    \n"
#endif
    "$ua_loop16w:                                \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 96(%[src])                     \n"
#endif
    "lwr       $t0, 0(%[src])                    \n"
    "lwl       $t0, 3(%[src])                    \n"
    "lwr       $t1, 4(%[src])                    \n"
    "bgtz      $v1, $ua_skip_pref30_96           \n"
    " lwl      $t1, 7(%[src])                    \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      30, 96(%[dst])                    \n"
#endif
    
    "$ua_skip_pref30_96:                         \n"
    "lwr       $t2, 8(%[src])                    \n"
    "lwl       $t2, 11(%[src])                   \n"
    "lwr       $t3, 12(%[src])                   \n"
    "lwl       $t3, 15(%[src])                   \n"
    "lwr       $t4, 16(%[src])                   \n"
    "lwl       $t4, 19(%[src])                   \n"
    "lwr       $t5, 20(%[src])                   \n"
    "lwl       $t5, 23(%[src])                   \n"
    "lwr       $t6, 24(%[src])                   \n"
    "lwl       $t6, 27(%[src])                   \n"
    "lwr       $t7, 28(%[src])                   \n"
    "lwl       $t7, 31(%[src])                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 128(%[src])                    \n"
#endif
    
    "sw        $t0, 0(%[dst])                    \n"
    "sw        $t1, 4(%[dst])                    \n"
    "sw        $t2, 8(%[dst])                    \n"
    "sw        $t3, 12(%[dst])                   \n"
    "sw        $t4, 16(%[dst])                   \n"
    "sw        $t5, 20(%[dst])                   \n"
    "sw        $t6, 24(%[dst])                   \n"
    "sw        $t7, 28(%[dst])                   \n"
    "lwr       $t0, 32(%[src])                   \n"
    "lwl       $t0, 35(%[src])                   \n"
    "lwr       $t1, 36(%[src])                   \n"
    "bgtz      $v1, ua_skip_pref30_128           \n"
    " lwl      $t1, 39(%[src])                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      30, 128(%[dst])                   \n"
#endif
    
    "ua_skip_pref30_128:                         \n"

    "lwr       $t2, 40(%[src])                   \n"
    "lwl       $t2, 43(%[src])                   \n"
    "lwr       $t3, 44(%[src])                   \n"
    "lwl       $t3, 47(%[src])                   \n"
    "lwr       $t4, 48(%[src])                   \n"
    "lwl       $t4, 51(%[src])                   \n"
    "lwr       $t5, 52(%[src])                   \n"
    "lwl       $t5, 55(%[src])                   \n"
    "lwr       $t6, 56(%[src])                   \n"
    "lwl       $t6, 59(%[src])                   \n"
    "lwr       $t7, 60(%[src])                   \n"
    "lwl       $t7, 63(%[src])                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 160(%[src])                    \n"
#endif
    
    "sw        $t0, 32(%[dst])                   \n"
    "sw        $t1, 36(%[dst])                   \n"
    "sw        $t2, 40(%[dst])                   \n"
    "sw        $t3, 44(%[dst])                   \n"
    "sw        $t4, 48(%[dst])                   \n"
    "sw        $t5, 52(%[dst])                   \n"
    "sw        $t6, 56(%[dst])                   \n"
    "sw        $t7, 60(%[dst])                   \n"

    "addiu     %[dst],%[dst],64                  \n"  
    "sgtu      $v1,%[dst],$t9                    \n"
    "bne       %[dst],$a3,$ua_loop16w            \n"
    " addiu    %[src],%[src],64                  \n"  
    "move      %[count],$t8                      \n"

    

    "ua_chk8w:                                   \n"
#ifdef HAS_MIPS_PREFETCH
    "pref      0, 0x0(%[src])                    \n"
#endif
    "andi      $t8, %[count], 0x1f               \n"  
    
    "beq       %[count], $t8, $ua_chk1w          \n"
    

    "lwr       $t0, 0(%[src])                    \n"
    "lwl       $t0, 3(%[src])                    \n"
    "lwr       $t1, 4(%[src])                    \n"
    "lwl       $t1, 7(%[src])                    \n"
    "lwr       $t2, 8(%[src])                    \n"
    "lwl       $t2, 11(%[src])                   \n"
    "lwr       $t3, 12(%[src])                   \n"
    "lwl       $t3, 15(%[src])                   \n"
    "lwr       $t4, 16(%[src])                   \n"
    "lwl       $t4, 19(%[src])                   \n"
    "lwr       $t5, 20(%[src])                   \n"
    "lwl       $t5, 23(%[src])                   \n"
    "lwr       $t6, 24(%[src])                   \n"
    "lwl       $t6, 27(%[src])                   \n"
    "lwr       $t7, 28(%[src])                   \n"
    "lwl       $t7, 31(%[src])                   \n"
    "addiu     %[src], %[src], 32                \n"

    "sw        $t0, 0(%[dst])                    \n"
    "sw        $t1, 4(%[dst])                    \n"
    "sw        $t2, 8(%[dst])                    \n"
    "sw        $t3, 12(%[dst])                   \n"
    "sw        $t4, 16(%[dst])                   \n"
    "sw        $t5, 20(%[dst])                   \n"
    "sw        $t6, 24(%[dst])                   \n"
    "sw        $t7, 28(%[dst])                   \n"
    "addiu     %[dst], %[dst], 32                \n"

    "$ua_chk1w:                                  \n"
    "andi      %[count], $t8, 0x3                \n"
    
    "beq       %[count], $t8, ua_smallCopy       \n"
    "subu      $a3, $t8, %[count]                \n"
    
    "addu      $a3, %[dst], $a3                  \n"
    

    
    "$ua_wordCopy_loop:                          \n"
    "lwr       $v1, 0(%[src])                    \n"
    "lwl       $v1, 3(%[src])                    \n"
    "addiu     %[src], %[src], 4                 \n"
    "addiu     %[dst], %[dst], 4                 \n"
    
    "bne       %[dst], $a3, $ua_wordCopy_loop    \n"
    " sw       $v1,-4(%[dst])                    \n"

    
    "ua_smallCopy:                               \n"
    "beqz      %[count], leave                   \n"
    " addu     $a3, %[dst], %[count]             \n" 
    "$ua_smallCopy_loop:                         \n"
    "lb        $v1, 0(%[src])                    \n"
    "addiu     %[src], %[src], 1                 \n"
    "addiu     %[dst], %[dst], 1                 \n"
    "bne       %[dst],$a3,$ua_smallCopy_loop     \n"
    " sb       $v1, -1(%[dst])                   \n"

    "j         $ra                               \n"
    " nop                                        \n"
    ".set      at                                \n"
    ".set      reorder                           \n"
       : [dst] "+r" (dst), [src] "+r" (src)
       : [count] "r" (count)
       : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
       "t8", "t9", "a3", "v1", "at"
  );
}
#endif  


#if !defined(LIBYUV_DISABLE_MIPS) && defined(__mips_dsp) && \
    (__mips_dsp_rev >= 2)
void SplitUVRow_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                           int width) {
  __asm__ __volatile__ (
    ".set push                                     \n"
    ".set noreorder                                \n"
    "srl             $t4, %[width], 4              \n"  
    "blez            $t4, 2f                       \n"
    " andi           %[width], %[width], 0xf       \n"  

    ".p2align        2                             \n"
  "1:                                              \n"
    "addiu           $t4, $t4, -1                  \n"
    "lw              $t0, 0(%[src_uv])             \n"  
    "lw              $t1, 4(%[src_uv])             \n"  
    "lw              $t2, 8(%[src_uv])             \n"  
    "lw              $t3, 12(%[src_uv])            \n"  
    "lw              $t5, 16(%[src_uv])            \n"  
    "lw              $t6, 20(%[src_uv])            \n"  
    "lw              $t7, 24(%[src_uv])            \n"  
    "lw              $t8, 28(%[src_uv])            \n"  
    "addiu           %[src_uv], %[src_uv], 32      \n"
    "precrq.qb.ph    $t9, $t1, $t0                 \n"  
    "precr.qb.ph     $t0, $t1, $t0                 \n"  
    "precrq.qb.ph    $t1, $t3, $t2                 \n"  
    "precr.qb.ph     $t2, $t3, $t2                 \n"  
    "precrq.qb.ph    $t3, $t6, $t5                 \n"  
    "precr.qb.ph     $t5, $t6, $t5                 \n"  
    "precrq.qb.ph    $t6, $t8, $t7                 \n"  
    "precr.qb.ph     $t7, $t8, $t7                 \n"  
    "sw              $t9, 0(%[dst_v])              \n"
    "sw              $t0, 0(%[dst_u])              \n"
    "sw              $t1, 4(%[dst_v])              \n"
    "sw              $t2, 4(%[dst_u])              \n"
    "sw              $t3, 8(%[dst_v])              \n"
    "sw              $t5, 8(%[dst_u])              \n"
    "sw              $t6, 12(%[dst_v])             \n"
    "sw              $t7, 12(%[dst_u])             \n"
    "addiu           %[dst_v], %[dst_v], 16        \n"
    "bgtz            $t4, 1b                       \n"
    " addiu          %[dst_u], %[dst_u], 16        \n"

    "beqz            %[width], 3f                  \n"
    " nop                                          \n"

  "2:                                              \n"
    "lbu             $t0, 0(%[src_uv])             \n"
    "lbu             $t1, 1(%[src_uv])             \n"
    "addiu           %[src_uv], %[src_uv], 2       \n"
    "addiu           %[width], %[width], -1        \n"
    "sb              $t0, 0(%[dst_u])              \n"
    "sb              $t1, 0(%[dst_v])              \n"
    "addiu           %[dst_u], %[dst_u], 1         \n"
    "bgtz            %[width], 2b                  \n"
    " addiu          %[dst_v], %[dst_v], 1         \n"

  "3:                                              \n"
    ".set pop                                      \n"
     : [src_uv] "+r" (src_uv),
       [width] "+r" (width),
       [dst_u] "+r" (dst_u),
       [dst_v] "+r" (dst_v)
     :
     : "t0", "t1", "t2", "t3",
     "t4", "t5", "t6", "t7", "t8", "t9"
  );
}

void SplitUVRow_Unaligned_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u,
                                     uint8* dst_v, int width) {
  __asm__ __volatile__ (
    ".set push                                     \n"
    ".set noreorder                                \n"
    "srl             $t4, %[width], 4              \n"  
    "blez            $t4, 2f                       \n"
    " andi           %[width], %[width], 0xf       \n"  

    ".p2align        2                             \n"
  "1:                                              \n"
    "addiu           $t4, $t4, -1                  \n"
    "lwr             $t0, 0(%[src_uv])             \n"
    "lwl             $t0, 3(%[src_uv])             \n"  
    "lwr             $t1, 4(%[src_uv])             \n"
    "lwl             $t1, 7(%[src_uv])             \n"  
    "lwr             $t2, 8(%[src_uv])             \n"
    "lwl             $t2, 11(%[src_uv])            \n"  
    "lwr             $t3, 12(%[src_uv])            \n"
    "lwl             $t3, 15(%[src_uv])            \n"  
    "lwr             $t5, 16(%[src_uv])            \n"
    "lwl             $t5, 19(%[src_uv])            \n"  
    "lwr             $t6, 20(%[src_uv])            \n"
    "lwl             $t6, 23(%[src_uv])            \n"  
    "lwr             $t7, 24(%[src_uv])            \n"
    "lwl             $t7, 27(%[src_uv])            \n"  
    "lwr             $t8, 28(%[src_uv])            \n"
    "lwl             $t8, 31(%[src_uv])            \n"  
    "precrq.qb.ph    $t9, $t1, $t0                 \n"  
    "precr.qb.ph     $t0, $t1, $t0                 \n"  
    "precrq.qb.ph    $t1, $t3, $t2                 \n"  
    "precr.qb.ph     $t2, $t3, $t2                 \n"  
    "precrq.qb.ph    $t3, $t6, $t5                 \n"  
    "precr.qb.ph     $t5, $t6, $t5                 \n"  
    "precrq.qb.ph    $t6, $t8, $t7                 \n"  
    "precr.qb.ph     $t7, $t8, $t7                 \n"  
    "addiu           %[src_uv], %[src_uv], 32      \n"
    "swr             $t9, 0(%[dst_v])              \n"
    "swl             $t9, 3(%[dst_v])              \n"
    "swr             $t0, 0(%[dst_u])              \n"
    "swl             $t0, 3(%[dst_u])              \n"
    "swr             $t1, 4(%[dst_v])              \n"
    "swl             $t1, 7(%[dst_v])              \n"
    "swr             $t2, 4(%[dst_u])              \n"
    "swl             $t2, 7(%[dst_u])              \n"
    "swr             $t3, 8(%[dst_v])              \n"
    "swl             $t3, 11(%[dst_v])             \n"
    "swr             $t5, 8(%[dst_u])              \n"
    "swl             $t5, 11(%[dst_u])             \n"
    "swr             $t6, 12(%[dst_v])             \n"
    "swl             $t6, 15(%[dst_v])             \n"
    "swr             $t7, 12(%[dst_u])             \n"
    "swl             $t7, 15(%[dst_u])             \n"
    "addiu           %[dst_u], %[dst_u], 16        \n"
    "bgtz            $t4, 1b                       \n"
    " addiu          %[dst_v], %[dst_v], 16        \n"

    "beqz            %[width], 3f                  \n"
    " nop                                          \n"

  "2:                                              \n"
    "lbu             $t0, 0(%[src_uv])             \n"
    "lbu             $t1, 1(%[src_uv])             \n"
    "addiu           %[src_uv], %[src_uv], 2       \n"
    "addiu           %[width], %[width], -1        \n"
    "sb              $t0, 0(%[dst_u])              \n"
    "sb              $t1, 0(%[dst_v])              \n"
    "addiu           %[dst_u], %[dst_u], 1         \n"
    "bgtz            %[width], 2b                  \n"
    " addiu          %[dst_v], %[dst_v], 1         \n"

  "3:                                              \n"
    ".set pop                                      \n"
     : [src_uv] "+r" (src_uv),
       [width] "+r" (width),
       [dst_u] "+r" (dst_u),
       [dst_v] "+r" (dst_v)
     :
     : "t0", "t1", "t2", "t3",
     "t4", "t5", "t6", "t7", "t8", "t9"
  );
}

void MirrorRow_MIPS_DSPR2(const uint8* src, uint8* dst, int width) {
  __asm__ __volatile__ (
    ".set push                             \n"
    ".set noreorder                        \n"

    "srl       $t4, %[width], 4            \n"  
    "andi      $t5, %[width], 0xf          \n"
    "blez      $t4, 2f                     \n"
    " addu     %[src], %[src], %[width]    \n"  

    ".p2align  2                           \n"
   "1:                                     \n"
    "lw        $t0, -16(%[src])            \n"  
    "lw        $t1, -12(%[src])            \n"  
    "lw        $t2, -8(%[src])             \n"  
    "lw        $t3, -4(%[src])             \n"  
    "wsbh      $t0, $t0                    \n"  
    "wsbh      $t1, $t1                    \n"  
    "wsbh      $t2, $t2                    \n"  
    "wsbh      $t3, $t3                    \n"  
    "rotr      $t0, $t0, 16                \n"  
    "rotr      $t1, $t1, 16                \n"  
    "rotr      $t2, $t2, 16                \n"  
    "rotr      $t3, $t3, 16                \n"  
    "addiu     %[src], %[src], -16         \n"
    "addiu     $t4, $t4, -1                \n"
    "sw        $t3, 0(%[dst])              \n"  
    "sw        $t2, 4(%[dst])              \n"  
    "sw        $t1, 8(%[dst])              \n"  
    "sw        $t0, 12(%[dst])             \n"  
    "bgtz      $t4, 1b                     \n"
    " addiu    %[dst], %[dst], 16          \n"
    "beqz      $t5, 3f                     \n"
    " nop                                  \n"

   "2:                                     \n"
    "lbu       $t0, -1(%[src])             \n"
    "addiu     $t5, $t5, -1                \n"
    "addiu     %[src], %[src], -1          \n"
    "sb        $t0, 0(%[dst])              \n"
    "bgez      $t5, 2b                     \n"
    " addiu    %[dst], %[dst], 1           \n"

   "3:                                     \n"
    ".set pop                              \n"
      : [src] "+r" (src), [dst] "+r" (dst)
      : [width] "r" (width)
      : "t0", "t1", "t2", "t3", "t4", "t5"
  );
}

void MirrorUVRow_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                            int width) {
  int x = 0;
  int y = 0;
  __asm__ __volatile__ (
    ".set push                                    \n"
    ".set noreorder                               \n"

    "addu            $t4, %[width], %[width]      \n"
    "srl             %[x], %[width], 4            \n"
    "andi            %[y], %[width], 0xf          \n"
    "blez            %[x], 2f                     \n"
    " addu           %[src_uv], %[src_uv], $t4    \n"

    ".p2align        2                            \n"
   "1:                                            \n"
    "lw              $t0, -32(%[src_uv])          \n"  
    "lw              $t1, -28(%[src_uv])          \n"  
    "lw              $t2, -24(%[src_uv])          \n"  
    "lw              $t3, -20(%[src_uv])          \n"  
    "lw              $t4, -16(%[src_uv])          \n"  
    "lw              $t6, -12(%[src_uv])          \n"  
    "lw              $t7, -8(%[src_uv])           \n"  
    "lw              $t8, -4(%[src_uv])           \n"  

    "rotr            $t0, $t0, 16                 \n"  
    "rotr            $t1, $t1, 16                 \n"  
    "rotr            $t2, $t2, 16                 \n"  
    "rotr            $t3, $t3, 16                 \n"  
    "rotr            $t4, $t4, 16                 \n"  
    "rotr            $t6, $t6, 16                 \n"  
    "rotr            $t7, $t7, 16                 \n"  
    "rotr            $t8, $t8, 16                 \n"  
    "precr.qb.ph     $t9, $t0, $t1                \n"  
    "precrq.qb.ph    $t5, $t0, $t1                \n"  
    "precr.qb.ph     $t0, $t2, $t3                \n"  
    "precrq.qb.ph    $t1, $t2, $t3                \n"  
    "precr.qb.ph     $t2, $t4, $t6                \n"  
    "precrq.qb.ph    $t3, $t4, $t6                \n"  
    "precr.qb.ph     $t4, $t7, $t8                \n"  
    "precrq.qb.ph    $t6, $t7, $t8                \n"  
    "addiu           %[src_uv], %[src_uv], -32    \n"
    "addiu           %[x], %[x], -1               \n"
    "swr             $t4, 0(%[dst_u])             \n"
    "swl             $t4, 3(%[dst_u])             \n"  
    "swr             $t6, 0(%[dst_v])             \n"
    "swl             $t6, 3(%[dst_v])             \n"  
    "swr             $t2, 4(%[dst_u])             \n"
    "swl             $t2, 7(%[dst_u])             \n"  
    "swr             $t3, 4(%[dst_v])             \n"
    "swl             $t3, 7(%[dst_v])             \n"  
    "swr             $t0, 8(%[dst_u])             \n"
    "swl             $t0, 11(%[dst_u])            \n"  
    "swr             $t1, 8(%[dst_v])             \n"
    "swl             $t1, 11(%[dst_v])            \n"  
    "swr             $t9, 12(%[dst_u])            \n"
    "swl             $t9, 15(%[dst_u])            \n"  
    "swr             $t5, 12(%[dst_v])            \n"
    "swl             $t5, 15(%[dst_v])            \n"  
    "addiu           %[dst_v], %[dst_v], 16       \n"
    "bgtz            %[x], 1b                     \n"
    " addiu          %[dst_u], %[dst_u], 16       \n"
    "beqz            %[y], 3f                     \n"
    " nop                                         \n"
    "b               2f                           \n"
    " nop                                         \n"

   "2:                                            \n"
    "lbu             $t0, -2(%[src_uv])           \n"
    "lbu             $t1, -1(%[src_uv])           \n"
    "addiu           %[src_uv], %[src_uv], -2     \n"
    "addiu           %[y], %[y], -1               \n"
    "sb              $t0, 0(%[dst_u])             \n"
    "sb              $t1, 0(%[dst_v])             \n"
    "addiu           %[dst_u], %[dst_u], 1        \n"
    "bgtz            %[y], 2b                     \n"
    " addiu          %[dst_v], %[dst_v], 1        \n"

   "3:                                            \n"
    ".set pop                                     \n"
      : [src_uv] "+r" (src_uv),
        [dst_u] "+r" (dst_u),
        [dst_v] "+r" (dst_v),
        [x] "=&r" (x),
        [y] "+r" (y)
      : [width] "r" (width)
      : "t0", "t1", "t2", "t3", "t4",
      "t5", "t7", "t8", "t9"
  );
}








#define I422ToTransientMipsRGB                                                 \
      "lw                $t0, 0(%[y_buf])       \n"                            \
      "lhu               $t1, 0(%[u_buf])       \n"                            \
      "lhu               $t2, 0(%[v_buf])       \n"                            \
      "preceu.ph.qbr     $t1, $t1               \n"                            \
      "preceu.ph.qbr     $t2, $t2               \n"                            \
      "preceu.ph.qbra    $t3, $t0               \n"                            \
      "preceu.ph.qbla    $t0, $t0               \n"                            \
      "subu.ph           $t1, $t1, $s5          \n"                            \
      "subu.ph           $t2, $t2, $s5          \n"                            \
      "subu.ph           $t3, $t3, $s4          \n"                            \
      "subu.ph           $t0, $t0, $s4          \n"                            \
      "mul.ph            $t3, $t3, $s0          \n"                            \
      "mul.ph            $t0, $t0, $s0          \n"                            \
      "shll.ph           $t4, $t1, 0x7          \n"                            \
      "subu.ph           $t4, $t4, $t1          \n"                            \
      "mul.ph            $t6, $t1, $s1          \n"                            \
      "mul.ph            $t1, $t2, $s2          \n"                            \
      "addq_s.ph         $t5, $t4, $t3          \n"                            \
      "addq_s.ph         $t4, $t4, $t0          \n"                            \
      "shra.ph           $t5, $t5, 6            \n"                            \
      "shra.ph           $t4, $t4, 6            \n"                            \
      "addiu             %[u_buf], 2            \n"                            \
      "addiu             %[v_buf], 2            \n"                            \
      "addu.ph           $t6, $t6, $t1          \n"                            \
      "mul.ph            $t1, $t2, $s3          \n"                            \
      "addu.ph           $t9, $t6, $t3          \n"                            \
      "addu.ph           $t8, $t6, $t0          \n"                            \
      "shra.ph           $t9, $t9, 6            \n"                            \
      "shra.ph           $t8, $t8, 6            \n"                            \
      "addu.ph           $t2, $t1, $t3          \n"                            \
      "addu.ph           $t1, $t1, $t0          \n"                            \
      "shra.ph           $t2, $t2, 6            \n"                            \
      "shra.ph           $t1, $t1, 6            \n"                            \
      "subu.ph           $t5, $t5, $s5          \n"                            \
      "subu.ph           $t4, $t4, $s5          \n"                            \
      "subu.ph           $t9, $t9, $s5          \n"                            \
      "subu.ph           $t8, $t8, $s5          \n"                            \
      "subu.ph           $t2, $t2, $s5          \n"                            \
      "subu.ph           $t1, $t1, $s5          \n"                            \
      "shll_s.ph         $t5, $t5, 8            \n"                            \
      "shll_s.ph         $t4, $t4, 8            \n"                            \
      "shll_s.ph         $t9, $t9, 8            \n"                            \
      "shll_s.ph         $t8, $t8, 8            \n"                            \
      "shll_s.ph         $t2, $t2, 8            \n"                            \
      "shll_s.ph         $t1, $t1, 8            \n"                            \
      "shra.ph           $t5, $t5, 8            \n"                            \
      "shra.ph           $t4, $t4, 8            \n"                            \
      "shra.ph           $t9, $t9, 8            \n"                            \
      "shra.ph           $t8, $t8, 8            \n"                            \
      "shra.ph           $t2, $t2, 8            \n"                            \
      "shra.ph           $t1, $t1, 8            \n"                            \
      "addu.ph           $t5, $t5, $s5          \n"                            \
      "addu.ph           $t4, $t4, $s5          \n"                            \
      "addu.ph           $t9, $t9, $s5          \n"                            \
      "addu.ph           $t8, $t8, $s5          \n"                            \
      "addu.ph           $t2, $t2, $s5          \n"                            \
      "addu.ph           $t1, $t1, $s5          \n"

void I422ToARGBRow_MIPS_DSPR2(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  __asm__ __volatile__ (
    ".set push                                \n"
    ".set noreorder                           \n"
    "beqz              %[width], 2f           \n"
    " repl.ph          $s0, 74                \n"  
    "repl.ph           $s1, -25               \n"  
    "repl.ph           $s2, -52               \n"  
    "repl.ph           $s3, 102               \n"  
    "repl.ph           $s4, 16                \n"  
    "repl.ph           $s5, 128               \n"  
    "lui               $s6, 0xff00            \n"
    "ori               $s6, 0xff00            \n"  

    ".p2align          2                      \n"
   "1:                                        \n"
      I422ToTransientMipsRGB

    "precr.qb.ph       $t4, $t8, $t4          \n"  
    "precr.qb.ph       $t5, $t9, $t5          \n"  
    "addiu             %[width], -4           \n"
    "precrq.qb.ph      $t8, $t4, $t5          \n"  
    "precr.qb.ph       $t9, $t4, $t5          \n"  
    "precr.qb.ph       $t2, $t1, $t2          \n"  

    "addiu             %[y_buf], 4            \n"
    "preceu.ph.qbla    $t1, $t2               \n"  
    "preceu.ph.qbra    $t2, $t2               \n"  
    "or                $t1, $t1, $s6          \n"  
    "or                $t2, $t2, $s6          \n"  
    "precrq.ph.w       $t0, $t2, $t9          \n"  
    "precrq.ph.w       $t3, $t1, $t8          \n"  
    "sll               $t9, $t9, 16           \n"
    "sll               $t8, $t8, 16           \n"
    "packrl.ph         $t2, $t2, $t9          \n"  
    "packrl.ph         $t1, $t1, $t8          \n"  

    "sw                $t2, 0(%[rgb_buf])     \n"
    "sw                $t0, 4(%[rgb_buf])     \n"
    "sw                $t1, 8(%[rgb_buf])     \n"
    "sw                $t3, 12(%[rgb_buf])    \n"
    "bnez              %[width], 1b           \n"
    " addiu            %[rgb_buf], 16         \n"
   "2:                                        \n"
    ".set pop                                 \n"
      :[y_buf] "+r" (y_buf),
       [u_buf] "+r" (u_buf),
       [v_buf] "+r" (v_buf),
       [width] "+r" (width),
       [rgb_buf] "+r" (rgb_buf)
      :
      : "t0", "t1",  "t2", "t3",  "t4", "t5",
      "t6", "t7", "t8", "t9",
      "s0", "s1", "s2", "s3",
      "s4", "s5", "s6"
  );
}

void I422ToABGRRow_MIPS_DSPR2(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  __asm__ __volatile__ (
    ".set push                                \n"
    ".set noreorder                           \n"
    "beqz              %[width], 2f           \n"
    " repl.ph          $s0, 74                \n"  
    "repl.ph           $s1, -25               \n"  
    "repl.ph           $s2, -52               \n"  
    "repl.ph           $s3, 102               \n"  
    "repl.ph           $s4, 16                \n"  
    "repl.ph           $s5, 128               \n"  
    "lui               $s6, 0xff00            \n"
    "ori               $s6, 0xff00            \n"  

    ".p2align          2                       \n"
   "1:                                         \n"
      I422ToTransientMipsRGB

    "precr.qb.ph      $t0, $t8, $t1           \n"  
    "precr.qb.ph      $t3, $t9, $t2           \n"  
    "precrq.qb.ph     $t8, $t0, $t3           \n"  
    "precr.qb.ph      $t9, $t0, $t3           \n"  

    "precr.qb.ph       $t2, $t4, $t5          \n"  
    "addiu             %[width], -4           \n"
    "addiu             %[y_buf], 4            \n"
    "preceu.ph.qbla    $t1, $t2               \n"  
    "preceu.ph.qbra    $t2, $t2               \n"  
    "or                $t1, $t1, $s6          \n"  
    "or                $t2, $t2, $s6          \n"  
    "precrq.ph.w       $t0, $t2, $t9          \n"  
    "precrq.ph.w       $t3, $t1, $t8          \n"  
    "sll               $t9, $t9, 16           \n"
    "sll               $t8, $t8, 16           \n"
    "packrl.ph         $t2, $t2, $t9          \n"  
    "packrl.ph         $t1, $t1, $t8          \n"  

    "sw                $t2, 0(%[rgb_buf])     \n"
    "sw                $t0, 4(%[rgb_buf])     \n"
    "sw                $t1, 8(%[rgb_buf])     \n"
    "sw                $t3, 12(%[rgb_buf])    \n"
    "bnez              %[width], 1b           \n"
    " addiu            %[rgb_buf], 16         \n"
   "2:                                        \n"
    ".set pop                                 \n"
      :[y_buf] "+r" (y_buf),
       [u_buf] "+r" (u_buf),
       [v_buf] "+r" (v_buf),
       [width] "+r" (width),
       [rgb_buf] "+r" (rgb_buf)
      :
      : "t0", "t1",  "t2", "t3",  "t4", "t5",
      "t6", "t7", "t8", "t9",
      "s0", "s1", "s2", "s3",
      "s4", "s5", "s6"
  );
}

void I422ToBGRARow_MIPS_DSPR2(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  __asm__ __volatile__ (
    ".set push                                \n"
    ".set noreorder                           \n"
    "beqz              %[width], 2f           \n"
    " repl.ph          $s0, 74                \n"  
    "repl.ph           $s1, -25               \n"  
    "repl.ph           $s2, -52               \n"  
    "repl.ph           $s3, 102               \n"  
    "repl.ph           $s4, 16                \n"  
    "repl.ph           $s5, 128               \n"  
    "lui               $s6, 0xff              \n"
    "ori               $s6, 0xff              \n"  

    ".p2align          2                      \n"
   "1:                                        \n"
      I422ToTransientMipsRGB
      
    "precr.qb.ph       $t4, $t4, $t8          \n"  
    "precr.qb.ph       $t5, $t5, $t9          \n"  
    "precrq.qb.ph      $t8, $t4, $t5          \n"  
    "precr.qb.ph       $t9, $t4, $t5          \n"  

    "precr.qb.ph       $t2, $t1, $t2          \n"  
    "addiu             %[width], -4           \n"
    "addiu             %[y_buf], 4            \n"
    "preceu.ph.qbla    $t1, $t2               \n"  
    "preceu.ph.qbra    $t2, $t2               \n"  
    "sll               $t1, $t1, 8            \n"  
    "sll               $t2, $t2, 8            \n"  
    "or                $t1, $t1, $s6          \n"  
    "or                $t2, $t2, $s6          \n"  
    "precrq.ph.w       $t0, $t9, $t2          \n"  
    "precrq.ph.w       $t3, $t8, $t1          \n"  
    "sll               $t1, $t1, 16           \n"
    "sll               $t2, $t2, 16           \n"
    "packrl.ph         $t2, $t9, $t2          \n"  
    "packrl.ph         $t1, $t8, $t1          \n"  

    "sw                $t2, 0(%[rgb_buf])     \n"
    "sw                $t0, 4(%[rgb_buf])     \n"
    "sw                $t1, 8(%[rgb_buf])     \n"
    "sw                $t3, 12(%[rgb_buf])    \n"
    "bnez              %[width], 1b           \n"
    " addiu            %[rgb_buf], 16         \n"
   "2:                                        \n"
    ".set pop                                 \n"
      :[y_buf] "+r" (y_buf),
       [u_buf] "+r" (u_buf),
       [v_buf] "+r" (v_buf),
       [width] "+r" (width),
       [rgb_buf] "+r" (rgb_buf)
      :
      : "t0", "t1",  "t2", "t3",  "t4", "t5",
      "t6", "t7", "t8", "t9",
      "s0", "s1", "s2", "s3",
      "s4", "s5", "s6"
  );
}


void InterpolateRows_MIPS_DSPR2(uint8* dst_ptr, const uint8* src_ptr,
                                ptrdiff_t src_stride, int dst_width,
                                int source_y_fraction) {
    int y0_fraction = 256 - source_y_fraction;
    const uint8* src_ptr1 = src_ptr + src_stride;

  __asm__ __volatile__ (
     ".set push                                           \n"
     ".set noreorder                                      \n"

     "replv.ph          $t0, %[y0_fraction]               \n"
     "replv.ph          $t1, %[source_y_fraction]         \n"

    ".p2align           2                                 \n"
   "1:                                                    \n"
     "lw                $t2, 0(%[src_ptr])                \n"
     "lw                $t3, 0(%[src_ptr1])               \n"
     "lw                $t4, 4(%[src_ptr])                \n"
     "lw                $t5, 4(%[src_ptr1])               \n"
     "muleu_s.ph.qbl    $t6, $t2, $t0                     \n"
     "muleu_s.ph.qbr    $t7, $t2, $t0                     \n"
     "muleu_s.ph.qbl    $t8, $t3, $t1                     \n"
     "muleu_s.ph.qbr    $t9, $t3, $t1                     \n"
     "muleu_s.ph.qbl    $t2, $t4, $t0                     \n"
     "muleu_s.ph.qbr    $t3, $t4, $t0                     \n"
     "muleu_s.ph.qbl    $t4, $t5, $t1                     \n"
     "muleu_s.ph.qbr    $t5, $t5, $t1                     \n"
     "addq.ph           $t6, $t6, $t8                     \n"
     "addq.ph           $t7, $t7, $t9                     \n"
     "addq.ph           $t2, $t2, $t4                     \n"
     "addq.ph           $t3, $t3, $t5                     \n"
     "shra.ph           $t6, $t6, 8                       \n"
     "shra.ph           $t7, $t7, 8                       \n"
     "shra.ph           $t2, $t2, 8                       \n"
     "shra.ph           $t3, $t3, 8                       \n"
     "precr.qb.ph       $t6, $t6, $t7                     \n"
     "precr.qb.ph       $t2, $t2, $t3                     \n"
     "addiu             %[src_ptr], %[src_ptr], 8         \n"
     "addiu             %[src_ptr1], %[src_ptr1], 8       \n"
     "addiu             %[dst_width], %[dst_width], -8    \n"
     "sw                $t6, 0(%[dst_ptr])                \n"
     "sw                $t2, 4(%[dst_ptr])                \n"
     "bgtz              %[dst_width], 1b                  \n"
     " addiu            %[dst_ptr], %[dst_ptr], 8         \n"

     ".set pop                                            \n"
  : [dst_ptr] "+r" (dst_ptr),
    [src_ptr1] "+r" (src_ptr1),
    [src_ptr] "+r" (src_ptr),
    [dst_width] "+r" (dst_width)
  : [source_y_fraction] "r" (source_y_fraction),
    [y0_fraction] "r" (y0_fraction),
    [src_stride] "r" (src_stride)
  : "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "t8", "t9"
  );
}
#endif  

#endif  

#ifdef __cplusplus
}  
}  
#endif
