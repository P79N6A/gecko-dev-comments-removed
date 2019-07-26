

























#ifndef _EVENT2_EVENT_KEYVALQ_STRUCT_H_
#define _EVENT2_EVENT_KEYVALQ_STRUCT_H_

#ifdef __cplusplus
extern "C" {
#endif



#ifndef TAILQ_ENTRY
#define _EVENT_DEFINED_TQENTRY
#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}
#endif 

#ifndef TAILQ_HEAD
#define _EVENT_DEFINED_TQHEAD
#define TAILQ_HEAD(name, type)			\
struct name {					\
	struct type *tqh_first;			\
	struct type **tqh_last;			\
}
#endif





struct evkeyval {
	TAILQ_ENTRY(evkeyval) next;

	char *key;
	char *value;
};

TAILQ_HEAD (evkeyvalq, evkeyval);


#ifdef _EVENT_DEFINED_TQENTRY
#undef TAILQ_ENTRY
#endif

#ifdef _EVENT_DEFINED_TQHEAD
#undef TAILQ_HEAD
#endif

#ifdef __cplusplus
}
#endif

#endif
