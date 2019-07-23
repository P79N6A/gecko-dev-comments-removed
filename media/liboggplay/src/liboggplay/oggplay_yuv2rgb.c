









































#include "oggplay_private.h"
















#if 0 

#if defined(WIN32)
#define restrict
#include <emmintrin.h>
#else
#include <xmmintrin.h>
#ifndef restrict
#define restrict __restrict__
#endif
#endif


void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb) {

  int               i;
  unsigned char   * restrict ptry;
  unsigned char   * restrict ptru;
  unsigned char   * restrict ptrv;

  register __m64    *y, *o;
  register __m64    zero, ut, vt, imm, imm2;
  register __m64    r, g, b;
  register __m64    tmp, tmp2;

  zero = _mm_setzero_si64();

  ptry = yuv->ptry;
  ptru = yuv->ptru;
  ptrv = yuv->ptrv;

  for (i = 0; i < yuv->y_height; i++) {
    int j;
    o = (__m64*)rgb->ptro;
    rgb->ptro += rgb->rgb_width * 4;
    for (j = 0; j < yuv->y_width; j += 8) {

      y = (__m64*)&ptry[j];

      ut = _m_from_int(*(int *)(ptru + j/2));
      vt = _m_from_int(*(int *)(ptrv + j/2));

      
      

      ut = _m_punpcklbw(ut, zero);
      vt = _m_punpcklbw(vt, zero);

      
      imm = _mm_set1_pi16(128);
      ut = _m_psubw(ut, imm);
      vt = _m_psubw(vt, imm);

      
      imm = _mm_set1_pi16(-51);
      g = _m_pmullw(ut, imm);
      imm = _mm_set1_pi16(130);
      b = _m_pmullw(ut, imm);
      imm = _mm_set1_pi16(146);
      r = _m_pmullw(vt, imm);
      imm = _mm_set1_pi16(-74);
      imm = _m_pmullw(vt, imm);
      g = _m_paddsw(g, imm);

      
      imm = _mm_set1_pi16(64);
      r = _m_paddsw(r, imm);
      g = _m_paddsw(g, imm);
      imm = _mm_set1_pi16(32);
      b = _m_paddsw(b, imm);

      
      r = _m_psrawi(r, 7);
      g = _m_psrawi(g, 7);
      b = _m_psrawi(b, 6);

      
      imm = _mm_set1_pi16(16);
      r = _m_psubsw(r, imm);
      g = _m_psubsw(g, imm);
      b = _m_psubsw(b, imm);

      y = (__m64*)&ptry[j];

      






      tmp = _m_punpckhwd(r, r);
      imm = _m_punpckhbw(*y, zero);
      
      tmp = _m_paddsw(tmp, imm);
      tmp2 = _m_punpcklwd(r, r);
      imm2 = _m_punpcklbw(*y, zero);
      tmp2 = _m_paddsw(tmp2, imm2);
      r = _m_packuswb(tmp2, tmp);

      tmp = _m_punpckhwd(g, g);
      tmp2 = _m_punpcklwd(g, g);
      tmp = _m_paddsw(tmp, imm);
      tmp2 = _m_paddsw(tmp2, imm2);
      g = _m_packuswb(tmp2, tmp);

      tmp = _m_punpckhwd(b, b);
      tmp2 = _m_punpcklwd(b, b);
      tmp = _m_paddsw(tmp, imm);
      tmp2 = _m_paddsw(tmp2, imm2);
      b = _m_packuswb(tmp2, tmp);
      

      


      
      
      imm = _mm_set1_pi32(0xFFFFFFFF);
      tmp = _m_punpcklbw(r, b);
      tmp2 = _m_punpcklbw(g, imm);
      *o++ = _m_punpcklbw(tmp, tmp2);
      *o++ = _m_punpckhbw(tmp, tmp2);
      
      
      tmp = _m_punpckhbw(r, b);
      tmp2 = _m_punpckhbw(g, imm);
      *o++ = _m_punpcklbw(tmp, tmp2);
      *o++ = _m_punpckhbw(tmp, tmp2);

      
    }
    if (i & 0x1) {
      ptru += yuv->uv_width;
      ptrv += yuv->uv_width;
    }
    ptry += yuv->y_width;
  }
  _m_empty();

}


void oggplay_yuv2bgr(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb) {

  int               i;
  unsigned char   * restrict ptry;
  unsigned char   * restrict ptru;
  unsigned char   * restrict ptrv;

  register __m64    *y, *o;
  register __m64    zero, ut, vt, imm, imm2;
  register __m64    r, g, b;
  register __m64    tmp, tmp2;

  zero = _mm_setzero_si64();

  ptry = yuv->ptry;
  ptru = yuv->ptru;
  ptrv = yuv->ptrv;

  for (i = 0; i < yuv->y_height; i++) {
    int j;
    o = (__m64*)rgb->ptro;
    rgb->ptro += rgb->rgb_width * 4;
    for (j = 0; j < yuv->y_width; j += 8) {

      y = (__m64*)&ptry[j];

      ut = _m_from_int(*(int *)(ptru + j/2));
      vt = _m_from_int(*(int *)(ptrv + j/2));

      
      

      ut = _m_punpcklbw(ut, zero);
      vt = _m_punpcklbw(vt, zero);

      
      imm = _mm_set1_pi16(128);
      ut = _m_psubw(ut, imm);
      vt = _m_psubw(vt, imm);

      
      imm = _mm_set1_pi16(-51);
      g = _m_pmullw(ut, imm);
      imm = _mm_set1_pi16(130);
      b = _m_pmullw(ut, imm);
      imm = _mm_set1_pi16(146);
      r = _m_pmullw(vt, imm);
      imm = _mm_set1_pi16(-74);
      imm = _m_pmullw(vt, imm);
      g = _m_paddsw(g, imm);

      
      imm = _mm_set1_pi16(64);
      r = _m_paddsw(r, imm);
      g = _m_paddsw(g, imm);
      imm = _mm_set1_pi16(32);
      b = _m_paddsw(b, imm);

      
      r = _m_psrawi(r, 7);
      g = _m_psrawi(g, 7);
      b = _m_psrawi(b, 6);

      
      imm = _mm_set1_pi16(16);
      r = _m_psubsw(r, imm);
      g = _m_psubsw(g, imm);
      b = _m_psubsw(b, imm);

      y = (__m64*)&ptry[j];

      






      tmp = _m_punpckhwd(r, r);
      imm = _m_punpckhbw(*y, zero);
      
      tmp = _m_paddsw(tmp, imm);
      tmp2 = _m_punpcklwd(r, r);
      imm2 = _m_punpcklbw(*y, zero);
      tmp2 = _m_paddsw(tmp2, imm2);
      r = _m_packuswb(tmp2, tmp);

      tmp = _m_punpckhwd(g, g);
      tmp2 = _m_punpcklwd(g, g);
      tmp = _m_paddsw(tmp, imm);
      tmp2 = _m_paddsw(tmp2, imm2);
      g = _m_packuswb(tmp2, tmp);

      tmp = _m_punpckhwd(b, b);
      tmp2 = _m_punpcklwd(b, b);
      tmp = _m_paddsw(tmp, imm);
      tmp2 = _m_paddsw(tmp2, imm2);
      b = _m_packuswb(tmp2, tmp);
      

      


      
      
      imm = _mm_set1_pi32(0xFFFFFFFF);
      tmp = _m_punpcklbw(b, r);
      tmp2 = _m_punpcklbw(g, imm);
      *o++ = _m_punpcklbw(tmp, tmp2);
      *o++ = _m_punpckhbw(tmp, tmp2);
      
      
      tmp = _m_punpckhbw(b, r);
      tmp2 = _m_punpckhbw(g, imm);
      *o++ = _m_punpcklbw(tmp, tmp2);
      *o++ = _m_punpckhbw(tmp, tmp2);

      
    }
    if (i & 0x1) {
      ptru += yuv->uv_width;
      ptrv += yuv->uv_width;
    }
    ptry += yuv->y_width;
  }
  _m_empty();

}

#elif defined(__xxAPPLExx__)









void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb) {
}

#else

#define CLAMP(v)    ((v) > 255 ? 255 : (v) < 0 ? 0 : (v))


void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb) {

  unsigned char * ptry = yuv->ptry;
  unsigned char * ptru = yuv->ptru;
  unsigned char * ptrv = yuv->ptrv;
  unsigned char * ptro = rgb->ptro;
  unsigned char * ptro2;
  int i, j;

  for (i = 0; i < yuv->y_height; i++) {
    ptro2 = ptro;
    for (j = 0; j < yuv->y_width; j += 2) {

      short pr, pg, pb, y;
      short r, g, b;

      pr = (-56992 + ptrv[j/2] * 409) >> 8;
      pg = (34784 - ptru[j/2] * 100 - ptrv[j/2] * 208) >> 8;
      pb = (-70688 + ptru[j/2] * 516) >> 8;

      y = 298*ptry[j] >> 8;
      r = y + pr;
      g = y + pg;
      b = y + pb;

      *ptro2++ = CLAMP(r);
      *ptro2++ = CLAMP(g);
      *ptro2++ = CLAMP(b);
      *ptro2++ = 255;

      y = 298*ptry[j + 1] >> 8;
      r = y + pr;
      g = y + pg;
      b = y + pb;

      *ptro2++ = CLAMP(r);
      *ptro2++ = CLAMP(g);
      *ptro2++ = CLAMP(b);
      *ptro2++ = 255;
    }
    ptry += yuv->y_width;
    if (i & 1) {
      ptru += yuv->uv_width;
      ptrv += yuv->uv_width;
    }
    ptro += rgb->rgb_width * 4;
  }
}


void oggplay_yuv2argb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb) {

  unsigned char * ptry = yuv->ptry;
  unsigned char * ptru = yuv->ptru;
  unsigned char * ptrv = yuv->ptrv;
  unsigned char * ptro = rgb->ptro;
  unsigned char * ptro2;
  int i, j;

  for (i = 0; i < yuv->y_height; i++) {
    ptro2 = ptro;
    for (j = 0; j < yuv->y_width; j += 2) {

      short pr, pg, pb, y;
      short r, g, b;

      pr = (-56992 + ptrv[j/2] * 409) >> 8;
      pg = (34784 - ptru[j/2] * 100 - ptrv[j/2] * 208) >> 8;
      pb = (-70688 + ptru[j/2] * 516) >> 8;

      y = 298*ptry[j] >> 8;
      r = y + pr;
      g = y + pg;
      b = y + pb;

      *ptro2++ = 255;
      *ptro2++ = CLAMP(r);
      *ptro2++ = CLAMP(g);
      *ptro2++ = CLAMP(b);

      y = 298*ptry[j + 1] >> 8;
      r = y + pr;
      g = y + pg;
      b = y + pb;

      *ptro2++ = 255;
      *ptro2++ = CLAMP(r);
      *ptro2++ = CLAMP(g);
      *ptro2++ = CLAMP(b);
    }
    ptry += yuv->y_width;
    if (i & 1) {
      ptru += yuv->uv_width;
      ptrv += yuv->uv_width;
    }
    ptro += rgb->rgb_width * 4;
  }
}



void oggplay_yuv2bgr(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb) {

  unsigned char * ptry = yuv->ptry;
  unsigned char * ptru = yuv->ptru;
  unsigned char * ptrv = yuv->ptrv;
  unsigned char * ptro = rgb->ptro;
  unsigned char * ptro2;
  int i, j;

  for (i = 0; i < yuv->y_height; i++) {
    ptro2 = ptro;
    for (j = 0; j < yuv->y_width; j += 2) {

      short pr, pg, pb, y;
      short r, g, b;

      pr = (-56992 + ptrv[j/2] * 409) >> 8;
      pg = (34784 - ptru[j/2] * 100 - ptrv[j/2] * 208) >> 8;
      pb = (-70688 + ptru[j/2] * 516) >> 8;

      y = 298*ptry[j] >> 8;
      r = y + pr;
      g = y + pg;
      b = y + pb;

      *ptro2++ = CLAMP(b);
      *ptro2++ = CLAMP(g);
      *ptro2++ = CLAMP(r);
      *ptro2++ = 255;

      y = 298*ptry[j + 1] >> 8;
      r = y + pr;
      g = y + pg;
      b = y + pb;

      *ptro2++ = CLAMP(b);
      *ptro2++ = CLAMP(g);
      *ptro2++ = CLAMP(r);
      *ptro2++ = 255;
    }
    ptry += yuv->y_width;
    if (i & 1) {
      ptru += yuv->uv_width;
      ptrv += yuv->uv_width;
    }
    ptro += rgb->rgb_width * 4;
  }
}

#endif
