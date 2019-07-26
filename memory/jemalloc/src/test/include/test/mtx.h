






typedef struct {
#ifdef _WIN32
	CRITICAL_SECTION	lock;
#elif (defined(JEMALLOC_OSSPIN))
	OSSpinLock		lock;
#else
	pthread_mutex_t		lock;
#endif
} mtx_t;

bool	mtx_init(mtx_t *mtx);
void	mtx_fini(mtx_t *mtx);
void	mtx_lock(mtx_t *mtx);
void	mtx_unlock(mtx_t *mtx);
