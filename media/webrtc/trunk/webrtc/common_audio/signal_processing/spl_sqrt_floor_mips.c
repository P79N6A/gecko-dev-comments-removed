






























#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
















int32_t WebRtcSpl_SqrtFloor(int32_t value)
{
  int32_t root = 0, tmp1, tmp2, tmp3, tmp4;

  __asm __volatile(
    ".set   push                                       \n\t"
    ".set   noreorder                                  \n\t"

    "lui    %[tmp1],      0x4000                       \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "sub    %[tmp3],      %[value],     %[tmp1]        \n\t"
    "lui    %[tmp1],      0x1                          \n\t"
    "or     %[tmp4],      %[root],      %[tmp1]        \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x4000         \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      14                           \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x8000         \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x2000         \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      13                           \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x4000         \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x1000         \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      12                           \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x2000         \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x800          \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      11                           \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x1000         \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x400          \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      10                           \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x800          \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x200          \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      9                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],       0x400         \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x100          \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      8                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x200          \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x80           \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      7                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x100          \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x40           \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      6                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x80           \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x20           \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      5                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x40           \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x10           \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      4                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x20           \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x8            \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      3                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x10           \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x4            \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      2                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x8            \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x2            \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "sll    %[tmp1],      1                            \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "subu   %[tmp3],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x4            \n\t"
    "movz   %[value],     %[tmp3],      %[tmp2]        \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    "addiu  %[tmp1],      $0,           0x1            \n\t"
    "addu   %[tmp1],      %[tmp1],      %[root]        \n\t"
    "slt    %[tmp2],      %[value],     %[tmp1]        \n\t"
    "ori    %[tmp4],      %[root],      0x2            \n\t"
    "movz   %[root],      %[tmp4],      %[tmp2]        \n\t"

    ".set   pop                                        \n\t"

    : [root] "+r" (root), [value] "+r" (value),
      [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
      [tmp3] "=&r" (tmp3), [tmp4] "=&r" (tmp4)
    :
  );

  return root >> 1;
}

