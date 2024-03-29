#if 0







































ps_4_0
dcl_constantbuffer cb0[1], immediateIndexed
dcl_resource_texture2darray (uint,uint,uint,uint) t0
dcl_input_ps_siv constant v1.x, rendertarget_array_index
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 1
dcl_indexableTemp x0[6], 4
resinfo_uint r0.xyzw, l(0), t0.xyzw
utof r0.xy, r0.xyxx
mul r0.xy, r0.xyxx, v2.xyxx
ftoi r0.xy, r0.xyxx
mov r0.z, v1.x
mov r0.w, l(0)
ld r0.xyzw, r0.xyzw, t0.xyzw
mov x0[0].x, r0.x
mov x0[1].x, r0.y
mov x0[2].x, r0.z
mov x0[3].x, r0.w
mov x0[4].x, l(0)
mov x0[5].x, l(1)
mov r0.x, cb0[0].x
mov o0.x, x0[r0.x + 0].x
mov r0.x, cb0[0].y
mov o0.y, x0[r0.x + 0].x
mov r0.x, cb0[0].z
mov o0.z, x0[r0.x + 0].x
mov r0.x, cb0[0].w
mov o0.w, x0[r0.x + 0].x
ret 

#endif

const BYTE g_PS_SwizzleUI2DArray[] =
{
     68,  88,  66,  67, 239, 117, 
    221,  64, 238, 147, 130,  15, 
     39,  48, 171, 143,  66,  71, 
    242,  70,   1,   0,   0,   0, 
    240,   4,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     64,   1,   0,   0, 200,   1, 
      0,   0, 252,   1,   0,   0, 
    116,   4,   0,   0,  82,  68, 
     69,  70,   4,   1,   0,   0, 
      1,   0,   0,   0, 128,   0, 
      0,   0,   2,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    255, 255,   0,   1,   0,   0, 
    208,   0,   0,   0,  92,   0, 
      0,   0,   2,   0,   0,   0, 
      4,   0,   0,   0,   5,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    109,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  84, 101, 120, 116, 
    117, 114, 101,  85,  73,  50, 
     68,  65, 114, 114,  97, 121, 
      0,  83, 119, 105, 122, 122, 
    108, 101,  80, 114, 111, 112, 
    101, 114, 116, 105, 101, 115, 
      0, 171, 109,   0,   0,   0, 
      1,   0,   0,   0, 152,   0, 
      0,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 176,   0,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
    192,   0,   0,   0,   0,   0, 
      0,   0,  83, 119, 105, 122, 
    122, 108, 101,  73, 110, 100, 
    105,  99, 101, 115,   0, 171, 
      1,   0,  19,   0,   1,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  57,  46,  51, 
     48,  46,  57,  50,  48,  48, 
     46,  49,  54,  51,  56,  52, 
      0, 171,  73,  83,  71,  78, 
    128,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     92,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,   1,   1,   0,   0, 
    118,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   7,   3,   0,   0, 
     83,  86,  95,  80,  79,  83, 
     73,  84,  73,  79,  78,   0, 
     83,  86,  95,  82,  69,  78, 
     68,  69,  82,  84,  65,  82, 
     71,  69,  84,  65,  82,  82, 
     65,  89,  73,  78,  68,  69, 
     88,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  65,  82,  71,  69, 
     84,   0, 171, 171,  83,  72, 
     68,  82, 112,   2,   0,   0, 
     64,   0,   0,   0, 156,   0, 
      0,   0,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     88,  64,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     68,  68,   0,   0, 100,   8, 
      0,   4,  18,  16,  16,   0, 
      1,   0,   0,   0,   4,   0, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   2,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 104,   0,   0,   2, 
      1,   0,   0,   0, 105,   0, 
      0,   4,   0,   0,   0,   0, 
      6,   0,   0,   0,   4,   0, 
      0,   0,  61,  16,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
     86,   0,   0,   5,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   2,   0,   0,   0, 
     27,   0,   0,   5,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
     66,   0,  16,   0,   0,   0, 
      0,   0,  10,  16,  16,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     45,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,  48,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,  48,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,  48,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,  48,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,  48,  32,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,  48,  32,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,   1,  64,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   6,  18,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  18,  32,  16,   0, 
      0,   0,   0,   0,  10,  48, 
     32,   4,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   6, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  26, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
     34,  32,  16,   0,   0,   0, 
      0,   0,  10,  48,  32,   4, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     42, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     54,   0,   0,   7,  66,  32, 
     16,   0,   0,   0,   0,   0, 
     10,  48,  32,   4,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,   0,  16,   0, 
      0,   0,   0,   0,  58, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7, 130,  32,  16,   0, 
      0,   0,   0,   0,  10,  48, 
     32,   4,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,  22,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,  10,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      7,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
