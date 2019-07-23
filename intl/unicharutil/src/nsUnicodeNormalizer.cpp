





 














































































#include <stdlib.h>
#include <string.h>

#include "nsUnicharUtils.h"
#include "nsMemory.h"
#include "nsCRT.h"
#include "nsUnicodeNormalizer.h"
#include "nsString.h"
#include "nsReadableUtils.h"

NS_IMPL_ISUPPORTS1(nsUnicodeNormalizer, nsIUnicodeNormalizer)


nsUnicodeNormalizer::nsUnicodeNormalizer()
{
}

nsUnicodeNormalizer::~nsUnicodeNormalizer()
{
}



#define NS_ERROR_UNORM_MOREOUTPUT  \
        NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL, 0x21)

#define NS_SUCCESS_UNORM_NOTFOUND  \
        NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x11)


#define END_BIT		0x80000000






#define SBase		0xac00
#define LBase		0x1100
#define VBase		0x1161
#define TBase		0x11a7
#define LCount		19
#define VCount		21
#define TCount		28
#define SLast		(SBase + LCount * VCount * TCount)

struct composition {
	PRUint32 c2;	
	PRUint32 comp;	
};


#include "normalization_data.h"




#define LOOKUPTBL(vprefix, mprefix, v) \
	DMAP(vprefix)[\
		IMAP(vprefix)[\
			IMAP(vprefix)[IDX0(mprefix, v)] + IDX1(mprefix, v)\
		]\
	].tbl[IDX2(mprefix, v)]

#define IDX0(mprefix, v) IDX_0(v, BITS1(mprefix), BITS2(mprefix))
#define IDX1(mprefix, v) IDX_1(v, BITS1(mprefix), BITS2(mprefix))
#define IDX2(mprefix, v) IDX_2(v, BITS1(mprefix), BITS2(mprefix))

#define IDX_0(v, bits1, bits2)	((v) >> ((bits1) + (bits2)))
#define IDX_1(v, bits1, bits2)	(((v) >> (bits2)) & ((1 << (bits1)) - 1))
#define IDX_2(v, bits1, bits2)	((v) & ((1 << (bits2)) - 1))

#define BITS1(mprefix)	mprefix ## _BITS_1
#define BITS2(mprefix)	mprefix ## _BITS_2

#define IMAP(vprefix)	vprefix ## _imap
#define DMAP(vprefix)	vprefix ## _table
#define SEQ(vprefix)	vprefix ## _seq

static PRInt32
canonclass(PRUint32 c) {
	
	return (LOOKUPTBL(canon_class, CANON_CLASS, c));
}

static PRInt32
decompose_char(PRUint32 c, const PRUint32 **seqp)
{
	
	PRInt32 seqidx = LOOKUPTBL(decompose, DECOMP, c);
	*seqp = SEQ(decompose) + (seqidx & ~DECOMP_COMPAT);
	return (seqidx);
}

static PRInt32
compose_char(PRUint32 c,
				const struct composition **compp)
{
	
	PRInt32 seqidx = LOOKUPTBL(compose, CANON_COMPOSE, c);
	*compp = SEQ(compose) + (seqidx & 0xffff);
	return (seqidx >> 16);
}

static nsresult
mdn__unicode_decompose(PRInt32 compat, PRUint32 *v, size_t vlen,
		       PRUint32 c, PRInt32 *decomp_lenp)
{
	PRUint32 *vorg = v;
	PRInt32 seqidx;
	const PRUint32 *seq;

	

	


	if (SBase <= c && c < SLast) {
		PRInt32 idx, t_offset, v_offset, l_offset;

		idx = c - SBase;
		t_offset = idx % TCount;
		idx /= TCount;
		v_offset = idx % VCount;
		l_offset = idx / VCount;
		if ((t_offset == 0 && vlen < 2) || (t_offset > 0 && vlen < 3))
			return (NS_ERROR_UNORM_MOREOUTPUT);
		*v++ = LBase + l_offset;
		*v++ = VBase + v_offset;
		if (t_offset > 0)
			*v++ = TBase + t_offset;
		*decomp_lenp = v - vorg;
		return (NS_OK);
	}

	




	seqidx = decompose_char(c, &seq);
	if (seqidx == 0 || (compat == 0 && (seqidx & DECOMP_COMPAT) != 0))
		return (NS_SUCCESS_UNORM_NOTFOUND);
	
	



	do {
		PRUint32 c;
		PRInt32 dlen;
		nsresult r;

		c = *seq & ~END_BIT;

		
		r = mdn__unicode_decompose(compat, v, vlen, c, &dlen);
		if (r == NS_OK) {
			v += dlen;
			vlen -= dlen;
		} else if (r == NS_SUCCESS_UNORM_NOTFOUND) {
			if (vlen < 1)
				return (NS_ERROR_UNORM_MOREOUTPUT);
			*v++ = c;
			vlen--;
		} else {
			return (r);
		}

	} while ((*seq++ & END_BIT) == 0);
	
	*decomp_lenp = v - vorg;

	return (NS_OK);
}

static PRInt32
mdn__unicode_iscompositecandidate(PRUint32 c)
{
	const struct composition *dummy;

	
	if ((LBase <= c && c < LBase + LCount) || (SBase <= c && c < SLast))
		return (1);

	




	if (compose_char(c, &dummy) == 0)
		return (0);
	else
		return (1);
}

static nsresult
mdn__unicode_compose(PRUint32 c1, PRUint32 c2, PRUint32 *compp)
{
	PRInt32 n;
	PRInt32 lo, hi;
	const struct composition *cseq;

	

	


	if (LBase <= c1 && c1 < LBase + LCount &&
	    VBase <= c2 && c2 < VBase + VCount) {
		


		*compp = SBase +
			((c1 - LBase) * VCount + (c2 - VBase)) * TCount;
		return (NS_OK);
	} else if (SBase <= c1 && c1 < SLast &&
		   TBase <= c2 && c2 < TBase + TCount &&
		   (c1 - SBase) % TCount == 0) {
		


		*compp = c1 + (c2 - TBase);
		return (NS_OK);
	}

	





	if ((n = compose_char(c1, &cseq)) == 0)
		return (NS_SUCCESS_UNORM_NOTFOUND);

	



	lo = 0;
	hi = n - 1;
	while (lo <= hi) {
		PRInt32 mid = (lo + hi) / 2;

		if (cseq[mid].c2 < c2) {
			lo = mid + 1;
		} else if (cseq[mid].c2 > c2) {
			hi = mid - 1;
		} else {
			*compp = cseq[mid].comp;
			return (NS_OK);
		}
	}
	return (NS_SUCCESS_UNORM_NOTFOUND);
}


#define WORKBUF_SIZE		128
#define WORKBUF_SIZE_MAX	10000

typedef struct {
	PRInt32 cur;		
	PRInt32 last;		
	PRInt32 size;		
	PRUint32 *ucs;	
	PRInt32 *cclass;		
	PRUint32 ucs_buf[WORKBUF_SIZE];	
	PRInt32 class_buf[WORKBUF_SIZE];		
} workbuf_t;

static nsresult	decompose(workbuf_t *wb, PRUint32 c, PRInt32 compat);
static void		get_class(workbuf_t *wb);
static void		reorder(workbuf_t *wb);
static void		compose(workbuf_t *wb);
static nsresult flush_before_cur(workbuf_t *wb, nsAString& aToStr);
static void		workbuf_init(workbuf_t *wb);
static void		workbuf_free(workbuf_t *wb);
static nsresult	workbuf_extend(workbuf_t *wb);
static nsresult	workbuf_append(workbuf_t *wb, PRUint32 c);
static void		workbuf_shift(workbuf_t *wb, PRInt32 shift);
static void		workbuf_removevoid(workbuf_t *wb);


static nsresult
mdn_normalize(PRBool do_composition, PRBool compat,
	  const nsAString& aSrcStr, nsAString& aToStr)
{
	workbuf_t wb;
	nsresult r = NS_OK;
	


	workbuf_init(&wb);

	nsAString::const_iterator start, end;
	aSrcStr.BeginReading(start); 
	aSrcStr.EndReading(end); 

	while (start != end) {
		PRUint32 c;
		PRUnichar curChar;

		

		


		curChar= *start++;

		if (NS_IS_HIGH_SURROGATE(curChar) && start != end && NS_IS_LOW_SURROGATE(*(start)) ) {
			c = SURROGATE_TO_UCS4(curChar, *start);
			++start;
		} else {
			c = curChar;
		}

		


		if ((r = decompose(&wb, c, compat)) != NS_OK)
			break;

		


		get_class(&wb);

		


		for (; wb.cur < wb.last; wb.cur++) {
			if (wb.cur == 0) {
				continue;
			} else if (wb.cclass[wb.cur] > 0) {
				




				reorder(&wb);
				continue;
			}

			





			if (do_composition && wb.cclass[0] == 0)
				compose(&wb);

			






			if (wb.cur > 0 && wb.cclass[wb.cur] == 0) {
				
				r = flush_before_cur(&wb, aToStr);
				if (r != NS_OK)
					break;
			}
		}
	}

	if (r == NS_OK) {
		if (do_composition && wb.cur > 0 && wb.cclass[0] == 0) {
			







			wb.cur--;
			compose(&wb);
			wb.cur++;
		}
		



		r = flush_before_cur(&wb, aToStr);
	}

	workbuf_free(&wb);

	return (r);
}

static nsresult
decompose(workbuf_t *wb, PRUint32 c, PRInt32 compat) {
	nsresult r;
	PRInt32 dec_len;

again:
	r = mdn__unicode_decompose(compat, wb->ucs + wb->last,
				   wb->size - wb->last, c, &dec_len);
	switch (r) {
	case NS_OK:
		wb->last += dec_len;
		return (NS_OK);
	case NS_SUCCESS_UNORM_NOTFOUND:
		return (workbuf_append(wb, c));
	case NS_ERROR_UNORM_MOREOUTPUT:
		if ((r = workbuf_extend(wb)) != NS_OK)
			return (r);
		if (wb->size > WORKBUF_SIZE_MAX) {
			
			return (NS_ERROR_FAILURE);
		}
		goto again;
	default:
		return (r);
	}
	
}

static void		
get_class(workbuf_t *wb) {
	PRInt32 i;

	for (i = wb->cur; i < wb->last; i++)
		wb->cclass[i] = canonclass(wb->ucs[i]);
}

static void
reorder(workbuf_t *wb) {
	PRUint32 c;
	PRInt32 i;
	PRInt32 cclass;

	

	i = wb->cur;
	c = wb->ucs[i];
	cclass = wb->cclass[i];

	while (i > 0 && wb->cclass[i - 1] > cclass) {
		wb->ucs[i] = wb->ucs[i - 1];
		wb->cclass[i] =wb->cclass[i - 1];
		i--;
		wb->ucs[i] = c;
		wb->cclass[i] = cclass;
	}
}

static void
compose(workbuf_t *wb) {
	PRInt32 cur;
	PRUint32 *ucs;
	PRInt32 *cclass;
	PRInt32 last_class;
	PRInt32 nvoids;
	PRInt32 i;

	

	cur = wb->cur;
	ucs = wb->ucs;
	cclass = wb->cclass;

	



	if (!mdn__unicode_iscompositecandidate(ucs[0]))
		return;

	last_class = 0;
	nvoids = 0;
	for (i = 1; i <= cur; i++) {
		PRUint32 c;
		PRInt32 cl = cclass[i];

		if ((last_class < cl || cl == 0) &&
		    mdn__unicode_compose(ucs[0], ucs[i],
					 &c) == NS_OK) {
			


			ucs[0] = c;
			cclass[0] = canonclass(c);

			cclass[i] = -1;	
			nvoids++;
		} else {
			last_class = cl;
		}
	}

	
	if (nvoids > 0)
		workbuf_removevoid(wb);
}

static nsresult
flush_before_cur(workbuf_t *wb, nsAString& aToStr) 
{
	PRInt32 i;

	for (i = 0; i < wb->cur; i++) {
		if (!IS_IN_BMP(wb->ucs[i])) {
			aToStr.Append((PRUnichar)H_SURROGATE(wb->ucs[i]));
			aToStr.Append((PRUnichar)L_SURROGATE(wb->ucs[i]));
		} else {
			aToStr.Append((PRUnichar)(wb->ucs[i]));
		}
	}

	workbuf_shift(wb, wb->cur);

	return (NS_OK);
}

static void
workbuf_init(workbuf_t *wb) {
	wb->cur = 0;
	wb->last = 0;
	wb->size = WORKBUF_SIZE;
	wb->ucs = wb->ucs_buf;
	wb->cclass = wb->class_buf;
}

static void
workbuf_free(workbuf_t *wb) {
	if (wb->ucs != wb->ucs_buf) {
		nsMemory::Free(wb->ucs);
		nsMemory::Free(wb->cclass);
	}
}

static nsresult
workbuf_extend(workbuf_t *wb) {
	PRInt32 newsize = wb->size * 3;

	if (wb->ucs == wb->ucs_buf) {
		wb->ucs = (PRUint32*)nsMemory::Alloc(sizeof(wb->ucs[0]) * newsize);
		if (!wb->ucs)
			return NS_ERROR_OUT_OF_MEMORY;
		wb->cclass = (PRInt32*)nsMemory::Alloc(sizeof(wb->cclass[0]) * newsize);
		if (!wb->cclass) {
			nsMemory::Free(wb->ucs);
			wb->ucs = NULL;
			return NS_ERROR_OUT_OF_MEMORY;
		}
	} else {
		void* buf = nsMemory::Realloc(wb->ucs, sizeof(wb->ucs[0]) * newsize);
		if (!buf)
			return NS_ERROR_OUT_OF_MEMORY;
		wb->ucs = (PRUint32*)buf;
		buf = nsMemory::Realloc(wb->cclass, sizeof(wb->cclass[0]) * newsize);
		if (!buf)
			return NS_ERROR_OUT_OF_MEMORY;
		wb->cclass = (PRInt32*)buf;
	}
	return (NS_OK);
}

static nsresult
workbuf_append(workbuf_t *wb, PRUint32 c) {
	nsresult r;

	if (wb->last >= wb->size && (r = workbuf_extend(wb)) != NS_OK)
		return (r);
	wb->ucs[wb->last++] = c;
	return (NS_OK);
}

static void
workbuf_shift(workbuf_t *wb, PRInt32 shift) {
	PRInt32 nmove;

	

	nmove = wb->last - shift;
	memmove(&wb->ucs[0], &wb->ucs[shift],
		      nmove * sizeof(wb->ucs[0]));
	memmove(&wb->cclass[0], &wb->cclass[shift],
		      nmove * sizeof(wb->cclass[0]));
	wb->cur -= shift;
	wb->last -= shift;
}

static void
workbuf_removevoid(workbuf_t *wb) {
	PRInt32 i, j;
	PRInt32 last = wb->last;

	for (i = j = 0; i < last; i++) {
		if (wb->cclass[i] >= 0) {
			if (j < i) {
				wb->ucs[j] = wb->ucs[i];
				wb->cclass[j] = wb->cclass[i];
			}
			j++;
		}
	}
	wb->cur -= last - j;
	wb->last = j;
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFD( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(PR_FALSE, PR_FALSE, aSrc, aDest);
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFC( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(PR_TRUE, PR_FALSE, aSrc, aDest);
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFKD( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(PR_FALSE, PR_TRUE, aSrc, aDest);
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFKC( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(PR_TRUE, PR_TRUE, aSrc, aDest);
}

