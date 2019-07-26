







































#ifndef FIXED_GENERIC_H
#define FIXED_GENERIC_H


#define MULT16_16SU(a,b) ((opus_val32)(opus_val16)(a)*(opus_val32)(opus_uint16)(b))


#define MULT16_32_Q16(a,b) ADD32(MULT16_16((a),SHR((b),16)), SHR(MULT16_16SU((a),((b)&0x0000ffff)),16))


#define MULT16_32_Q15(a,b) ADD32(SHL(MULT16_16((a),SHR((b),16)),1), SHR(MULT16_16SU((a),((b)&0x0000ffff)),15))


#define MULT32_32_Q31(a,b) ADD32(ADD32(SHL(MULT16_16(SHR((a),16),SHR((b),16)),1), SHR(MULT16_16SU(SHR((a),16),((b)&0x0000ffff)),15)), SHR(MULT16_16SU(SHR((b),16),((a)&0x0000ffff)),15))


#define QCONST16(x,bits) ((opus_val16)(.5+(x)*(((opus_val32)1)<<(bits))))


#define QCONST32(x,bits) ((opus_val32)(.5+(x)*(((opus_val32)1)<<(bits))))


#define NEG16(x) (-(x))

#define NEG32(x) (-(x))


#define EXTRACT16(x) ((opus_val16)(x))

#define EXTEND32(x) ((opus_val32)(x))


#define SHR16(a,shift) ((a) >> (shift))

#define SHL16(a,shift) ((opus_int16)((opus_uint16)(a)<<(shift)))

#define SHR32(a,shift) ((a) >> (shift))

#define SHL32(a,shift) ((opus_int32)((opus_uint32)(a)<<(shift)))


#define PSHR32(a,shift) (SHR32((a)+((EXTEND32(1)<<((shift))>>1)),shift))

#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))


#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) SHL32(a,shift)
#define PSHR(a,shift) (SHR((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define SATURATE(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))


#define ROUND16(x,a) (EXTRACT16(PSHR32((x),(a))))

#define HALF16(x)  (SHR16(x,1))
#define HALF32(x)  (SHR32(x,1))


#define ADD16(a,b) ((opus_val16)((opus_val16)(a)+(opus_val16)(b)))

#define SUB16(a,b) ((opus_val16)(a)-(opus_val16)(b))

#define ADD32(a,b) ((opus_val32)(a)+(opus_val32)(b))

#define SUB32(a,b) ((opus_val32)(a)-(opus_val32)(b))


#define MULT16_16_16(a,b)     ((((opus_val16)(a))*((opus_val16)(b))))



#define MULT16_16(a,b)     (((opus_val32)(opus_val16)(a))*((opus_val32)(opus_val16)(b)))


#define MAC16_16(c,a,b) (ADD32((c),MULT16_16((a),(b))))

#define MAC16_32_Q15(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15)))

#define MULT16_16_Q11_32(a,b) (SHR(MULT16_16((a),(b)),11))
#define MULT16_16_Q13(a,b) (SHR(MULT16_16((a),(b)),13))
#define MULT16_16_Q14(a,b) (SHR(MULT16_16((a),(b)),14))
#define MULT16_16_Q15(a,b) (SHR(MULT16_16((a),(b)),15))

#define MULT16_16_P13(a,b) (SHR(ADD32(4096,MULT16_16((a),(b))),13))
#define MULT16_16_P14(a,b) (SHR(ADD32(8192,MULT16_16((a),(b))),14))
#define MULT16_16_P15(a,b) (SHR(ADD32(16384,MULT16_16((a),(b))),15))


#define DIV32_16(a,b) ((opus_val16)(((opus_val32)(a))/((opus_val16)(b))))


#define DIV32(a,b) (((opus_val32)(a))/((opus_val32)(b)))

#endif
