























#ifndef _ICROP_H_
#define _ICROP_H_

typedef struct _mergeRopBits {
    FbBits   ca1, cx1, ca2, cx2;
} FbMergeRopRec, *FbMergeRopPtr;

extern pixman_private const FbMergeRopRec FbMergeRopBits[16];

#define FbDeclareMergeRop() FbBits   _ca1, _cx1, _ca2, _cx2;
#define FbDeclarePrebuiltMergeRop()	FbBits	_cca, _ccx;

#define FbInitializeMergeRop(alu,pm) {\
    const FbMergeRopRec  *_bits; \
    _bits = &FbMergeRopBits[alu]; \
    _ca1 = _bits->ca1 &  pm; \
    _cx1 = _bits->cx1 | ~pm; \
    _ca2 = _bits->ca2 &  pm; \
    _cx2 = _bits->cx2 &  pm; \
}

#define FbDestInvarientRop(alu,pm)  ((pm) == FB_ALLONES && \
				     (((alu) >> 1 & 5) == ((alu) & 5)))

#define FbDestInvarientMergeRop()   (_ca1 == 0 && _cx1 == 0)



#define FbDoMergeRop(src, dst) \
    (((dst) & (((src) & _ca1) ^ _cx1)) ^ (((src) & _ca2) ^ _cx2))

#define FbDoDestInvarientMergeRop(src)	(((src) & _ca2) ^ _cx2)

#define FbDoMaskMergeRop(src, dst, mask) \
    (((dst) & ((((src) & _ca1) ^ _cx1) | ~(mask))) ^ ((((src) & _ca2) ^ _cx2) & (mask)))

#define FbDoLeftMaskByteMergeRop(dst, src, lb, l) { \
    FbBits  __xor = ((src) & _ca2) ^ _cx2; \
    FbDoLeftMaskByteRRop(dst,lb,l,((src) & _ca1) ^ _cx1,__xor); \
}

#define FbDoRightMaskByteMergeRop(dst, src, rb, r) { \
    FbBits  __xor = ((src) & _ca2) ^ _cx2; \
    FbDoRightMaskByteRRop(dst,rb,r,((src) & _ca1) ^ _cx1,__xor); \
}

#define FbDoRRop(dst, and, xor)	(((dst) & (and)) ^ (xor))

#define FbDoMaskRRop(dst, and, xor, mask) \
    (((dst) & ((and) | ~(mask))) ^ (xor & mask))




#define fbFillFromBit(b,t)	(~((t) ((b) & 1)-1))

#define fbXorT(rop,fg,pm,t) ((((fg) & fbFillFromBit((rop) >> 1,t)) | \
			      (~(fg) & fbFillFromBit((rop) >> 3,t))) & (pm))

#define fbAndT(rop,fg,pm,t) ((((fg) & fbFillFromBit (rop ^ (rop>>1),t)) | \
			      (~(fg) & fbFillFromBit((rop>>2) ^ (rop>>3),t))) | \
			     ~(pm))

#define fbXor(rop,fg,pm)	fbXorT(rop,fg,pm,FbBits)

#define fbAnd(rop,fg,pm)	fbAndT(rop,fg,pm,FbBits)

#define fbXorStip(rop,fg,pm)    fbXorT(rop,fg,pm,FbStip)

#define fbAndStip(rop,fg,pm)	fbAndT(rop,fg,pm,FbStip)






extern pixman_private const pixman_bits_t fbStipple16Bits[256];
#define FbStipple16Bits(b) \
    (fbStipple16Bits[(b)&0xff] | fbStipple16Bits[(b) >> 8] << FB_HALFUNIT)

pixman_private const pixman_bits_t *
fbStippleTable(int bits);

#define FbStippleRRop(dst, b, fa, fx, ba, bx) \
    (FbDoRRop(dst, fa, fx) & b) | (FbDoRRop(dst, ba, bx) & ~b)

#define FbStippleRRopMask(dst, b, fa, fx, ba, bx, m) \
    (FbDoMaskRRop(dst, fa, fx, m) & (b)) | (FbDoMaskRRop(dst, ba, bx, m) & ~(b))

#define FbDoLeftMaskByteStippleRRop(dst, b, fa, fx, ba, bx, lb, l) { \
    FbBits  __xor = ((fx) & (b)) | ((bx) & ~(b)); \
    FbDoLeftMaskByteRRop(dst, lb, l, ((fa) & (b)) | ((ba) & ~(b)), __xor); \
}

#define FbDoRightMaskByteStippleRRop(dst, b, fa, fx, ba, bx, rb, r) { \
    FbBits  __xor = ((fx) & (b)) | ((bx) & ~(b)); \
    FbDoRightMaskByteRRop(dst, rb, r, ((fa) & (b)) | ((ba) & ~(b)), __xor); \
}

#define FbOpaqueStipple(b, fg, bg) (((fg) & (b)) | ((bg) & ~(b)))





#define FbStipple1RopPick(alu,b)    (((alu) >> (2 - (((b) & 1) << 1))) & 3)

#define FbOpaqueStipple1Rop(alu,fg,bg)    (FbStipple1RopPick(alu,fg) | \
					   (FbStipple1RopPick(alu,bg) << 2))

#define FbStipple1Rop(alu,fg)	    (FbStipple1RopPick(alu,fg) | 4)

#endif
