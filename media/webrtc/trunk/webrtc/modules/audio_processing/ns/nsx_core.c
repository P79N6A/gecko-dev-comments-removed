









#include "webrtc/modules/audio_processing/ns/include/noise_suppression_x.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "webrtc/common_audio/signal_processing/include/real_fft.h"
#include "webrtc/modules/audio_processing/ns/nsx_core.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"

#if (defined WEBRTC_DETECT_ARM_NEON || defined WEBRTC_ARCH_ARM_NEON)

extern const int16_t WebRtcNsx_kLogTable[9];
extern const int16_t WebRtcNsx_kCounterDiv[201];
extern const int16_t WebRtcNsx_kLogTableFrac[256];
#else
static const int16_t WebRtcNsx_kLogTable[9] = {
  0, 177, 355, 532, 710, 887, 1065, 1242, 1420
};

static const int16_t WebRtcNsx_kCounterDiv[201] = {
  32767, 16384, 10923, 8192, 6554, 5461, 4681, 4096, 3641, 3277, 2979, 2731,
  2521, 2341, 2185, 2048, 1928, 1820, 1725, 1638, 1560, 1489, 1425, 1365, 1311,
  1260, 1214, 1170, 1130, 1092, 1057, 1024, 993, 964, 936, 910, 886, 862, 840,
  819, 799, 780, 762, 745, 728, 712, 697, 683, 669, 655, 643, 630, 618, 607,
  596, 585, 575, 565, 555, 546, 537, 529, 520, 512, 504, 496, 489, 482, 475,
  468, 462, 455, 449, 443, 437, 431, 426, 420, 415, 410, 405, 400, 395, 390,
  386, 381, 377, 372, 368, 364, 360, 356, 352, 349, 345, 341, 338, 334, 331,
  328, 324, 321, 318, 315, 312, 309, 306, 303, 301, 298, 295, 293, 290, 287,
  285, 282, 280, 278, 275, 273, 271, 269, 266, 264, 262, 260, 258, 256, 254,
  252, 250, 248, 246, 245, 243, 241, 239, 237, 236, 234, 232, 231, 229, 228,
  226, 224, 223, 221, 220, 218, 217, 216, 214, 213, 211, 210, 209, 207, 206,
  205, 204, 202, 201, 200, 199, 197, 196, 195, 194, 193, 192, 191, 189, 188,
  187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173,
  172, 172, 171, 170, 169, 168, 167, 166, 165, 165, 164, 163
};

static const int16_t WebRtcNsx_kLogTableFrac[256] = {
  0,   1,   3,   4,   6,   7,   9,  10,  11,  13,  14,  16,  17,  18,  20,  21,
  22,  24,  25,  26,  28,  29,  30,  32,  33,  34,  36,  37,  38,  40,  41,  42,
  44,  45,  46,  47,  49,  50,  51,  52,  54,  55,  56,  57,  59,  60,  61,  62,
  63,  65,  66,  67,  68,  69,  71,  72,  73,  74,  75,  77,  78,  79,  80,  81,
  82,  84,  85,  86,  87,  88,  89,  90,  92,  93,  94,  95,  96,  97,  98,  99,
  100, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 116,
  117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
  147, 148, 149, 150, 151, 152, 153, 154, 155, 155, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 174,
  175, 176, 177, 178, 178, 179, 180, 181, 182, 183, 184, 185, 185, 186, 187,
  188, 189, 190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200,
  201, 202, 203, 203, 204, 205, 206, 207, 208, 208, 209, 210, 211, 212, 212,
  213, 214, 215, 216, 216, 217, 218, 219, 220, 220, 221, 222, 223, 224, 224,
  225, 226, 227, 228, 228, 229, 230, 231, 231, 232, 233, 234, 234, 235, 236,
  237, 238, 238, 239, 240, 241, 241, 242, 243, 244, 244, 245, 246, 247, 247,
  248, 249, 249, 250, 251, 252, 252, 253, 254, 255, 255
};
#endif  


static const int kStartBand = 5;


static const int16_t kBlocks80w128x[128] = {
  0,    536,   1072,   1606,   2139,   2669,   3196,   3720,   4240,   4756,   5266,
  5771,   6270,   6762,   7246,   7723,   8192,   8652,   9102,   9543,   9974,  10394,
  10803,  11200,  11585,  11958,  12318,  12665,  12998,  13318,  13623,  13913,  14189,
  14449,  14694,  14924,  15137,  15334,  15515,  15679,  15826,  15956,  16069,  16165,
  16244,  16305,  16349,  16375,  16384,  16384,  16384,  16384,  16384,  16384,  16384,
  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,
  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,
  16384,  16384,  16384,  16384,  16375,  16349,  16305,  16244,  16165,  16069,  15956,
  15826,  15679,  15515,  15334,  15137,  14924,  14694,  14449,  14189,  13913,  13623,
  13318,  12998,  12665,  12318,  11958,  11585,  11200,  10803,  10394,   9974,   9543,
  9102,   8652,   8192,   7723,   7246,   6762,   6270,   5771,   5266,   4756,   4240,
  3720,   3196,   2669,   2139,   1606,   1072,    536
};


static const int16_t kBlocks160w256x[256] = {
  0,   268,   536,   804,  1072,  1339,  1606,  1872,
  2139,  2404,  2669,  2933,  3196,  3459,  3720,  3981,
  4240,  4499,  4756,  5012,  5266,  5520,  5771,  6021,
  6270,  6517,  6762,  7005,  7246,  7486,  7723,  7959,
  8192,  8423,  8652,  8878,  9102,  9324,  9543,  9760,
  9974, 10185, 10394, 10600, 10803, 11003, 11200, 11394,
  11585, 11773, 11958, 12140, 12318, 12493, 12665, 12833,
  12998, 13160, 13318, 13472, 13623, 13770, 13913, 14053,
  14189, 14321, 14449, 14574, 14694, 14811, 14924, 15032,
  15137, 15237, 15334, 15426, 15515, 15599, 15679, 15754,
  15826, 15893, 15956, 16015, 16069, 16119, 16165, 16207,
  16244, 16277, 16305, 16329, 16349, 16364, 16375, 16382,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
  16384, 16382, 16375, 16364, 16349, 16329, 16305, 16277,
  16244, 16207, 16165, 16119, 16069, 16015, 15956, 15893,
  15826, 15754, 15679, 15599, 15515, 15426, 15334, 15237,
  15137, 15032, 14924, 14811, 14694, 14574, 14449, 14321,
  14189, 14053, 13913, 13770, 13623, 13472, 13318, 13160,
  12998, 12833, 12665, 12493, 12318, 12140, 11958, 11773,
  11585, 11394, 11200, 11003, 10803, 10600, 10394, 10185,
  9974,  9760,  9543,  9324,  9102,  8878,  8652,  8423,
  8192,  7959,  7723,  7486,  7246,  7005,  6762,  6517,
  6270,  6021,  5771,  5520,  5266,  5012,  4756,  4499,
  4240,  3981,  3720,  3459,  3196,  2933,  2669,  2404,
  2139,  1872,  1606,  1339,  1072,   804,   536,   268
};











static const int16_t kFactor1Table[257] = {
  8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8233, 8274, 8315, 8355, 8396, 8436, 8475, 8515, 8554, 8592, 8631, 8669,
  8707, 8745, 8783, 8820, 8857, 8894, 8931, 8967, 9003, 9039, 9075, 9111, 9146, 9181,
  9216, 9251, 9286, 9320, 9354, 9388, 9422, 9456, 9489, 9523, 9556, 9589, 9622, 9655,
  9687, 9719, 9752, 9784, 9816, 9848, 9879, 9911, 9942, 9973, 10004, 10035, 10066,
  10097, 10128, 10158, 10188, 10218, 10249, 10279, 10308, 10338, 10368, 10397, 10426,
  10456, 10485, 10514, 10543, 10572, 10600, 10629, 10657, 10686, 10714, 10742, 10770,
  10798, 10826, 10854, 10882, 10847, 10810, 10774, 10737, 10701, 10666, 10631, 10596,
  10562, 10527, 10494, 10460, 10427, 10394, 10362, 10329, 10297, 10266, 10235, 10203,
  10173, 10142, 10112, 10082, 10052, 10023, 9994, 9965, 9936, 9908, 9879, 9851, 9824,
  9796, 9769, 9742, 9715, 9689, 9662, 9636, 9610, 9584, 9559, 9534, 9508, 9484, 9459,
  9434, 9410, 9386, 9362, 9338, 9314, 9291, 9268, 9245, 9222, 9199, 9176, 9154, 9132,
  9110, 9088, 9066, 9044, 9023, 9002, 8980, 8959, 8939, 8918, 8897, 8877, 8857, 8836,
  8816, 8796, 8777, 8757, 8738, 8718, 8699, 8680, 8661, 8642, 8623, 8605, 8586, 8568,
  8550, 8532, 8514, 8496, 8478, 8460, 8443, 8425, 8408, 8391, 8373, 8356, 8339, 8323,
  8306, 8289, 8273, 8256, 8240, 8224, 8208, 8192
};













static const int16_t kFactor2Aggressiveness1[257] = {
  7577, 7577, 7577, 7577, 7577, 7577,
  7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7577, 7596, 7614, 7632,
  7650, 7667, 7683, 7699, 7715, 7731, 7746, 7761, 7775, 7790, 7804, 7818, 7832, 7845,
  7858, 7871, 7884, 7897, 7910, 7922, 7934, 7946, 7958, 7970, 7982, 7993, 8004, 8016,
  8027, 8038, 8049, 8060, 8070, 8081, 8091, 8102, 8112, 8122, 8132, 8143, 8152, 8162,
  8172, 8182, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};


static const int16_t kFactor2Aggressiveness2[257] = {
  7270, 7270, 7270, 7270, 7270, 7306,
  7339, 7369, 7397, 7424, 7448, 7472, 7495, 7517, 7537, 7558, 7577, 7596, 7614, 7632,
  7650, 7667, 7683, 7699, 7715, 7731, 7746, 7761, 7775, 7790, 7804, 7818, 7832, 7845,
  7858, 7871, 7884, 7897, 7910, 7922, 7934, 7946, 7958, 7970, 7982, 7993, 8004, 8016,
  8027, 8038, 8049, 8060, 8070, 8081, 8091, 8102, 8112, 8122, 8132, 8143, 8152, 8162,
  8172, 8182, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};


static const int16_t kFactor2Aggressiveness3[257] = {
  7184, 7184, 7184, 7229, 7270, 7306,
  7339, 7369, 7397, 7424, 7448, 7472, 7495, 7517, 7537, 7558, 7577, 7596, 7614, 7632,
  7650, 7667, 7683, 7699, 7715, 7731, 7746, 7761, 7775, 7790, 7804, 7818, 7832, 7845,
  7858, 7871, 7884, 7897, 7910, 7922, 7934, 7946, 7958, 7970, 7982, 7993, 8004, 8016,
  8027, 8038, 8049, 8060, 8070, 8081, 8091, 8102, 8112, 8122, 8132, 8143, 8152, 8162,
  8172, 8182, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};



static const int16_t kSumLogIndex[66] = {
  0,  22917,  22917,  22885,  22834,  22770,  22696,  22613,
  22524,  22428,  22326,  22220,  22109,  21994,  21876,  21754,
  21629,  21501,  21370,  21237,  21101,  20963,  20822,  20679,
  20535,  20388,  20239,  20089,  19937,  19783,  19628,  19470,
  19312,  19152,  18991,  18828,  18664,  18498,  18331,  18164,
  17994,  17824,  17653,  17480,  17306,  17132,  16956,  16779,
  16602,  16423,  16243,  16063,  15881,  15699,  15515,  15331,
  15146,  14960,  14774,  14586,  14398,  14209,  14019,  13829,
  13637,  13445
};



static const int16_t kSumSquareLogIndex[66] = {
  0,  16959,  16959,  16955,  16945,  16929,  16908,  16881,
  16850,  16814,  16773,  16729,  16681,  16630,  16575,  16517,
  16456,  16392,  16325,  16256,  16184,  16109,  16032,  15952,
  15870,  15786,  15700,  15612,  15521,  15429,  15334,  15238,
  15140,  15040,  14938,  14834,  14729,  14622,  14514,  14404,
  14292,  14179,  14064,  13947,  13830,  13710,  13590,  13468,
  13344,  13220,  13094,  12966,  12837,  12707,  12576,  12444,
  12310,  12175,  12039,  11902,  11763,  11624,  11483,  11341,
  11198,  11054
};



static const int16_t kLogIndex[129] = {
  0,      0,   4096,   6492,   8192,   9511,  10588,  11499,
  12288,  12984,  13607,  14170,  14684,  15157,  15595,  16003,
  16384,  16742,  17080,  17400,  17703,  17991,  18266,  18529,
  18780,  19021,  19253,  19476,  19691,  19898,  20099,  20292,
  20480,  20662,  20838,  21010,  21176,  21338,  21496,  21649,
  21799,  21945,  22087,  22226,  22362,  22495,  22625,  22752,
  22876,  22998,  23117,  23234,  23349,  23462,  23572,  23680,
  23787,  23892,  23994,  24095,  24195,  24292,  24388,  24483,
  24576,  24668,  24758,  24847,  24934,  25021,  25106,  25189,
  25272,  25354,  25434,  25513,  25592,  25669,  25745,  25820,
  25895,  25968,  26041,  26112,  26183,  26253,  26322,  26390,
  26458,  26525,  26591,  26656,  26721,  26784,  26848,  26910,
  26972,  27033,  27094,  27154,  27213,  27272,  27330,  27388,
  27445,  27502,  27558,  27613,  27668,  27722,  27776,  27830,
  27883,  27935,  27988,  28039,  28090,  28141,  28191,  28241,
  28291,  28340,  28388,  28437,  28484,  28532,  28579,  28626,
  28672
};



static const int16_t kDeterminantEstMatrix[66] = {
  0,  29814,  25574,  22640,  20351,  18469,  16873,  15491,
  14277,  13199,  12233,  11362,  10571,   9851,   9192,   8587,
  8030,   7515,   7038,   6596,   6186,   5804,   5448,   5115,
  4805,   4514,   4242,   3988,   3749,   3524,   3314,   3116,
  2930,   2755,   2590,   2435,   2289,   2152,   2022,   1900,
  1785,   1677,   1575,   1478,   1388,   1302,   1221,   1145,
  1073,   1005,    942,    881,    825,    771,    721,    674,
  629,    587,    547,    510,    475,    442,    411,    382,
  355,    330
};


static void UpdateNoiseEstimate(NsxInst_t* inst, int offset) {
  int32_t tmp32no1 = 0;
  int32_t tmp32no2 = 0;
  int16_t tmp16 = 0;
  const int16_t kExp2Const = 11819; 

  int i = 0;

  tmp16 = WebRtcSpl_MaxValueW16(inst->noiseEstLogQuantile + offset,
                                   inst->magnLen);
  
  inst->qNoise = 14 - (int) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                   kExp2Const, tmp16, 21);
  for (i = 0; i < inst->magnLen; i++) {
    
    
    tmp32no2 = WEBRTC_SPL_MUL_16_16(kExp2Const,
                                    inst->noiseEstLogQuantile[offset + i]);
    tmp32no1 = (0x00200000 | (tmp32no2 & 0x001FFFFF)); 
    tmp16 = (int16_t) WEBRTC_SPL_RSHIFT_W32(tmp32no2, 21);
    tmp16 -= 21;
    tmp16 += (int16_t) inst->qNoise; 
    if (tmp16 < 0) {
      tmp32no1 = WEBRTC_SPL_RSHIFT_W32(tmp32no1, -tmp16);
    } else {
      tmp32no1 = WEBRTC_SPL_LSHIFT_W32(tmp32no1, tmp16);
    }
    inst->noiseEstQuantile[i] = WebRtcSpl_SatW32ToW16(tmp32no1);
  }
}


static void NoiseEstimationC(NsxInst_t* inst,
                             uint16_t* magn,
                             uint32_t* noise,
                             int16_t* q_noise) {
  int16_t lmagn[HALF_ANAL_BLOCKL], counter, countDiv;
  int16_t countProd, delta, zeros, frac;
  int16_t log2, tabind, logval, tmp16, tmp16no1, tmp16no2;
  const int16_t log2_const = 22713; 
  const int16_t width_factor = 21845;

  int i, s, offset;

  tabind = inst->stages - inst->normData;
  assert(tabind < 9);
  assert(tabind > -9);
  if (tabind < 0) {
    logval = -WebRtcNsx_kLogTable[-tabind];
  } else {
    logval = WebRtcNsx_kLogTable[tabind];
  }

  
  
  
  
  for (i = 0; i < inst->magnLen; i++) {
    if (magn[i]) {
      zeros = WebRtcSpl_NormU32((uint32_t)magn[i]);
      frac = (int16_t)((((uint32_t)magn[i] << zeros)
                              & 0x7FFFFFFF) >> 23);
      
      assert(frac < 256);
      log2 = (int16_t)(((31 - zeros) << 8)
                             + WebRtcNsx_kLogTableFrac[frac]);
      
      lmagn[i] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(log2, log2_const, 15);
      
      lmagn[i] += logval;
    } else {
      lmagn[i] = logval;
    }
  }

  
  for (s = 0; s < SIMULT; s++) {
    offset = s * inst->magnLen;

    
    counter = inst->noiseEstCounter[s];
    assert(counter < 201);
    countDiv = WebRtcNsx_kCounterDiv[counter];
    countProd = (int16_t)WEBRTC_SPL_MUL_16_16(counter, countDiv);

    
    for (i = 0; i < inst->magnLen; i++) {
      
      if (inst->noiseEstDensity[offset + i] > 512) {
        
        int factor = WebRtcSpl_NormW16(inst->noiseEstDensity[offset + i]);
        delta = (int16_t)(FACTOR_Q16 >> (14 - factor));
      } else {
        delta = FACTOR_Q7;
        if (inst->blockIndex < END_STARTUP_LONG) {
          
          
          delta = FACTOR_Q7_STARTUP;
        }
      }

      
      tmp16 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(delta, countDiv, 14);
      if (lmagn[i] > inst->noiseEstLogQuantile[offset + i]) {
        
        
        tmp16 += 2;
        tmp16no1 = WEBRTC_SPL_RSHIFT_W16(tmp16, 2);
        inst->noiseEstLogQuantile[offset + i] += tmp16no1;
      } else {
        tmp16 += 1;
        tmp16no1 = WEBRTC_SPL_RSHIFT_W16(tmp16, 1);
        
        tmp16no2 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tmp16no1, 3, 1);
        inst->noiseEstLogQuantile[offset + i] -= tmp16no2;
        if (inst->noiseEstLogQuantile[offset + i] < logval) {
          
          
          inst->noiseEstLogQuantile[offset + i] = logval;
        }
      }

      
      if (WEBRTC_SPL_ABS_W16(lmagn[i] - inst->noiseEstLogQuantile[offset + i])
          < WIDTH_Q8) {
        tmp16no1 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                     inst->noiseEstDensity[offset + i], countProd, 15);
        tmp16no2 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                     width_factor, countDiv, 15);
        inst->noiseEstDensity[offset + i] = tmp16no1 + tmp16no2;
      }
    }  

    if (counter >= END_STARTUP_LONG) {
      inst->noiseEstCounter[s] = 0;
      if (inst->blockIndex >= END_STARTUP_LONG) {
        UpdateNoiseEstimate(inst, offset);
      }
    }
    inst->noiseEstCounter[s]++;

  }  

  
  if (inst->blockIndex < END_STARTUP_LONG) {
    UpdateNoiseEstimate(inst, offset);
  }

  for (i = 0; i < inst->magnLen; i++) {
    noise[i] = (uint32_t)(inst->noiseEstQuantile[i]); 
  }
  (*q_noise) = (int16_t)inst->qNoise;
}


static void PrepareSpectrumC(NsxInst_t* inst, int16_t* freq_buf) {
  int i = 0, j = 0;

  for (i = 0; i < inst->magnLen; i++) {
    inst->real[i] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(inst->real[i],
        (int16_t)(inst->noiseSupFilter[i]), 14); 
    inst->imag[i] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(inst->imag[i],
        (int16_t)(inst->noiseSupFilter[i]), 14); 
  }

  freq_buf[0] = inst->real[0];
  freq_buf[1] = -inst->imag[0];
  for (i = 1, j = 2; i < inst->anaLen2; i += 1, j += 2) {
    freq_buf[j] = inst->real[i];
    freq_buf[j + 1] = -inst->imag[i];
  }
  freq_buf[inst->anaLen] = inst->real[inst->anaLen2];
  freq_buf[inst->anaLen + 1] = -inst->imag[inst->anaLen2];
}


static void DenormalizeC(NsxInst_t* inst, int16_t* in, int factor) {
  int i = 0;
  int32_t tmp32 = 0;
  for (i = 0; i < inst->anaLen; i += 1) {
    tmp32 = WEBRTC_SPL_SHIFT_W32((int32_t)in[i],
                                 factor - inst->normData);
    inst->real[i] = WebRtcSpl_SatW32ToW16(tmp32); 
  }
}



static void SynthesisUpdateC(NsxInst_t* inst,
                             int16_t* out_frame,
                             int16_t gain_factor) {
  int i = 0;
  int16_t tmp16a = 0;
  int16_t tmp16b = 0;
  int32_t tmp32 = 0;

  
  for (i = 0; i < inst->anaLen; i++) {
    tmp16a = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                 inst->window[i], inst->real[i], 14); 
    tmp32 = WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(tmp16a, gain_factor, 13); 
    
    tmp16b = WebRtcSpl_SatW32ToW16(tmp32); 
    inst->synthesisBuffer[i] = WEBRTC_SPL_ADD_SAT_W16(inst->synthesisBuffer[i],
                                                      tmp16b); 
  }

  
  for (i = 0; i < inst->blockLen10ms; i++) {
    out_frame[i] = inst->synthesisBuffer[i]; 
  }

  
  WEBRTC_SPL_MEMCPY_W16(inst->synthesisBuffer,
                        inst->synthesisBuffer + inst->blockLen10ms,
                        inst->anaLen - inst->blockLen10ms);
  WebRtcSpl_ZerosArrayW16(inst->synthesisBuffer
      + inst->anaLen - inst->blockLen10ms, inst->blockLen10ms);
}


static void AnalysisUpdateC(NsxInst_t* inst,
                            int16_t* out,
                            int16_t* new_speech) {
  int i = 0;

  
  WEBRTC_SPL_MEMCPY_W16(inst->analysisBuffer,
                        inst->analysisBuffer + inst->blockLen10ms,
                        inst->anaLen - inst->blockLen10ms);
  WEBRTC_SPL_MEMCPY_W16(inst->analysisBuffer
      + inst->anaLen - inst->blockLen10ms, new_speech, inst->blockLen10ms);

  
  for (i = 0; i < inst->anaLen; i++) {
    out[i] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
               inst->window[i], inst->analysisBuffer[i], 14); 
  }
}


static void NormalizeRealBufferC(NsxInst_t* inst,
                                 const int16_t* in,
                                 int16_t* out) {
  int i = 0;
  for (i = 0; i < inst->anaLen; ++i) {
    out[i] = WEBRTC_SPL_LSHIFT_W16(in[i], inst->normData); 
  }
}


NoiseEstimation WebRtcNsx_NoiseEstimation;
PrepareSpectrum WebRtcNsx_PrepareSpectrum;
SynthesisUpdate WebRtcNsx_SynthesisUpdate;
AnalysisUpdate WebRtcNsx_AnalysisUpdate;
Denormalize WebRtcNsx_Denormalize;
NormalizeRealBuffer WebRtcNsx_NormalizeRealBuffer;

#if (defined WEBRTC_DETECT_ARM_NEON || defined WEBRTC_ARCH_ARM_NEON)

static void WebRtcNsx_InitNeon(void) {
  WebRtcNsx_NoiseEstimation = WebRtcNsx_NoiseEstimationNeon;
  WebRtcNsx_PrepareSpectrum = WebRtcNsx_PrepareSpectrumNeon;
  WebRtcNsx_SynthesisUpdate = WebRtcNsx_SynthesisUpdateNeon;
  WebRtcNsx_AnalysisUpdate = WebRtcNsx_AnalysisUpdateNeon;
}
#endif

#if defined(MIPS32_LE)

static void WebRtcNsx_InitMips(void) {
  WebRtcNsx_PrepareSpectrum = WebRtcNsx_PrepareSpectrum_mips;
  WebRtcNsx_SynthesisUpdate = WebRtcNsx_SynthesisUpdate_mips;
  WebRtcNsx_AnalysisUpdate = WebRtcNsx_AnalysisUpdate_mips;
  WebRtcNsx_NormalizeRealBuffer = WebRtcNsx_NormalizeRealBuffer_mips;
#if defined(MIPS_DSP_R1_LE)
  WebRtcNsx_Denormalize = WebRtcNsx_Denormalize_mips;
#endif
}
#endif

void WebRtcNsx_CalcParametricNoiseEstimate(NsxInst_t* inst,
                                           int16_t pink_noise_exp_avg,
                                           int32_t pink_noise_num_avg,
                                           int freq_index,
                                           uint32_t* noise_estimate,
                                           uint32_t* noise_estimate_avg) {
  int32_t tmp32no1 = 0;
  int32_t tmp32no2 = 0;

  int16_t int_part = 0;
  int16_t frac_part = 0;

  
  
  assert(freq_index >= 0);
  assert(freq_index < 129);
  tmp32no2 = WEBRTC_SPL_MUL_16_16(pink_noise_exp_avg, kLogIndex[freq_index]); 
  tmp32no2 = WEBRTC_SPL_RSHIFT_W32(tmp32no2, 15); 
  tmp32no1 = pink_noise_num_avg - tmp32no2; 

  
  
  tmp32no1 += WEBRTC_SPL_LSHIFT_W32((int32_t)(inst->minNorm - inst->stages), 11);
  if (tmp32no1 > 0) {
    int_part = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32no1, 11);
    frac_part = (int16_t)(tmp32no1 & 0x000007ff); 
    
    
    
    if (WEBRTC_SPL_RSHIFT_W16(frac_part, 10)) {
      
      tmp32no2 = WEBRTC_SPL_MUL_16_16(2048 - frac_part, 1244); 
      tmp32no2 = 2048 - WEBRTC_SPL_RSHIFT_W32(tmp32no2, 10);
    } else {
      
      tmp32no2 = WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL_16_16(frac_part, 804), 10);
    }
    
    tmp32no2 = WEBRTC_SPL_SHIFT_W32(tmp32no2, int_part - 11);
    *noise_estimate_avg = WEBRTC_SPL_LSHIFT_U32(1, int_part) + (uint32_t)tmp32no2;
    
    *noise_estimate = (*noise_estimate_avg) * (uint32_t)(inst->blockIndex + 1);
  }
}


int32_t WebRtcNsx_InitCore(NsxInst_t* inst, uint32_t fs) {
  int i;

  
  if (inst == NULL) {
    return -1;
  }
  

  
  if (fs == 8000 || fs == 16000 || fs == 32000) {
    inst->fs = fs;
  } else {
    return -1;
  }

  if (fs == 8000) {
    inst->blockLen10ms = 80;
    inst->anaLen = 128;
    inst->stages = 7;
    inst->window = kBlocks80w128x;
    inst->thresholdLogLrt = 131072; 
    inst->maxLrt = 0x0040000;
    inst->minLrt = 52429;
  } else if (fs == 16000) {
    inst->blockLen10ms = 160;
    inst->anaLen = 256;
    inst->stages = 8;
    inst->window = kBlocks160w256x;
    inst->thresholdLogLrt = 212644; 
    inst->maxLrt = 0x0080000;
    inst->minLrt = 104858;
  } else if (fs == 32000) {
    inst->blockLen10ms = 160;
    inst->anaLen = 256;
    inst->stages = 8;
    inst->window = kBlocks160w256x;
    inst->thresholdLogLrt = 212644; 
    inst->maxLrt = 0x0080000;
    inst->minLrt = 104858;
  }
  inst->anaLen2 = WEBRTC_SPL_RSHIFT_W16(inst->anaLen, 1);
  inst->magnLen = inst->anaLen2 + 1;

  if (inst->real_fft != NULL) {
    WebRtcSpl_FreeRealFFT(inst->real_fft);
  }
  inst->real_fft = WebRtcSpl_CreateRealFFT(inst->stages);
  if (inst->real_fft == NULL) {
    return -1;
  }

  WebRtcSpl_ZerosArrayW16(inst->analysisBuffer, ANAL_BLOCKL_MAX);
  WebRtcSpl_ZerosArrayW16(inst->synthesisBuffer, ANAL_BLOCKL_MAX);

  
  WebRtcSpl_ZerosArrayW16(inst->dataBufHBFX, ANAL_BLOCKL_MAX);
  
  WebRtcSpl_ZerosArrayW16(inst->noiseEstQuantile, HALF_ANAL_BLOCKL);
  for (i = 0; i < SIMULT * HALF_ANAL_BLOCKL; i++) {
    inst->noiseEstLogQuantile[i] = 2048; 
    inst->noiseEstDensity[i] = 153; 
  }
  for (i = 0; i < SIMULT; i++) {
    inst->noiseEstCounter[i] = (int16_t)(END_STARTUP_LONG * (i + 1)) / SIMULT;
  }

  
  WebRtcSpl_MemSetW16((int16_t*)inst->noiseSupFilter, 16384, HALF_ANAL_BLOCKL);

  
  inst->aggrMode = 0;

  
  inst->priorNonSpeechProb = 8192; 
  for (i = 0; i < HALF_ANAL_BLOCKL; i++) {
    inst->prevMagnU16[i] = 0;
    inst->prevNoiseU32[i] = 0; 
    inst->logLrtTimeAvgW32[i] = 0; 
    inst->avgMagnPause[i] = 0; 
    inst->initMagnEst[i] = 0; 
  }

  
  inst->thresholdSpecDiff = 50; 
  inst->thresholdSpecFlat = 20480; 
  inst->featureLogLrt = inst->thresholdLogLrt; 
  inst->featureSpecFlat = inst->thresholdSpecFlat; 
  inst->featureSpecDiff = inst->thresholdSpecDiff; 
  inst->weightLogLrt = 6; 
  inst->weightSpecFlat = 0; 
  inst->weightSpecDiff = 0; 

  inst->curAvgMagnEnergy = 0; 
  inst->timeAvgMagnEnergy = 0; 
  inst->timeAvgMagnEnergyTmp = 0; 

  
  WebRtcSpl_ZerosArrayW16(inst->histLrt, HIST_PAR_EST);
  WebRtcSpl_ZerosArrayW16(inst->histSpecDiff, HIST_PAR_EST);
  WebRtcSpl_ZerosArrayW16(inst->histSpecFlat, HIST_PAR_EST);

  inst->blockIndex = -1; 

  
  inst->modelUpdate = (1 << STAT_UPDATES); 
  inst->cntThresUpdate = 0; 

  inst->sumMagn = 0;
  inst->magnEnergy = 0;
  inst->prevQMagn = 0;
  inst->qNoise = 0;
  inst->prevQNoise = 0;

  inst->energyIn = 0;
  inst->scaleEnergyIn = 0;

  inst->whiteNoiseLevel = 0;
  inst->pinkNoiseNumerator = 0;
  inst->pinkNoiseExp = 0;
  inst->minNorm = 15; 
  inst->zeroInputSignal = 0;

  
  WebRtcNsx_set_policy_core(inst, 0);

#ifdef NS_FILEDEBUG
  inst->infile = fopen("indebug.pcm", "wb");
  inst->outfile = fopen("outdebug.pcm", "wb");
  inst->file1 = fopen("file1.pcm", "wb");
  inst->file2 = fopen("file2.pcm", "wb");
  inst->file3 = fopen("file3.pcm", "wb");
  inst->file4 = fopen("file4.pcm", "wb");
  inst->file5 = fopen("file5.pcm", "wb");
#endif

  
  WebRtcNsx_NoiseEstimation = NoiseEstimationC;
  WebRtcNsx_PrepareSpectrum = PrepareSpectrumC;
  WebRtcNsx_SynthesisUpdate = SynthesisUpdateC;
  WebRtcNsx_AnalysisUpdate = AnalysisUpdateC;
  WebRtcNsx_Denormalize = DenormalizeC;
  WebRtcNsx_NormalizeRealBuffer = NormalizeRealBufferC;

#ifdef WEBRTC_DETECT_ARM_NEON
  uint64_t features = WebRtc_GetCPUFeaturesARM();
  if ((features & kCPUFeatureNEON) != 0) {
      WebRtcNsx_InitNeon();
  }
#elif defined(WEBRTC_ARCH_ARM_NEON)
  WebRtcNsx_InitNeon();
#endif

#if defined(MIPS32_LE)
  WebRtcNsx_InitMips();
#endif

  inst->initFlag = 1;

  return 0;
}

int WebRtcNsx_set_policy_core(NsxInst_t* inst, int mode) {
  
  if (mode < 0 || mode > 3) {
    return -1;
  }

  inst->aggrMode = mode;
  if (mode == 0) {
    inst->overdrive = 256; 
    inst->denoiseBound = 8192; 
    inst->gainMap = 0; 
  } else if (mode == 1) {
    inst->overdrive = 256; 
    inst->denoiseBound = 4096; 
    inst->factor2Table = kFactor2Aggressiveness1;
    inst->gainMap = 1;
  } else if (mode == 2) {
    inst->overdrive = 282; 
    inst->denoiseBound = 2048; 
    inst->factor2Table = kFactor2Aggressiveness2;
    inst->gainMap = 1;
  } else if (mode == 3) {
    inst->overdrive = 320; 
    inst->denoiseBound = 1475; 
    inst->factor2Table = kFactor2Aggressiveness3;
    inst->gainMap = 1;
  }
  return 0;
}






void WebRtcNsx_FeatureParameterExtraction(NsxInst_t* inst, int flag) {
  uint32_t tmpU32;
  uint32_t histIndex;
  uint32_t posPeak1SpecFlatFX, posPeak2SpecFlatFX;
  uint32_t posPeak1SpecDiffFX, posPeak2SpecDiffFX;

  int32_t tmp32;
  int32_t fluctLrtFX, thresFluctLrtFX;
  int32_t avgHistLrtFX, avgSquareHistLrtFX, avgHistLrtComplFX;

  int16_t j;
  int16_t numHistLrt;

  int i;
  int useFeatureSpecFlat, useFeatureSpecDiff, featureSum;
  int maxPeak1, maxPeak2;
  int weightPeak1SpecFlat, weightPeak2SpecFlat;
  int weightPeak1SpecDiff, weightPeak2SpecDiff;

  
  if (!flag) {
    
    
    
    histIndex = (uint32_t)(inst->featureLogLrt);
    if (histIndex < HIST_PAR_EST) {
      inst->histLrt[histIndex]++;
    }
    
    
    histIndex = WEBRTC_SPL_RSHIFT_U32(inst->featureSpecFlat * 5, 8);
    if (histIndex < HIST_PAR_EST) {
      inst->histSpecFlat[histIndex]++;
    }
    
    histIndex = HIST_PAR_EST;
    if (inst->timeAvgMagnEnergy > 0) {
      
      
      
      histIndex = WEBRTC_SPL_UDIV((inst->featureSpecDiff * 5) >> inst->stages,
                                  inst->timeAvgMagnEnergy);
    }
    if (histIndex < HIST_PAR_EST) {
      inst->histSpecDiff[histIndex]++;
    }
  }

  
  if (flag) {
    useFeatureSpecDiff = 1;
    
    
    avgHistLrtFX = 0;
    avgSquareHistLrtFX = 0;
    numHistLrt = 0;
    for (i = 0; i < BIN_SIZE_LRT; i++) {
      j = (2 * i + 1);
      tmp32 = WEBRTC_SPL_MUL_16_16(inst->histLrt[i], j);
      avgHistLrtFX += tmp32;
      numHistLrt += inst->histLrt[i];
      avgSquareHistLrtFX += WEBRTC_SPL_MUL_32_16(tmp32, j);
    }
    avgHistLrtComplFX = avgHistLrtFX;
    for (; i < HIST_PAR_EST; i++) {
      j = (2 * i + 1);
      tmp32 = WEBRTC_SPL_MUL_16_16(inst->histLrt[i], j);
      avgHistLrtComplFX += tmp32;
      avgSquareHistLrtFX += WEBRTC_SPL_MUL_32_16(tmp32, j);
    }
    fluctLrtFX = WEBRTC_SPL_MUL(avgSquareHistLrtFX, numHistLrt);
    fluctLrtFX -= WEBRTC_SPL_MUL(avgHistLrtFX, avgHistLrtComplFX);
    thresFluctLrtFX = THRES_FLUCT_LRT * numHistLrt;
    
    tmpU32 = (FACTOR_1_LRT_DIFF * (uint32_t)avgHistLrtFX);
    if ((fluctLrtFX < thresFluctLrtFX) || (numHistLrt == 0) ||
        (tmpU32 > (uint32_t)(100 * numHistLrt))) {
      
      inst->thresholdLogLrt = inst->maxLrt;
    } else {
      tmp32 = (int32_t)((tmpU32 << (9 + inst->stages)) / numHistLrt /
                              25);
      
      inst->thresholdLogLrt = WEBRTC_SPL_SAT(inst->maxLrt,
                                             tmp32,
                                             inst->minLrt);
    }
    if (fluctLrtFX < thresFluctLrtFX) {
      
      
      useFeatureSpecDiff = 0;
    }

    
    maxPeak1 = 0;
    maxPeak2 = 0;
    posPeak1SpecFlatFX = 0;
    posPeak2SpecFlatFX = 0;
    weightPeak1SpecFlat = 0;
    weightPeak2SpecFlat = 0;

    
    for (i = 0; i < HIST_PAR_EST; i++) {
      if (inst->histSpecFlat[i] > maxPeak1) {
        
        maxPeak2 = maxPeak1;
        weightPeak2SpecFlat = weightPeak1SpecFlat;
        posPeak2SpecFlatFX = posPeak1SpecFlatFX;

        maxPeak1 = inst->histSpecFlat[i];
        weightPeak1SpecFlat = inst->histSpecFlat[i];
        posPeak1SpecFlatFX = (uint32_t)(2 * i + 1);
      } else if (inst->histSpecFlat[i] > maxPeak2) {
        
        maxPeak2 = inst->histSpecFlat[i];
        weightPeak2SpecFlat = inst->histSpecFlat[i];
        posPeak2SpecFlatFX = (uint32_t)(2 * i + 1);
      }
    }

    
    useFeatureSpecFlat = 1;
    
    if ((posPeak1SpecFlatFX - posPeak2SpecFlatFX < LIM_PEAK_SPACE_FLAT_DIFF)
        && (weightPeak2SpecFlat * LIM_PEAK_WEIGHT_FLAT_DIFF > weightPeak1SpecFlat)) {
      weightPeak1SpecFlat += weightPeak2SpecFlat;
      posPeak1SpecFlatFX = (posPeak1SpecFlatFX + posPeak2SpecFlatFX) >> 1;
    }
    
    if (weightPeak1SpecFlat < THRES_WEIGHT_FLAT_DIFF || posPeak1SpecFlatFX
        < THRES_PEAK_FLAT) {
      useFeatureSpecFlat = 0;
    } else { 
      
      inst->thresholdSpecFlat = WEBRTC_SPL_SAT(MAX_FLAT_Q10, FACTOR_2_FLAT_Q10
                                               * posPeak1SpecFlatFX, MIN_FLAT_Q10); 
    }
    

    if (useFeatureSpecDiff) {
      
      maxPeak1 = 0;
      maxPeak2 = 0;
      posPeak1SpecDiffFX = 0;
      posPeak2SpecDiffFX = 0;
      weightPeak1SpecDiff = 0;
      weightPeak2SpecDiff = 0;
      
      for (i = 0; i < HIST_PAR_EST; i++) {
        if (inst->histSpecDiff[i] > maxPeak1) {
          
          maxPeak2 = maxPeak1;
          weightPeak2SpecDiff = weightPeak1SpecDiff;
          posPeak2SpecDiffFX = posPeak1SpecDiffFX;

          maxPeak1 = inst->histSpecDiff[i];
          weightPeak1SpecDiff = inst->histSpecDiff[i];
          posPeak1SpecDiffFX = (uint32_t)(2 * i + 1);
        } else if (inst->histSpecDiff[i] > maxPeak2) {
          
          maxPeak2 = inst->histSpecDiff[i];
          weightPeak2SpecDiff = inst->histSpecDiff[i];
          posPeak2SpecDiffFX = (uint32_t)(2 * i + 1);
        }
      }

      
      if ((posPeak1SpecDiffFX - posPeak2SpecDiffFX < LIM_PEAK_SPACE_FLAT_DIFF)
          && (weightPeak2SpecDiff * LIM_PEAK_WEIGHT_FLAT_DIFF > weightPeak1SpecDiff)) {
        weightPeak1SpecDiff += weightPeak2SpecDiff;
        posPeak1SpecDiffFX = (posPeak1SpecDiffFX + posPeak2SpecDiffFX) >> 1;
      }
      
      inst->thresholdSpecDiff = WEBRTC_SPL_SAT(MAX_DIFF, FACTOR_1_LRT_DIFF
                                               * posPeak1SpecDiffFX, MIN_DIFF); 
      
      if (weightPeak1SpecDiff < THRES_WEIGHT_FLAT_DIFF) {
        useFeatureSpecDiff = 0;
      }
      
    }

    
    
    featureSum = 6 / (1 + useFeatureSpecFlat + useFeatureSpecDiff);
    inst->weightLogLrt = featureSum;
    inst->weightSpecFlat = useFeatureSpecFlat * featureSum;
    inst->weightSpecDiff = useFeatureSpecDiff * featureSum;

    
    WebRtcSpl_ZerosArrayW16(inst->histLrt, HIST_PAR_EST);
    WebRtcSpl_ZerosArrayW16(inst->histSpecDiff, HIST_PAR_EST);
    WebRtcSpl_ZerosArrayW16(inst->histSpecFlat, HIST_PAR_EST);
  }  
}





void WebRtcNsx_ComputeSpectralFlatness(NsxInst_t* inst, uint16_t* magn) {
  uint32_t tmpU32;
  uint32_t avgSpectralFlatnessNum, avgSpectralFlatnessDen;

  int32_t tmp32;
  int32_t currentSpectralFlatness, logCurSpectralFlatness;

  int16_t zeros, frac, intPart;

  int i;

  
  avgSpectralFlatnessNum = 0;
  avgSpectralFlatnessDen = inst->sumMagn - (uint32_t)magn[0]; 

  
  
  
  
  for (i = 1; i < inst->magnLen; i++) {
    
    if (magn[i]) {
      zeros = WebRtcSpl_NormU32((uint32_t)magn[i]);
      frac = (int16_t)(((uint32_t)((uint32_t)(magn[i]) << zeros)
                              & 0x7FFFFFFF) >> 23);
      
      assert(frac < 256);
      tmpU32 = (uint32_t)(((31 - zeros) << 8)
                                + WebRtcNsx_kLogTableFrac[frac]); 
      avgSpectralFlatnessNum += tmpU32; 
    } else {
      
      tmpU32 = WEBRTC_SPL_UMUL_32_16(inst->featureSpecFlat, SPECT_FLAT_TAVG_Q14); 
      inst->featureSpecFlat -= WEBRTC_SPL_RSHIFT_U32(tmpU32, 14); 
      return;
    }
  }
  
  zeros = WebRtcSpl_NormU32(avgSpectralFlatnessDen);
  frac = (int16_t)(((avgSpectralFlatnessDen << zeros) & 0x7FFFFFFF) >> 23);
  
  assert(frac < 256);
  tmp32 = (int32_t)(((31 - zeros) << 8) + WebRtcNsx_kLogTableFrac[frac]); 
  logCurSpectralFlatness = (int32_t)avgSpectralFlatnessNum;
  logCurSpectralFlatness += ((int32_t)(inst->stages - 1) << (inst->stages + 7)); 
  logCurSpectralFlatness -= (tmp32 << (inst->stages - 1));
  logCurSpectralFlatness = WEBRTC_SPL_LSHIFT_W32(logCurSpectralFlatness, 10 - inst->stages); 
  tmp32 = (int32_t)(0x00020000 | (WEBRTC_SPL_ABS_W32(logCurSpectralFlatness)
                                        & 0x0001FFFF)); 
  intPart = -(int16_t)WEBRTC_SPL_RSHIFT_W32(logCurSpectralFlatness, 17);
  intPart += 7; 
  if (intPart > 0) {
    currentSpectralFlatness = WEBRTC_SPL_RSHIFT_W32(tmp32, intPart);
  } else {
    currentSpectralFlatness = WEBRTC_SPL_LSHIFT_W32(tmp32, -intPart);
  }

  
  tmp32 = currentSpectralFlatness - (int32_t)inst->featureSpecFlat; 
  tmp32 = WEBRTC_SPL_MUL_32_16(SPECT_FLAT_TAVG_Q14, tmp32); 
  inst->featureSpecFlat = (uint32_t)((int32_t)inst->featureSpecFlat
                                           + WEBRTC_SPL_RSHIFT_W32(tmp32, 14)); 
  
}






void WebRtcNsx_ComputeSpectralDifference(NsxInst_t* inst, uint16_t* magnIn) {
  
  

  uint32_t tmpU32no1, tmpU32no2;
  uint32_t varMagnUFX, varPauseUFX, avgDiffNormMagnUFX;

  int32_t tmp32no1, tmp32no2;
  int32_t avgPauseFX, avgMagnFX, covMagnPauseFX;
  int32_t maxPause, minPause;

  int16_t tmp16no1;

  int i, norm32, nShifts;

  avgPauseFX = 0;
  maxPause = 0;
  minPause = inst->avgMagnPause[0]; 
  
  for (i = 0; i < inst->magnLen; i++) {
    
    avgPauseFX += inst->avgMagnPause[i]; 
    maxPause = WEBRTC_SPL_MAX(maxPause, inst->avgMagnPause[i]);
    minPause = WEBRTC_SPL_MIN(minPause, inst->avgMagnPause[i]);
  }
  
  avgPauseFX = WEBRTC_SPL_RSHIFT_W32(avgPauseFX, inst->stages - 1);
  avgMagnFX = (int32_t)WEBRTC_SPL_RSHIFT_U32(inst->sumMagn, inst->stages - 1);
  
  tmp32no1 = WEBRTC_SPL_MAX(maxPause - avgPauseFX, avgPauseFX - minPause);
  
  nShifts = WEBRTC_SPL_MAX(0, 10 + inst->stages - WebRtcSpl_NormW32(tmp32no1));

  varMagnUFX = 0;
  varPauseUFX = 0;
  covMagnPauseFX = 0;
  for (i = 0; i < inst->magnLen; i++) {
    
    tmp16no1 = (int16_t)((int32_t)magnIn[i] - avgMagnFX);
    tmp32no2 = inst->avgMagnPause[i] - avgPauseFX;
    varMagnUFX += (uint32_t)WEBRTC_SPL_MUL_16_16(tmp16no1, tmp16no1); 
    tmp32no1 = WEBRTC_SPL_MUL_32_16(tmp32no2, tmp16no1); 
    covMagnPauseFX += tmp32no1; 
    tmp32no1 = WEBRTC_SPL_RSHIFT_W32(tmp32no2, nShifts); 
    varPauseUFX += (uint32_t)WEBRTC_SPL_MUL(tmp32no1, tmp32no1); 
  }
  
  inst->curAvgMagnEnergy += WEBRTC_SPL_RSHIFT_U32(inst->magnEnergy, 2 * inst->normData
                                                  + inst->stages - 1);

  avgDiffNormMagnUFX = varMagnUFX; 
  if ((varPauseUFX) && (covMagnPauseFX)) {
    tmpU32no1 = (uint32_t)WEBRTC_SPL_ABS_W32(covMagnPauseFX); 
    norm32 = WebRtcSpl_NormU32(tmpU32no1) - 16;
    if (norm32 > 0) {
      tmpU32no1 = WEBRTC_SPL_LSHIFT_U32(tmpU32no1, norm32); 
    } else {
      tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no1, -norm32); 
    }
    tmpU32no2 = WEBRTC_SPL_UMUL(tmpU32no1, tmpU32no1); 

    nShifts += norm32;
    nShifts <<= 1;
    if (nShifts < 0) {
      varPauseUFX >>= (-nShifts); 
      nShifts = 0;
    }
    if (varPauseUFX > 0) {
      
      tmpU32no1 = WEBRTC_SPL_UDIV(tmpU32no2, varPauseUFX);
      tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no1, nShifts);

      
      avgDiffNormMagnUFX -= WEBRTC_SPL_MIN(avgDiffNormMagnUFX, tmpU32no1);
    } else {
      avgDiffNormMagnUFX = 0;
    }
  }
  
  tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(avgDiffNormMagnUFX, 2 * inst->normData);
  if (inst->featureSpecDiff > tmpU32no1) {
    tmpU32no2 = WEBRTC_SPL_UMUL_32_16(inst->featureSpecDiff - tmpU32no1,
                                      SPECT_DIFF_TAVG_Q8); 
    inst->featureSpecDiff -= WEBRTC_SPL_RSHIFT_U32(tmpU32no2, 8); 
  } else {
    tmpU32no2 = WEBRTC_SPL_UMUL_32_16(tmpU32no1 - inst->featureSpecDiff,
                                      SPECT_DIFF_TAVG_Q8); 
    inst->featureSpecDiff += WEBRTC_SPL_RSHIFT_U32(tmpU32no2, 8); 
  }
}


void WebRtcNsx_DataAnalysis(NsxInst_t* inst, short* speechFrame, uint16_t* magnU16) {

  uint32_t tmpU32no1, tmpU32no2;

  int32_t   tmp_1_w32 = 0;
  int32_t   tmp_2_w32 = 0;
  int32_t   sum_log_magn = 0;
  int32_t   sum_log_i_log_magn = 0;

  uint16_t  sum_log_magn_u16 = 0;
  uint16_t  tmp_u16 = 0;

  int16_t   sum_log_i = 0;
  int16_t   sum_log_i_square = 0;
  int16_t   frac = 0;
  int16_t   log2 = 0;
  int16_t   matrix_determinant = 0;
  int16_t   maxWinData;

  int i, j;
  int zeros;
  int net_norm = 0;
  int right_shifts_in_magnU16 = 0;
  int right_shifts_in_initMagnEst = 0;

  int16_t winData_buff[ANAL_BLOCKL_MAX * 2 + 16];
  int16_t realImag_buff[ANAL_BLOCKL_MAX * 2 + 16];

  
  int16_t* winData = (int16_t*) (((uintptr_t)winData_buff + 31) & ~31);
  int16_t* realImag = (int16_t*) (((uintptr_t) realImag_buff + 31) & ~31);

  
  WebRtcNsx_AnalysisUpdate(inst, winData, speechFrame);

  
  inst->energyIn = WebRtcSpl_Energy(winData, (int)inst->anaLen, &(inst->scaleEnergyIn));

  
  inst->zeroInputSignal = 0;
  
  maxWinData = WebRtcSpl_MaxAbsValueW16(winData, inst->anaLen);
  inst->normData = WebRtcSpl_NormW16(maxWinData);
  if (maxWinData == 0) {
    
    inst->zeroInputSignal = 1;
    return;
  }

  
  net_norm = inst->stages - inst->normData;
  
  right_shifts_in_magnU16 = inst->normData - inst->minNorm;
  right_shifts_in_initMagnEst = WEBRTC_SPL_MAX(-right_shifts_in_magnU16, 0);
  inst->minNorm -= right_shifts_in_initMagnEst;
  right_shifts_in_magnU16 = WEBRTC_SPL_MAX(right_shifts_in_magnU16, 0);

  
  WebRtcNsx_NormalizeRealBuffer(inst, winData, realImag);

  
  WebRtcSpl_RealForwardFFT(inst->real_fft, realImag, winData);

  inst->imag[0] = 0; 
  inst->imag[inst->anaLen2] = 0;
  inst->real[0] = winData[0]; 
  inst->real[inst->anaLen2] = winData[inst->anaLen];
  
  inst->magnEnergy = (uint32_t)WEBRTC_SPL_MUL_16_16(inst->real[0], inst->real[0]);
  inst->magnEnergy += (uint32_t)WEBRTC_SPL_MUL_16_16(inst->real[inst->anaLen2],
                                                           inst->real[inst->anaLen2]);
  magnU16[0] = (uint16_t)WEBRTC_SPL_ABS_W16(inst->real[0]); 
  magnU16[inst->anaLen2] = (uint16_t)WEBRTC_SPL_ABS_W16(inst->real[inst->anaLen2]);
  inst->sumMagn = (uint32_t)magnU16[0]; 
  inst->sumMagn += (uint32_t)magnU16[inst->anaLen2];

  if (inst->blockIndex >= END_STARTUP_SHORT) {
    for (i = 1, j = 2; i < inst->anaLen2; i += 1, j += 2) {
      inst->real[i] = winData[j];
      inst->imag[i] = -winData[j + 1];
      
      
      tmpU32no1 = (uint32_t)WEBRTC_SPL_MUL_16_16(winData[j], winData[j]);
      tmpU32no1 += (uint32_t)WEBRTC_SPL_MUL_16_16(winData[j + 1], winData[j + 1]);
      inst->magnEnergy += tmpU32no1; 

      magnU16[i] = (uint16_t)WebRtcSpl_SqrtFloor(tmpU32no1); 
      inst->sumMagn += (uint32_t)magnU16[i]; 
    }
  } else {
    
    
    

    
    inst->initMagnEst[0] = WEBRTC_SPL_RSHIFT_U32(inst->initMagnEst[0],
                                                 right_shifts_in_initMagnEst);
    inst->initMagnEst[inst->anaLen2] =
      WEBRTC_SPL_RSHIFT_U32(inst->initMagnEst[inst->anaLen2],
                            right_shifts_in_initMagnEst); 

    
    tmpU32no1 = WEBRTC_SPL_RSHIFT_W32((uint32_t)magnU16[0],
                                      right_shifts_in_magnU16); 
    tmpU32no2 = WEBRTC_SPL_RSHIFT_W32((uint32_t)magnU16[inst->anaLen2],
                                      right_shifts_in_magnU16); 

    
    inst->initMagnEst[0] += tmpU32no1; 
    inst->initMagnEst[inst->anaLen2] += tmpU32no2; 

    log2 = 0;
    if (magnU16[inst->anaLen2]) {
      
      zeros = WebRtcSpl_NormU32((uint32_t)magnU16[inst->anaLen2]);
      frac = (int16_t)((((uint32_t)magnU16[inst->anaLen2] << zeros) &
                              0x7FFFFFFF) >> 23); 
      
      assert(frac < 256);
      log2 = (int16_t)(((31 - zeros) << 8) + WebRtcNsx_kLogTableFrac[frac]);
    }

    sum_log_magn = (int32_t)log2; 
    
    sum_log_i_log_magn = (WEBRTC_SPL_MUL_16_16(kLogIndex[inst->anaLen2], log2) >> 3);

    for (i = 1, j = 2; i < inst->anaLen2; i += 1, j += 2) {
      inst->real[i] = winData[j];
      inst->imag[i] = -winData[j + 1];
      
      
      tmpU32no1 = (uint32_t)WEBRTC_SPL_MUL_16_16(winData[j], winData[j]);
      tmpU32no1 += (uint32_t)WEBRTC_SPL_MUL_16_16(winData[j + 1], winData[j + 1]);
      inst->magnEnergy += tmpU32no1; 

      magnU16[i] = (uint16_t)WebRtcSpl_SqrtFloor(tmpU32no1); 
      inst->sumMagn += (uint32_t)magnU16[i]; 

      
      inst->initMagnEst[i] = WEBRTC_SPL_RSHIFT_U32(inst->initMagnEst[i],
                                                   right_shifts_in_initMagnEst);

      
      tmpU32no1 = WEBRTC_SPL_RSHIFT_W32((uint32_t)magnU16[i],
                                        right_shifts_in_magnU16);
      
      inst->initMagnEst[i] += tmpU32no1; 

      if (i >= kStartBand) {
        
        log2 = 0;
        if (magnU16[i]) {
          zeros = WebRtcSpl_NormU32((uint32_t)magnU16[i]);
          frac = (int16_t)((((uint32_t)magnU16[i] << zeros) &
                                  0x7FFFFFFF) >> 23);
          
          assert(frac < 256);
          log2 = (int16_t)(((31 - zeros) << 8)
                                 + WebRtcNsx_kLogTableFrac[frac]);
        }
        sum_log_magn += (int32_t)log2; 
        
        sum_log_i_log_magn += (WEBRTC_SPL_MUL_16_16(kLogIndex[i], log2) >> 3);
      }
    }

    
    
    

    

    
    inst->whiteNoiseLevel = WEBRTC_SPL_RSHIFT_U32(inst->whiteNoiseLevel,
                                                  right_shifts_in_initMagnEst);

    
    tmpU32no1 = WEBRTC_SPL_UMUL_32_16(inst->sumMagn, inst->overdrive);
    tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no1, inst->stages + 8);

    
    
    tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no1, right_shifts_in_magnU16);
    
    assert(END_STARTUP_SHORT < 128);
    inst->whiteNoiseLevel += tmpU32no1; 

    
    
    
    
    assert(kStartBand < 66);
    matrix_determinant = kDeterminantEstMatrix[kStartBand]; 
    sum_log_i = kSumLogIndex[kStartBand]; 
    sum_log_i_square = kSumSquareLogIndex[kStartBand]; 
    if (inst->fs == 8000) {
      
      tmp_1_w32 = (int32_t)matrix_determinant;
      tmp_1_w32 += WEBRTC_SPL_MUL_16_16_RSFT(kSumLogIndex[65], sum_log_i, 9);
      tmp_1_w32 -= WEBRTC_SPL_MUL_16_16_RSFT(kSumLogIndex[65], kSumLogIndex[65], 10);
      tmp_1_w32 -= WEBRTC_SPL_LSHIFT_W32((int32_t)sum_log_i_square, 4);
      tmp_1_w32 -= WEBRTC_SPL_MUL_16_16_RSFT((int16_t)
                       (inst->magnLen - kStartBand), kSumSquareLogIndex[65], 2);
      matrix_determinant = (int16_t)tmp_1_w32;
      sum_log_i -= kSumLogIndex[65]; 
      sum_log_i_square -= kSumSquareLogIndex[65]; 
    }

    
    zeros = 16 - WebRtcSpl_NormW32(sum_log_magn);
    if (zeros < 0) {
      zeros = 0;
    }
    tmp_1_w32 = WEBRTC_SPL_LSHIFT_W32(sum_log_magn, 1); 
    sum_log_magn_u16 = (uint16_t)WEBRTC_SPL_RSHIFT_W32(tmp_1_w32, zeros);

    
    tmp_2_w32 = WEBRTC_SPL_MUL_16_U16(sum_log_i_square, sum_log_magn_u16); 
    tmpU32no1 = WEBRTC_SPL_RSHIFT_U32((uint32_t)sum_log_i_log_magn, 12); 

    
    tmp_u16 = WEBRTC_SPL_LSHIFT_U16((uint16_t)sum_log_i, 1); 
    if ((uint32_t)sum_log_i > tmpU32no1) {
      tmp_u16 = WEBRTC_SPL_RSHIFT_U16(tmp_u16, zeros);
    } else {
      tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no1, zeros);
    }
    tmp_2_w32 -= (int32_t)WEBRTC_SPL_UMUL_32_16(tmpU32no1, tmp_u16); 
    matrix_determinant = WEBRTC_SPL_RSHIFT_W16(matrix_determinant, zeros); 
    tmp_2_w32 = WebRtcSpl_DivW32W16(tmp_2_w32, matrix_determinant); 
    tmp_2_w32 += WEBRTC_SPL_LSHIFT_W32((int32_t)net_norm, 11); 
    if (tmp_2_w32 < 0) {
      tmp_2_w32 = 0;
    }
    inst->pinkNoiseNumerator += tmp_2_w32; 

    
    tmp_2_w32 = WEBRTC_SPL_MUL_16_U16(sum_log_i, sum_log_magn_u16); 
    tmp_1_w32 = WEBRTC_SPL_RSHIFT_W32(sum_log_i_log_magn, 3 + zeros);
    tmp_1_w32 = WEBRTC_SPL_MUL((int32_t)(inst->magnLen - kStartBand),
                               tmp_1_w32);
    tmp_2_w32 -= tmp_1_w32; 
    if (tmp_2_w32 > 0) {
      
      
      tmp_1_w32 = WebRtcSpl_DivW32W16(tmp_2_w32, matrix_determinant); 
      inst->pinkNoiseExp += WEBRTC_SPL_SAT(16384, tmp_1_w32, 0); 
    }
  }
}

void WebRtcNsx_DataSynthesis(NsxInst_t* inst, short* outFrame) {
  int32_t energyOut;

  int16_t realImag_buff[ANAL_BLOCKL_MAX * 2 + 16];
  int16_t rfft_out_buff[ANAL_BLOCKL_MAX * 2 + 16];

  
  int16_t* realImag = (int16_t*) (((uintptr_t)realImag_buff + 31) & ~31);
  int16_t* rfft_out = (int16_t*) (((uintptr_t) rfft_out_buff + 31) & ~31);

  int16_t tmp16no1, tmp16no2;
  int16_t energyRatio;
  int16_t gainFactor, gainFactor1, gainFactor2;

  int i;
  int outCIFFT;
  int scaleEnergyOut = 0;

  if (inst->zeroInputSignal) {
    
    
    for (i = 0; i < inst->blockLen10ms; i++) {
      outFrame[i] = inst->synthesisBuffer[i]; 
    }
    
    WEBRTC_SPL_MEMCPY_W16(inst->synthesisBuffer,
                          inst->synthesisBuffer + inst->blockLen10ms,
                          inst->anaLen - inst->blockLen10ms);
    WebRtcSpl_ZerosArrayW16(inst->synthesisBuffer + inst->anaLen - inst->blockLen10ms,
                            inst->blockLen10ms);
    return;
  }

  
  WebRtcNsx_PrepareSpectrum(inst, realImag);

  
  outCIFFT = WebRtcSpl_RealInverseFFT(inst->real_fft, realImag, rfft_out);

  WebRtcNsx_Denormalize(inst, rfft_out, outCIFFT);

  
  gainFactor = 8192; 
  if (inst->gainMap == 1 &&
      inst->blockIndex > END_STARTUP_LONG &&
      inst->energyIn > 0) {
    energyOut = WebRtcSpl_Energy(inst->real, (int)inst->anaLen, &scaleEnergyOut); 
    if (scaleEnergyOut == 0 && !(energyOut & 0x7f800000)) {
      energyOut = WEBRTC_SPL_SHIFT_W32(energyOut, 8 + scaleEnergyOut
                                       - inst->scaleEnergyIn);
    } else {
      inst->energyIn = WEBRTC_SPL_RSHIFT_W32(inst->energyIn, 8 + scaleEnergyOut
                                             - inst->scaleEnergyIn); 
    }

    assert(inst->energyIn > 0);
    energyRatio = (int16_t)WEBRTC_SPL_DIV(energyOut
        + WEBRTC_SPL_RSHIFT_W32(inst->energyIn, 1), inst->energyIn); 
    
    energyRatio = WEBRTC_SPL_SAT(256, energyRatio, 0);

    
    assert(energyRatio < 257);
    gainFactor1 = kFactor1Table[energyRatio]; 
    gainFactor2 = inst->factor2Table[energyRatio]; 

    

    
    tmp16no1 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(16384 - inst->priorNonSpeechProb,
                                                        gainFactor1, 14); 
    tmp16no2 = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(inst->priorNonSpeechProb,
                                                        gainFactor2, 14); 
    gainFactor = tmp16no1 + tmp16no2; 
  }  

  
  WebRtcNsx_SynthesisUpdate(inst, outFrame, gainFactor);
}

int WebRtcNsx_ProcessCore(NsxInst_t* inst, short* speechFrame, short* speechFrameHB,
                          short* outFrame, short* outFrameHB) {
  

  uint32_t tmpU32no1, tmpU32no2, tmpU32no3;
  uint32_t satMax, maxNoiseU32;
  uint32_t tmpMagnU32, tmpNoiseU32;
  uint32_t nearMagnEst;
  uint32_t noiseUpdateU32;
  uint32_t noiseU32[HALF_ANAL_BLOCKL];
  uint32_t postLocSnr[HALF_ANAL_BLOCKL];
  uint32_t priorLocSnr[HALF_ANAL_BLOCKL];
  uint32_t prevNearSnr[HALF_ANAL_BLOCKL];
  uint32_t curNearSnr;
  uint32_t priorSnr;
  uint32_t noise_estimate = 0;
  uint32_t noise_estimate_avg = 0;
  uint32_t numerator = 0;

  int32_t tmp32no1, tmp32no2;
  int32_t pink_noise_num_avg = 0;

  uint16_t tmpU16no1;
  uint16_t magnU16[HALF_ANAL_BLOCKL];
  uint16_t prevNoiseU16[HALF_ANAL_BLOCKL];
  uint16_t nonSpeechProbFinal[HALF_ANAL_BLOCKL];
  uint16_t gammaNoise, prevGammaNoise;
  uint16_t noiseSupFilterTmp[HALF_ANAL_BLOCKL];

  int16_t qMagn, qNoise;
  int16_t avgProbSpeechHB, gainModHB, avgFilterGainHB, gainTimeDomainHB;
  int16_t pink_noise_exp_avg = 0;

  int i;
  int nShifts, postShifts;
  int norm32no1, norm32no2;
  int flag, sign;
  int q_domain_to_use = 0;

  
  assert(inst->anaLen > 0);
  assert(inst->anaLen2 > 0);
  assert(inst->anaLen % 16 == 0);
  assert(inst->anaLen2 % 8 == 0);
  assert(inst->blockLen10ms > 0);
  assert(inst->blockLen10ms % 16 == 0);
  assert(inst->magnLen == inst->anaLen2 + 1);

#ifdef NS_FILEDEBUG
  if (fwrite(spframe, sizeof(short),
             inst->blockLen10ms, inst->infile) != inst->blockLen10ms) {
    return -1;
  }
#endif

  
  if (inst->initFlag != 1) {
    return -1;
  }
  
  if ((inst->fs == 32000) && (speechFrameHB == NULL)) {
    return -1;
  }

  
  WebRtcNsx_DataAnalysis(inst, speechFrame, magnU16);

  if (inst->zeroInputSignal) {
    WebRtcNsx_DataSynthesis(inst, outFrame);

    if (inst->fs == 32000) {
      
      
      WEBRTC_SPL_MEMCPY_W16(inst->dataBufHBFX, inst->dataBufHBFX + inst->blockLen10ms,
                            inst->anaLen - inst->blockLen10ms);
      WEBRTC_SPL_MEMCPY_W16(inst->dataBufHBFX + inst->anaLen - inst->blockLen10ms,
                            speechFrameHB, inst->blockLen10ms);
      for (i = 0; i < inst->blockLen10ms; i++) {
        outFrameHB[i] = inst->dataBufHBFX[i]; 
      }
    }  
    return 0;
  }

  
  inst->blockIndex++;
  

  
  qMagn = inst->normData - inst->stages;

  
  WebRtcNsx_ComputeSpectralFlatness(inst, magnU16);

  
  WebRtcNsx_NoiseEstimation(inst, magnU16, noiseU32, &qNoise);

  
  for (i = 0; i < inst->magnLen; i++) {
    prevNoiseU16[i] = (uint16_t)WEBRTC_SPL_RSHIFT_U32(inst->prevNoiseU32[i], 11); 
  }

  if (inst->blockIndex < END_STARTUP_SHORT) {
    
    q_domain_to_use = WEBRTC_SPL_MIN((int)qNoise, inst->minNorm - inst->stages);

    
    
    if (inst->pinkNoiseExp) {
      pink_noise_exp_avg = (int16_t)WebRtcSpl_DivW32W16(inst->pinkNoiseExp,
                                                              (int16_t)(inst->blockIndex + 1)); 
      pink_noise_num_avg = WebRtcSpl_DivW32W16(inst->pinkNoiseNumerator,
                                               (int16_t)(inst->blockIndex + 1)); 
      WebRtcNsx_CalcParametricNoiseEstimate(inst,
                                            pink_noise_exp_avg,
                                            pink_noise_num_avg,
                                            kStartBand,
                                            &noise_estimate,
                                            &noise_estimate_avg);
    } else {
      
      noise_estimate = inst->whiteNoiseLevel; 
      noise_estimate_avg = noise_estimate / (inst->blockIndex + 1); 
    }
    for (i = 0; i < inst->magnLen; i++) {
      
      if ((inst->pinkNoiseExp) && (i >= kStartBand)) {
        
        noise_estimate = 0;
        noise_estimate_avg = 0;
        
        WebRtcNsx_CalcParametricNoiseEstimate(inst,
                                              pink_noise_exp_avg,
                                              pink_noise_num_avg,
                                              i,
                                              &noise_estimate,
                                              &noise_estimate_avg);
      }
      
      noiseSupFilterTmp[i] = inst->denoiseBound;
      if (inst->initMagnEst[i]) {
        
        
        tmpU32no1 = WEBRTC_SPL_UMUL_32_16(noise_estimate, inst->overdrive);
        numerator = WEBRTC_SPL_LSHIFT_U32(inst->initMagnEst[i], 8);
        if (numerator > tmpU32no1) {
          
          numerator -= tmpU32no1;

          
          
          nShifts = WebRtcSpl_NormU32(numerator);
          nShifts = WEBRTC_SPL_SAT(6, nShifts, 0);

          
          numerator = WEBRTC_SPL_LSHIFT_U32(numerator, nShifts);

          
          tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(inst->initMagnEst[i], 6 - nShifts);
          if (tmpU32no1 == 0) {
            
            
            tmpU32no1 = 1;
          }
          tmpU32no2 = WEBRTC_SPL_UDIV(numerator, tmpU32no1); 
          noiseSupFilterTmp[i] = (uint16_t)WEBRTC_SPL_SAT(16384, tmpU32no2,
              (uint32_t)(inst->denoiseBound)); 
        }
      }
      
      
      
      
      
      

      
      tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(noiseU32[i], (int)qNoise - q_domain_to_use);
      
      tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(noise_estimate_avg, inst->minNorm - inst->stages
                                        - q_domain_to_use);
      
      
      nShifts = 0;
      if (tmpU32no1 & 0xfc000000) {
        tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no1, 6);
        tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(tmpU32no2, 6);
        nShifts = 6;
      }
      tmpU32no1 *= inst->blockIndex;
      tmpU32no2 *= (END_STARTUP_SHORT - inst->blockIndex);
      
      noiseU32[i] = WebRtcSpl_DivU32U16(tmpU32no1 + tmpU32no2, END_STARTUP_SHORT);
      
      noiseU32[i] = WEBRTC_SPL_LSHIFT_U32(noiseU32[i], nShifts);
    }
    
    qNoise = q_domain_to_use;
  }
  
  
  if (inst->blockIndex < END_STARTUP_LONG) {
    
    inst->timeAvgMagnEnergyTmp
    += WEBRTC_SPL_RSHIFT_U32(inst->magnEnergy,
                             2 * inst->normData + inst->stages - 1);
    inst->timeAvgMagnEnergy = WebRtcSpl_DivU32U16(inst->timeAvgMagnEnergyTmp,
                                                  inst->blockIndex + 1);
  }

  
  

  
  satMax = (uint32_t)1048575;
  postShifts = 6 + qMagn - qNoise;
  nShifts = 5 - inst->prevQMagn + inst->prevQNoise;
  for (i = 0; i < inst->magnLen; i++) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    postLocSnr[i] = 2048; 
    tmpU32no1 = WEBRTC_SPL_LSHIFT_U32((uint32_t)magnU16[i], 6); 
    if (postShifts < 0) {
      tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(noiseU32[i], -postShifts); 
    } else {
      tmpU32no2 = WEBRTC_SPL_LSHIFT_U32(noiseU32[i], postShifts); 
    }
    if (tmpU32no1 > tmpU32no2) {
      
      tmpU32no1 = WEBRTC_SPL_LSHIFT_U32(tmpU32no1, 11); 
      if (tmpU32no2 > 0) {
        tmpU32no1 = WEBRTC_SPL_UDIV(tmpU32no1, tmpU32no2); 
        postLocSnr[i] = WEBRTC_SPL_MIN(satMax, tmpU32no1); 
      } else {
        postLocSnr[i] = satMax;
      }
    }

    
    nearMagnEst = WEBRTC_SPL_UMUL_16_16(inst->prevMagnU16[i], inst->noiseSupFilter[i]); 
    tmpU32no1 = WEBRTC_SPL_LSHIFT_U32(nearMagnEst, 3); 
    tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(inst->prevNoiseU32[i], nShifts); 

    if (tmpU32no2 > 0) {
      tmpU32no1 = WEBRTC_SPL_UDIV(tmpU32no1, tmpU32no2); 
      tmpU32no1 = WEBRTC_SPL_MIN(satMax, tmpU32no1); 
    } else {
      tmpU32no1 = satMax; 
    }
    prevNearSnr[i] = tmpU32no1; 

    
    tmpU32no1 = WEBRTC_SPL_UMUL_32_16(prevNearSnr[i], DD_PR_SNR_Q11); 
    tmpU32no2 = WEBRTC_SPL_UMUL_32_16(postLocSnr[i] - 2048, ONE_MINUS_DD_PR_SNR_Q11); 
    priorSnr = tmpU32no1 + tmpU32no2 + 512; 
    
    priorLocSnr[i] = 2048 + WEBRTC_SPL_RSHIFT_U32(priorSnr, 10); 
  }  
  

  

  
  WebRtcNsx_ComputeSpectralDifference(inst, magnU16);
  
  
  
  inst->cntThresUpdate++;
  flag = (int)(inst->cntThresUpdate == inst->modelUpdate);
  
  WebRtcNsx_FeatureParameterExtraction(inst, flag);
  
  if (flag) {
    inst->cntThresUpdate = 0; 
    
    

    
    inst->curAvgMagnEnergy = WEBRTC_SPL_RSHIFT_U32(inst->curAvgMagnEnergy, STAT_UPDATES);

    tmpU32no1 = (inst->curAvgMagnEnergy + inst->timeAvgMagnEnergy + 1) >> 1; 
    
    if ((tmpU32no1 != inst->timeAvgMagnEnergy) && (inst->featureSpecDiff) &&
        (inst->timeAvgMagnEnergy > 0)) {
      norm32no1 = 0;
      tmpU32no3 = tmpU32no1;
      while (0xFFFF0000 & tmpU32no3) {
        tmpU32no3 >>= 1;
        norm32no1++;
      }
      tmpU32no2 = inst->featureSpecDiff;
      while (0xFFFF0000 & tmpU32no2) {
        tmpU32no2 >>= 1;
        norm32no1++;
      }
      tmpU32no3 = WEBRTC_SPL_UMUL(tmpU32no3, tmpU32no2);
      tmpU32no3 = WEBRTC_SPL_UDIV(tmpU32no3, inst->timeAvgMagnEnergy);
      if (WebRtcSpl_NormU32(tmpU32no3) < norm32no1) {
        inst->featureSpecDiff = 0x007FFFFF;
      } else {
        inst->featureSpecDiff = WEBRTC_SPL_MIN(0x007FFFFF,
            WEBRTC_SPL_LSHIFT_U32(tmpU32no3, norm32no1));
      }
    }

    inst->timeAvgMagnEnergy = tmpU32no1; 
    inst->curAvgMagnEnergy = 0;
  }

  
  WebRtcNsx_SpeechNoiseProb(inst, nonSpeechProbFinal, priorLocSnr, postLocSnr);

  
  gammaNoise = NOISE_UPDATE_Q8; 

  maxNoiseU32 = 0;
  postShifts = inst->prevQNoise - qMagn;
  nShifts = inst->prevQMagn - qMagn;
  for (i = 0; i < inst->magnLen; i++) {
    
    
    

    if (postShifts < 0) {
      tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(magnU16[i], -postShifts); 
    } else {
      tmpU32no2 = WEBRTC_SPL_LSHIFT_U32(magnU16[i], postShifts); 
    }
    if (prevNoiseU16[i] > tmpU32no2) {
      sign = -1;
      tmpU32no1 = prevNoiseU16[i] - tmpU32no2;
    } else {
      sign = 1;
      tmpU32no1 = tmpU32no2 - prevNoiseU16[i];
    }
    noiseUpdateU32 = inst->prevNoiseU32[i]; 
    tmpU32no3 = 0;
    if ((tmpU32no1) && (nonSpeechProbFinal[i])) {
      
      tmpU32no3 = WEBRTC_SPL_UMUL_32_16(tmpU32no1, nonSpeechProbFinal[i]); 
      if (0x7c000000 & tmpU32no3) {
        
        tmpU32no2
          = WEBRTC_SPL_UMUL_32_16(WEBRTC_SPL_RSHIFT_U32(tmpU32no3, 5), gammaNoise); 
      } else {
        
        tmpU32no2
          = WEBRTC_SPL_RSHIFT_U32(WEBRTC_SPL_UMUL_32_16(tmpU32no3, gammaNoise), 5); 
      }
      if (sign > 0) {
        noiseUpdateU32 += tmpU32no2; 
      } else {
        
        
        noiseUpdateU32 -= tmpU32no2; 
      }
    }

    
    prevGammaNoise = gammaNoise;
    gammaNoise = NOISE_UPDATE_Q8;
    
    
    if (nonSpeechProbFinal[i] < ONE_MINUS_PROB_RANGE_Q8) {
      gammaNoise = GAMMA_NOISE_TRANS_AND_SPEECH_Q8;
    }

    if (prevGammaNoise != gammaNoise) {
      
      
      
      
      

      if (0x7c000000 & tmpU32no3) {
        
        tmpU32no2
          = WEBRTC_SPL_UMUL_32_16(WEBRTC_SPL_RSHIFT_U32(tmpU32no3, 5), gammaNoise); 
      } else {
        
        tmpU32no2
          = WEBRTC_SPL_RSHIFT_U32(WEBRTC_SPL_UMUL_32_16(tmpU32no3, gammaNoise), 5); 
      }
      if (sign > 0) {
        tmpU32no1 = inst->prevNoiseU32[i] + tmpU32no2; 
      } else {
        tmpU32no1 = inst->prevNoiseU32[i] - tmpU32no2; 
      }
      if (noiseUpdateU32 > tmpU32no1) {
        noiseUpdateU32 = tmpU32no1; 
      }
    }
    noiseU32[i] = noiseUpdateU32; 
    if (noiseUpdateU32 > maxNoiseU32) {
      maxNoiseU32 = noiseUpdateU32;
    }

    
    
    
    
    

    tmp32no2 = WEBRTC_SPL_SHIFT_W32(inst->avgMagnPause[i], -nShifts);
    if (nonSpeechProbFinal[i] > ONE_MINUS_PROB_RANGE_Q8) {
      if (nShifts < 0) {
        tmp32no1 = (int32_t)magnU16[i] - tmp32no2; 
        tmp32no1 = WEBRTC_SPL_MUL_32_16(tmp32no1, ONE_MINUS_GAMMA_PAUSE_Q8); 
        tmp32no1 = WEBRTC_SPL_RSHIFT_W32(tmp32no1 + 128, 8); 
      } else {
        tmp32no1 = WEBRTC_SPL_LSHIFT_W32((int32_t)magnU16[i], nShifts)
                   - inst->avgMagnPause[i]; 
        tmp32no1 = WEBRTC_SPL_MUL_32_16(tmp32no1, ONE_MINUS_GAMMA_PAUSE_Q8); 
        tmp32no1 = WEBRTC_SPL_RSHIFT_W32(tmp32no1 + (128 << nShifts), 8 + nShifts); 
      }
      tmp32no2 += tmp32no1; 
    }
    inst->avgMagnPause[i] = tmp32no2;
  }  

  norm32no1 = WebRtcSpl_NormU32(maxNoiseU32);
  qNoise = inst->prevQNoise + norm32no1 - 5;
  

  
  nShifts = inst->prevQNoise + 11 - qMagn;
  for (i = 0; i < inst->magnLen; i++) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    curNearSnr = 0; 
    if (nShifts < 0) {
      
      tmpMagnU32 = (uint32_t)magnU16[i]; 
      tmpNoiseU32 = WEBRTC_SPL_LSHIFT_U32(noiseU32[i], -nShifts); 
    } else if (nShifts > 17) {
      tmpMagnU32 = WEBRTC_SPL_LSHIFT_U32(magnU16[i], 17); 
      tmpNoiseU32 = WEBRTC_SPL_RSHIFT_U32(noiseU32[i], nShifts - 17); 
    } else {
      tmpMagnU32 = WEBRTC_SPL_LSHIFT_U32((uint32_t)magnU16[i], nShifts); 
      tmpNoiseU32 = noiseU32[i]; 
    }
    if (tmpMagnU32 > tmpNoiseU32) {
      tmpU32no1 = tmpMagnU32 - tmpNoiseU32; 
      norm32no2 = WEBRTC_SPL_MIN(11, WebRtcSpl_NormU32(tmpU32no1));
      tmpU32no1 = WEBRTC_SPL_LSHIFT_U32(tmpU32no1, norm32no2); 
      tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(tmpNoiseU32, 11 - norm32no2); 
      if (tmpU32no2 > 0) {
        tmpU32no1 = WEBRTC_SPL_UDIV(tmpU32no1, tmpU32no2); 
      }
      curNearSnr = WEBRTC_SPL_MIN(satMax, tmpU32no1); 
    }

    
    
    

    tmpU32no1 = WEBRTC_SPL_UMUL_32_16(prevNearSnr[i], DD_PR_SNR_Q11); 
    tmpU32no2 = WEBRTC_SPL_UMUL_32_16(curNearSnr, ONE_MINUS_DD_PR_SNR_Q11); 
    priorSnr = tmpU32no1 + tmpU32no2; 

    
    tmpU32no1 = (uint32_t)(inst->overdrive)
                + WEBRTC_SPL_RSHIFT_U32(priorSnr + 8192, 14); 
    assert(inst->overdrive > 0);
    tmpU16no1 = (uint16_t)WEBRTC_SPL_UDIV(priorSnr + (tmpU32no1 >> 1), tmpU32no1); 
    inst->noiseSupFilter[i] = WEBRTC_SPL_SAT(16384, tmpU16no1, inst->denoiseBound); 

    
    if (inst->blockIndex < END_STARTUP_SHORT) {
      
      tmpU32no1 = WEBRTC_SPL_UMUL_16_16(inst->noiseSupFilter[i],
                                        (uint16_t)inst->blockIndex);
      tmpU32no2 = WEBRTC_SPL_UMUL_16_16(noiseSupFilterTmp[i],
                                        (uint16_t)(END_STARTUP_SHORT
                                                         - inst->blockIndex));
      tmpU32no1 += tmpU32no2;
      inst->noiseSupFilter[i] = (uint16_t)WebRtcSpl_DivU32U16(tmpU32no1,
                                                                    END_STARTUP_SHORT);
    }
  }  
  

  
  inst->prevQNoise = qNoise;
  inst->prevQMagn = qMagn;
  if (norm32no1 > 5) {
    for (i = 0; i < inst->magnLen; i++) {
      inst->prevNoiseU32[i] = WEBRTC_SPL_LSHIFT_U32(noiseU32[i], norm32no1 - 5); 
      inst->prevMagnU16[i] = magnU16[i]; 
    }
  } else {
    for (i = 0; i < inst->magnLen; i++) {
      inst->prevNoiseU32[i] = WEBRTC_SPL_RSHIFT_U32(noiseU32[i], 5 - norm32no1); 
      inst->prevMagnU16[i] = magnU16[i]; 
    }
  }

  WebRtcNsx_DataSynthesis(inst, outFrame);
#ifdef NS_FILEDEBUG
  if (fwrite(outframe, sizeof(short),
             inst->blockLen10ms, inst->outfile) != inst->blockLen10ms) {
    return -1;
  }
#endif

  
  
  if (inst->fs == 32000) {
    
    
    WEBRTC_SPL_MEMCPY_W16(inst->dataBufHBFX, inst->dataBufHBFX + inst->blockLen10ms, inst->anaLen - inst->blockLen10ms);
    WEBRTC_SPL_MEMCPY_W16(inst->dataBufHBFX + inst->anaLen - inst->blockLen10ms, speechFrameHB, inst->blockLen10ms);
    

    gainTimeDomainHB = 16384; 
    
    
    
    tmpU32no1 = 0; 
    tmpU16no1 = 0; 
    for (i = inst->anaLen2 - (inst->anaLen2 >> 2); i < inst->anaLen2; i++) {
      tmpU16no1 += nonSpeechProbFinal[i]; 
      tmpU32no1 += (uint32_t)(inst->noiseSupFilter[i]); 
    }
    avgProbSpeechHB = (int16_t)(4096
        - WEBRTC_SPL_RSHIFT_U16(tmpU16no1, inst->stages - 7)); 
    avgFilterGainHB = (int16_t)WEBRTC_SPL_RSHIFT_U32(
        tmpU32no1, inst->stages - 3); 

    
    
    
    

    
    
    
    
    
    
    
    
    gainModHB = WEBRTC_SPL_MIN(avgProbSpeechHB, 3607);

    
    
    
    
    
    
    
    


    
    if (avgProbSpeechHB < 2048) {
      
      
      gainTimeDomainHB = (gainModHB << 1) + (avgFilterGainHB >> 1); 
    } else {
      
      gainTimeDomainHB = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(3, avgFilterGainHB, 2); 
      gainTimeDomainHB += gainModHB; 
    }
    
    gainTimeDomainHB
      = WEBRTC_SPL_SAT(16384, gainTimeDomainHB, (int16_t)(inst->denoiseBound)); 


    
    for (i = 0; i < inst->blockLen10ms; i++) {
      outFrameHB[i]
        = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(gainTimeDomainHB, inst->dataBufHBFX[i], 14); 
    }
  }  

  return 0;
}
