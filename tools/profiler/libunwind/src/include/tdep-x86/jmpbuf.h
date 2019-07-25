


























#if defined __linux__

#define JB_SP		4
#define JB_RP		5
#define JB_MASK_SAVED	6
#define JB_MASK		7

#elif defined __FreeBSD__

#define JB_SP		2
#define JB_RP		0
#define JB_MASK_SAVED	11
#define JB_MASK		7

#endif
