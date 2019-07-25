#include "jemalloc/internal/jemalloc_internal.h"





#define	QUARANTINE_STATE_REINCARNATED	((quarantine_t *)(uintptr_t)1)
#define	QUARANTINE_STATE_PURGATORY	((quarantine_t *)(uintptr_t)2)
#define	QUARANTINE_STATE_MAX		QUARANTINE_STATE_PURGATORY




typedef struct quarantine_obj_s quarantine_obj_t;
typedef struct quarantine_s quarantine_t;

struct quarantine_obj_s {
	void	*ptr;
	size_t	usize;
};

struct quarantine_s {
	size_t			curbytes;
	size_t			curobjs;
	size_t			first;
#define	LG_MAXOBJS_INIT 10
	size_t			lg_maxobjs;
	quarantine_obj_t	objs[1]; 
};

static void	quarantine_cleanup(void *arg);

malloc_tsd_data(static, quarantine, quarantine_t *, NULL)
malloc_tsd_funcs(JEMALLOC_INLINE, quarantine, quarantine_t *, NULL,
    quarantine_cleanup)




static quarantine_t	*quarantine_init(size_t lg_maxobjs);
static quarantine_t	*quarantine_grow(quarantine_t *quarantine);
static void	quarantine_drain(quarantine_t *quarantine, size_t upper_bound);



static quarantine_t *
quarantine_init(size_t lg_maxobjs)
{
	quarantine_t *quarantine;

	quarantine = (quarantine_t *)imalloc(offsetof(quarantine_t, objs) +
	    ((ZU(1) << lg_maxobjs) * sizeof(quarantine_obj_t)));
	if (quarantine == NULL)
		return (NULL);
	quarantine->curbytes = 0;
	quarantine->curobjs = 0;
	quarantine->first = 0;
	quarantine->lg_maxobjs = lg_maxobjs;

	quarantine_tsd_set(&quarantine);

	return (quarantine);
}

static quarantine_t *
quarantine_grow(quarantine_t *quarantine)
{
	quarantine_t *ret;

	ret = quarantine_init(quarantine->lg_maxobjs + 1);
	if (ret == NULL)
		return (quarantine);

	ret->curbytes = quarantine->curbytes;
	ret->curobjs = quarantine->curobjs;
	if (quarantine->first + quarantine->curobjs <= (ZU(1) <<
	    quarantine->lg_maxobjs)) {
		
		memcpy(ret->objs, &quarantine->objs[quarantine->first],
		    quarantine->curobjs * sizeof(quarantine_obj_t));
	} else {
		
		size_t ncopy_a = (ZU(1) << quarantine->lg_maxobjs) -
		    quarantine->first;
		size_t ncopy_b = quarantine->curobjs - ncopy_a;

		memcpy(ret->objs, &quarantine->objs[quarantine->first], ncopy_a
		    * sizeof(quarantine_obj_t));
		memcpy(&ret->objs[ncopy_a], quarantine->objs, ncopy_b *
		    sizeof(quarantine_obj_t));
	}

	return (ret);
}

static void
quarantine_drain(quarantine_t *quarantine, size_t upper_bound)
{

	while (quarantine->curbytes > upper_bound && quarantine->curobjs > 0) {
		quarantine_obj_t *obj = &quarantine->objs[quarantine->first];
		assert(obj->usize == isalloc(obj->ptr, config_prof));
		idalloc(obj->ptr);
		quarantine->curbytes -= obj->usize;
		quarantine->curobjs--;
		quarantine->first = (quarantine->first + 1) & ((ZU(1) <<
		    quarantine->lg_maxobjs) - 1);
	}
}

void
quarantine(void *ptr)
{
	quarantine_t *quarantine;
	size_t usize = isalloc(ptr, config_prof);

	cassert(config_fill);
	assert(opt_quarantine);

	quarantine = *quarantine_tsd_get();
	if ((uintptr_t)quarantine <= (uintptr_t)QUARANTINE_STATE_MAX) {
		if (quarantine == NULL) {
			if ((quarantine = quarantine_init(LG_MAXOBJS_INIT)) ==
			    NULL) {
				idalloc(ptr);
				return;
			}
		} else {
			if (quarantine == QUARANTINE_STATE_PURGATORY) {
				



				quarantine = QUARANTINE_STATE_REINCARNATED;
				quarantine_tsd_set(&quarantine);
			}
			idalloc(ptr);
			return;
		}
	}
	



	if (quarantine->curbytes + usize > opt_quarantine) {
		size_t upper_bound = (opt_quarantine >= usize) ? opt_quarantine
		    - usize : 0;
		quarantine_drain(quarantine, upper_bound);
	}
	
	if (quarantine->curobjs == (ZU(1) << quarantine->lg_maxobjs))
		quarantine = quarantine_grow(quarantine);
	
	assert(quarantine->curobjs < (ZU(1) << quarantine->lg_maxobjs));
	
	if (quarantine->curbytes + usize <= opt_quarantine) {
		size_t offset = (quarantine->first + quarantine->curobjs) &
		    ((ZU(1) << quarantine->lg_maxobjs) - 1);
		quarantine_obj_t *obj = &quarantine->objs[offset];
		obj->ptr = ptr;
		obj->usize = usize;
		quarantine->curbytes += usize;
		quarantine->curobjs++;
		if (opt_junk)
			memset(ptr, 0x5a, usize);
	} else {
		assert(quarantine->curbytes == 0);
		idalloc(ptr);
	}
}

static void
quarantine_cleanup(void *arg)
{
	quarantine_t *quarantine = *(quarantine_t **)arg;

	if (quarantine == QUARANTINE_STATE_REINCARNATED) {
		




		quarantine = QUARANTINE_STATE_PURGATORY;
		quarantine_tsd_set(&quarantine);
	} else if (quarantine == QUARANTINE_STATE_PURGATORY) {
		





	} else if (quarantine != NULL) {
		quarantine_drain(quarantine, 0);
		idalloc(quarantine);
		quarantine = QUARANTINE_STATE_PURGATORY;
		quarantine_tsd_set(&quarantine);
	}
}

bool
quarantine_boot(void)
{

	cassert(config_fill);

	if (quarantine_tsd_boot())
		return (true);

	return (false);
}
