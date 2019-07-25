
























#if defined __linux__



#define JB_SP		6
#define JB_RP		7
#define JB_MASK_SAVED	8
#define JB_MASK		9

#elif defined __FreeBSD__

#define JB_SP		2
#define JB_RP		0

#define JB_MASK_SAVED	0
#define JB_MASK		9

#endif
