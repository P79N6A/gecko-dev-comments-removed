







































































#ifndef PAGE_H
#define PAGE_H

#define	PAIRSIZE(K,D)	(2*sizeof(uint16) + (K)->size + (D)->size)
#define BIGOVERHEAD	(4*sizeof(uint16))
#define KEYSIZE(K)	(4*sizeof(uint16) + (K)->size);
#define OVFLSIZE	(2*sizeof(uint16))
#define FREESPACE(P)	((P)[(P)[0]+1])
#define	OFFSET(P)	((P)[(P)[0]+2])
#define PAIRFITS(P,K,D) \
	(((P)[2] >= REAL_KEY) && \
	    (PAIRSIZE((K),(D)) + OVFLSIZE) <= FREESPACE((P)))
#define PAGE_META(N)	(((N)+3) * sizeof(uint16))

typedef struct {
	BUFHEAD *newp;
	BUFHEAD *oldp;
	BUFHEAD *nextp;
	uint16 next_addr;
}       SPLIT_RETURN;
#endif

