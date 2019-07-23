














































#ifndef NAMEPREP_TEMPLATE_INIT
#define NAMEPREP_TEMPLATE_INIT

#include "prtypes.h"


#define compose_sym2(a, b)		compose_sym2X(a, b)
#define compose_sym2X(a, b)		a ## b
#define compose_sym3(a, b, c)		compose_sym3X(a, b, c)
#define compose_sym3X(a, b, c)		a ## b ## c






#define IDX0(type, v) IDX_0(v, BITS1(type), BITS2(type))
#define IDX1(type, v) IDX_1(v, BITS1(type), BITS2(type))
#define IDX2(type, v) IDX_2(v, BITS1(type), BITS2(type))

#define IDX_0(v, bits1, bits2)	((v) >> ((bits1) + (bits2)))
#define IDX_1(v, bits1, bits2)	(((v) >> (bits2)) & ((1 << (bits1)) - 1))
#define IDX_2(v, bits1, bits2)	((v) & ((1 << (bits2)) - 1))

#define BITS1(type)	type ## _BITS_1
#define BITS2(type)	type ## _BITS_2

#endif 

static const char *
compose_sym2(nameprep_map_, VERSION) (PRUint32 v) {
	int idx0 = IDX0(MAP, v);
	int idx1 = IDX1(MAP, v);
	int idx2 = IDX2(MAP, v);
	int offset;

#define IMAP	compose_sym3(nameprep_, VERSION, _map_imap)
#define TABLE	compose_sym3(nameprep_, VERSION, _map_table)
#define DATA	compose_sym3(nameprep_, VERSION, _map_data)
	offset = TABLE[IMAP[IMAP[idx0] + idx1]].tbl[idx2];
	if (offset == 0)
		return (NULL);	
	return (const char *)(DATA + offset);
#undef IMAP
#undef TABLE
#undef DATA
}

static int
compose_sym2(nameprep_prohibited_, VERSION) (PRUint32 v) {
	int idx0 = IDX0(PROH, v);
	int idx1 = IDX1(PROH, v);
	int idx2 = IDX2(PROH, v);
	const unsigned char *bm;

#define IMAP	compose_sym3(nameprep_, VERSION, _prohibited_imap)
#define BITMAP	compose_sym3(nameprep_, VERSION, _prohibited_bitmap)
	bm = BITMAP[IMAP[IMAP[idx0] + idx1]].bm;
	return (bm[idx2 / 8] & (1 << (idx2 % 8)));
#undef IMAP
#undef BITMAP
}

static int
compose_sym2(nameprep_unassigned_, VERSION) (PRUint32 v) {
	int idx0 = IDX0(UNAS, v);
	int idx1 = IDX1(UNAS, v);
	int idx2 = IDX2(UNAS, v);
	const unsigned char *bm;

#define IMAP	compose_sym3(nameprep_, VERSION, _unassigned_imap)
#define BITMAP	compose_sym3(nameprep_, VERSION, _unassigned_bitmap)
	bm = BITMAP[IMAP[IMAP[idx0] + idx1]].bm;
	return (bm[idx2 / 8] & (1 << (idx2 % 8)));
#undef IMAP
#undef BITMAP
}

static idn_biditype_t
compose_sym2(nameprep_biditype_, VERSION) (PRUint32 v) {
	int idx0 = IDX0(BIDI, v);
	int idx1 = IDX1(BIDI, v);
	int idx2 = IDX2(BIDI, v);
	int offset;

#define IMAP	compose_sym3(nameprep_, VERSION, _bidi_imap)
#define TABLE	compose_sym3(nameprep_, VERSION, _bidi_table)
#define DATA	compose_sym3(nameprep_, VERSION, _bidi_data)
	offset = TABLE[IMAP[IMAP[idx0] + idx1]].tbl[idx2];
	return DATA[offset];
#undef IMAP
#undef TABLE
#undef DATA
}
