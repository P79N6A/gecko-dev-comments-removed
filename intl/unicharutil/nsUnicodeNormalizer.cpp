




















































#include <string.h>

#include "nsMemory.h"
#include "nsUnicodeNormalizer.h"
#include "nsString.h"
#include "mozilla/BinarySearch.h"

NS_IMPL_ISUPPORTS(nsUnicodeNormalizer, nsIUnicodeNormalizer)


nsUnicodeNormalizer::nsUnicodeNormalizer()
{
}

nsUnicodeNormalizer::~nsUnicodeNormalizer()
{
}



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
	uint32_t c2;	
	uint32_t comp;	
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

static int32_t
canonclass(uint32_t c) {
	
	return (LOOKUPTBL(canon_class, CANON_CLASS, c));
}

static int32_t
decompose_char(uint32_t c, const uint32_t **seqp)
{
	
	int32_t seqidx = LOOKUPTBL(decompose, DECOMP, c);
	*seqp = SEQ(decompose) + (seqidx & ~DECOMP_COMPAT);
	return (seqidx);
}

static int32_t
compose_char(uint32_t c,
				const struct composition **compp)
{
	
	int32_t seqidx = LOOKUPTBL(compose, CANON_COMPOSE, c);
	*compp = SEQ(compose) + (seqidx & 0xffff);
	return (seqidx >> 16);
}

static nsresult
mdn__unicode_decompose(int32_t compat, uint32_t *v, size_t vlen,
		       uint32_t c, int32_t *decomp_lenp)
{
	uint32_t *vorg = v;
	int32_t seqidx;
	const uint32_t *seq;

	

	


	if (SBase <= c && c < SLast) {
		int32_t idx, t_offset, v_offset, l_offset;

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
		uint32_t c;
		int32_t dlen;
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

static int32_t
mdn__unicode_iscompositecandidate(uint32_t c)
{
	const struct composition *dummy;

	
	if ((LBase <= c && c < LBase + LCount) || (SBase <= c && c < SLast))
		return (1);

	




	if (compose_char(c, &dummy) == 0)
		return (0);
	else
		return (1);
}

namespace {

struct SequenceAdaptor
{
	const composition* const mSequence;
	SequenceAdaptor(const composition* aSequence) : mSequence(aSequence) {}
	uint32_t operator[](size_t aIdx) const {
		return mSequence[aIdx].c2;
	}
};

} 

static nsresult
mdn__unicode_compose(uint32_t c1, uint32_t c2, uint32_t *compp)
{
	int32_t n;
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

	



	size_t idx;
	if (mozilla::BinarySearch(SequenceAdaptor(cseq), 0, n, c2, &idx)) {
		*compp = cseq[idx].comp;
		return (NS_OK);
	}

	return (NS_SUCCESS_UNORM_NOTFOUND);
}


#define WORKBUF_SIZE		128
#define WORKBUF_SIZE_MAX	10000

typedef struct {
	int32_t cur;		
	int32_t last;		
	int32_t size;		
	uint32_t *ucs;	
	int32_t *cclass;		
	uint32_t ucs_buf[WORKBUF_SIZE];	
	int32_t class_buf[WORKBUF_SIZE];		
} workbuf_t;

static nsresult	decompose(workbuf_t *wb, uint32_t c, int32_t compat);
static void		get_class(workbuf_t *wb);
static void		reorder(workbuf_t *wb);
static void		compose(workbuf_t *wb);
static nsresult flush_before_cur(workbuf_t *wb, nsAString& aToStr);
static void		workbuf_init(workbuf_t *wb);
static void		workbuf_free(workbuf_t *wb);
static nsresult	workbuf_extend(workbuf_t *wb);
static nsresult	workbuf_append(workbuf_t *wb, uint32_t c);
static void		workbuf_shift(workbuf_t *wb, int32_t shift);
static void		workbuf_removevoid(workbuf_t *wb);


static nsresult
mdn_normalize(bool do_composition, bool compat,
	  const nsAString& aSrcStr, nsAString& aToStr)
{
	workbuf_t wb;
	nsresult r = NS_OK;
	


	workbuf_init(&wb);

	nsAString::const_iterator start, end;
	aSrcStr.BeginReading(start); 
	aSrcStr.EndReading(end); 

	while (start != end) {
		uint32_t c;
		char16_t curChar;

		

		


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
decompose(workbuf_t *wb, uint32_t c, int32_t compat) {
	nsresult r;
	int32_t dec_len;

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
	int32_t i;

	for (i = wb->cur; i < wb->last; i++)
		wb->cclass[i] = canonclass(wb->ucs[i]);
}

static void
reorder(workbuf_t *wb) {
	uint32_t c;
	int32_t i;
	int32_t cclass;

	

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
	int32_t cur;
	uint32_t *ucs;
	int32_t *cclass;
	int32_t last_class;
	int32_t nvoids;
	int32_t i;

	

	cur = wb->cur;
	ucs = wb->ucs;
	cclass = wb->cclass;

	



	if (!mdn__unicode_iscompositecandidate(ucs[0]))
		return;

	last_class = 0;
	nvoids = 0;
	for (i = 1; i <= cur; i++) {
		uint32_t c;
		int32_t cl = cclass[i];

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
	int32_t i;

	for (i = 0; i < wb->cur; i++) {
		if (!IS_IN_BMP(wb->ucs[i])) {
			aToStr.Append((char16_t)H_SURROGATE(wb->ucs[i]));
			aToStr.Append((char16_t)L_SURROGATE(wb->ucs[i]));
		} else {
			aToStr.Append((char16_t)(wb->ucs[i]));
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
	int32_t newsize = wb->size * 3;

	if (wb->ucs == wb->ucs_buf) {
		wb->ucs = (uint32_t*)nsMemory::Alloc(sizeof(wb->ucs[0]) * newsize);
		if (!wb->ucs)
			return NS_ERROR_OUT_OF_MEMORY;
		wb->cclass = (int32_t*)nsMemory::Alloc(sizeof(wb->cclass[0]) * newsize);
		if (!wb->cclass) {
			nsMemory::Free(wb->ucs);
			wb->ucs = nullptr;
			return NS_ERROR_OUT_OF_MEMORY;
		}
	} else {
		void* buf = nsMemory::Realloc(wb->ucs, sizeof(wb->ucs[0]) * newsize);
		if (!buf)
			return NS_ERROR_OUT_OF_MEMORY;
		wb->ucs = (uint32_t*)buf;
		buf = nsMemory::Realloc(wb->cclass, sizeof(wb->cclass[0]) * newsize);
		if (!buf)
			return NS_ERROR_OUT_OF_MEMORY;
		wb->cclass = (int32_t*)buf;
	}
	return (NS_OK);
}

static nsresult
workbuf_append(workbuf_t *wb, uint32_t c) {
	nsresult r;

	if (wb->last >= wb->size && (r = workbuf_extend(wb)) != NS_OK)
		return (r);
	wb->ucs[wb->last++] = c;
	return (NS_OK);
}

static void
workbuf_shift(workbuf_t *wb, int32_t shift) {
	int32_t nmove;

	

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
	int32_t i, j;
	int32_t last = wb->last;

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
  return mdn_normalize(false, false, aSrc, aDest);
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFC( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(true, false, aSrc, aDest);
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFKD( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(false, true, aSrc, aDest);
}

nsresult  
nsUnicodeNormalizer::NormalizeUnicodeNFKC( const nsAString& aSrc, nsAString& aDest)
{
  return mdn_normalize(true, true, aSrc, aDest);
}

bool
nsUnicodeNormalizer::Compose(uint32_t a, uint32_t b, uint32_t *ab)
{
  return mdn__unicode_compose(a, b, ab) == NS_OK;
}

bool
nsUnicodeNormalizer::DecomposeNonRecursively(uint32_t c, uint32_t *c1, uint32_t *c2)
{
  
  
  
  
  
  const uint32_t *seq;
  uint32_t seqidx = decompose_char(c, &seq);
  if (seqidx == 0 || ((seqidx & DECOMP_COMPAT) != 0)) {
    return false;
  }
  *c1 = *seq & ~END_BIT;
  if (*seq & END_BIT) {
    *c2 = 0;
  } else {
    *c2 = *++seq & ~END_BIT;
  }
  return true;
}
