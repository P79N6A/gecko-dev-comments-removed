






























#ifndef __sctp_process_lock_h__
#define __sctp_process_lock_h__















#ifdef SCTP_PER_SOCKET_LOCKING




#if defined(__Userspace_os_Windows)

#define SCTP_INP_INFO_LOCK_INIT()
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_RUNLOCK()
#define SCTP_INP_INFO_WLOCK()
#define SCTP_INP_INFO_WUNLOCK()
#define SCTP_INP_INFO_LOCK_DESTROY()
#define SCTP_IPI_COUNT_INIT()
#define SCTP_IPI_COUNT_DESTROY()
#else
#define SCTP_INP_INFO_LOCK_INIT()
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_RUNLOCK()
#define SCTP_INP_INFO_WLOCK()
#define SCTP_INP_INFO_WUNLOCK()
#define SCTP_INP_INFO_LOCK_DESTROY()
#define SCTP_IPI_COUNT_INIT()
#define SCTP_IPI_COUNT_DESTROY()
#endif

#define SCTP_TCB_SEND_LOCK_INIT(_tcb)
#define SCTP_TCB_SEND_LOCK_DESTROY(_tcb)
#define SCTP_TCB_SEND_LOCK(_tcb)
#define SCTP_TCB_SEND_UNLOCK(_tcb)


#define SCTP_INP_LOCK_INIT(_inp)
#define SCTP_INP_LOCK_DESTROY(_inp)

#define SCTP_INP_RLOCK(_inp)
#define SCTP_INP_RUNLOCK(_inp)
#define SCTP_INP_WLOCK(_inp)
#define SCTP_INP_WUNLOCK(_inep)
#define SCTP_INP_INCR_REF(_inp)
#define SCTP_INP_DECR_REF(_inp)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)
#define SCTP_ASOC_CREATE_LOCK(_inp)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)

#define SCTP_INP_READ_INIT(_inp)
#define SCTP_INP_READ_DESTROY(_inp)
#define SCTP_INP_READ_LOCK(_inp)
#define SCTP_INP_READ_UNLOCK(_inp)


#define SCTP_TCB_LOCK_INIT(_tcb)
#define SCTP_TCB_LOCK_DESTROY(_tcb)
#define SCTP_TCB_LOCK(_tcb)
#define SCTP_TCB_TRYLOCK(_tcb) 1
#define SCTP_TCB_UNLOCK(_tcb)
#define SCTP_TCB_UNLOCK_IFOWNED(_tcb)
#define SCTP_TCB_LOCK_ASSERT(_tcb)

#else



#define SCTP_IPI_COUNT_INIT()

#if defined(__Userspace_os_Windows)
#define SCTP_WQ_ADDR_INIT() \
        InitializeCriticalSection(&SCTP_BASE_INFO(wq_addr_mtx))
#define SCTP_WQ_ADDR_DESTROY() \
	DeleteCriticalSection(&SCTP_BASE_INFO(wq_addr_mtx))
#define SCTP_WQ_ADDR_LOCK() \
        EnterCriticalSection(&SCTP_BASE_INFO(wq_addr_mtx))
#define SCTP_WQ_ADDR_UNLOCK() \
        LeaveCriticalSection(&SCTP_BASE_INFO(wq_addr_mtx))


#define SCTP_INP_INFO_LOCK_INIT() \
	InitializeCriticalSection(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_RLOCK() \
	EnterCriticalSection(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_TRYLOCK()	\
        TryEnterCriticalSection(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_WLOCK() \
	EnterCriticalSection(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_RUNLOCK() \
 	LeaveCriticalSection(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_WUNLOCK()	\
	LeaveCriticalSection(&SCTP_BASE_INFO(ipi_ep_mtx))

#define SCTP_IP_PKTLOG_INIT() \
        InitializeCriticalSection(&SCTP_BASE_INFO(ipi_pktlog_mtx))
#define SCTP_IP_PKTLOG_DESTROY () \
	DeleteCriticalSection(&SCTP_BASE_INFO(ipi_pktlog_mtx))
#define SCTP_IP_PKTLOG_LOCK() \
    	EnterCriticalSection(&SCTP_BASE_INFO(ipi_pktlog_mtx))
#define SCTP_IP_PKTLOG_UNLOCK() \
	LeaveCriticalSection(&SCTP_BASE_INFO(ipi_pktlog_mtx))






#define SCTP_INP_READ_INIT(_inp) \
	InitializeCriticalSection(&(_inp)->inp_rdata_mtx)
#define SCTP_INP_READ_DESTROY(_inp) \
	DeleteCriticalSection(&(_inp)->inp_rdata_mtx)
#define SCTP_INP_READ_LOCK(_inp) \
	EnterCriticalSection(&(_inp)->inp_rdata_mtx)
#define SCTP_INP_READ_UNLOCK(_inp) \
	LeaveCriticalSection(&(_inp)->inp_rdata_mtx)

#define SCTP_INP_LOCK_INIT(_inp) \
	InitializeCriticalSection(&(_inp)->inp_mtx)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp) \
	InitializeCriticalSection(&(_inp)->inp_create_mtx)

#define SCTP_INP_LOCK_DESTROY(_inp) \
	DeleteCriticalSection(&(_inp)->inp_mtx)

#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp) \
	DeleteCriticalSection(&(_inp)->inp_create_mtx)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_INP_RLOCK(_inp)	do { 					\
	if(SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOCK_LOGGING_ENABLE) sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
	EnterCriticalSection(&(_inp)->inp_mtx);			\
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
	EnterCriticalSection(&(_inp)->inp_mtx);			\
} while (0)
#else

#define SCTP_INP_RLOCK(_inp)	do { 					\
	EnterCriticalSection(&(_inp)->inp_mtx);			\
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	EnterCriticalSection(&(_inp)->inp_mtx);			\
} while (0)
#endif


#define SCTP_TCB_SEND_LOCK_INIT(_tcb) \
	InitializeCriticalSection(&(_tcb)->tcb_send_mtx)

#define SCTP_TCB_SEND_LOCK_DESTROY(_tcb) \
	DeleteCriticalSection(&(_tcb)->tcb_send_mtx)

#define SCTP_TCB_SEND_LOCK(_tcb) do { \
	EnterCriticalSection(&(_tcb)->tcb_send_mtx); \
} while (0)

#define SCTP_TCB_SEND_UNLOCK(_tcb) \
	LeaveCriticalSection(&(_tcb)->tcb_send_mtx)

#define SCTP_INP_INCR_REF(_inp) atomic_add_int(&((_inp)->refcount), 1)
#define SCTP_INP_DECR_REF(_inp) atomic_add_int(&((_inp)->refcount), -1)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_ASOC_CREATE_LOCK(_inp) do {				\
	if(SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOCK_LOGGING_ENABLE) sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_CREATE); \
	EnterCriticalSection(&(_inp)->inp_create_mtx);		\
} while (0)
#else
#define SCTP_ASOC_CREATE_LOCK(_inp) do {				\
	EnterCriticalSection(&(_inp)->inp_create_mtx);		\
} while (0)
#endif

#define SCTP_INP_RUNLOCK(_inp) \
	LeaveCriticalSection(&(_inp)->inp_mtx)
#define SCTP_INP_WUNLOCK(_inp) \
	LeaveCriticalSection(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_UNLOCK(_inp) \
	LeaveCriticalSection(&(_inp)->inp_create_mtx)









#define SCTP_TCB_LOCK_INIT(_tcb) \
	InitializeCriticalSection(&(_tcb)->tcb_mtx)

#define SCTP_TCB_LOCK_DESTROY(_tcb) \
	DeleteCriticalSection(&(_tcb)->tcb_mtx)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_TCB_LOCK(_tcb)  do {					\
	if(SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOCK_LOGGING_ENABLE) sctp_log_lock(_tcb->sctp_ep, _tcb, SCTP_LOG_LOCK_TCB);		\
	EnterCriticalSection(&(_tcb)->tcb_mtx);			\
} while (0)

#else
#define SCTP_TCB_LOCK(_tcb)  do {					\
	EnterCriticalSection(&(_tcb)->tcb_mtx);			\
} while (0)
#endif

#define SCTP_TCB_TRYLOCK(_tcb) 	((TryEnterCriticalSection(&(_tcb)->tcb_mtx)))

#define SCTP_TCB_UNLOCK(_tcb)	do {  \
	LeaveCriticalSection(&(_tcb)->tcb_mtx);  \
} while (0)

#define SCTP_TCB_LOCK_ASSERT(_tcb)

#else 
#define SCTP_WQ_ADDR_INIT() \
        (void)pthread_mutex_init(&SCTP_BASE_INFO(wq_addr_mtx), NULL)
#define SCTP_WQ_ADDR_DESTROY() \
	(void)pthread_mutex_destroy(&SCTP_BASE_INFO(wq_addr_mtx))
#define SCTP_WQ_ADDR_LOCK() \
        (void)pthread_mutex_lock(&SCTP_BASE_INFO(wq_addr_mtx))
#define SCTP_WQ_ADDR_UNLOCK() \
        (void)pthread_mutex_unlock(&SCTP_BASE_INFO(wq_addr_mtx))


#define SCTP_INP_INFO_LOCK_INIT() \
	(void)pthread_mutex_init(&SCTP_BASE_INFO(ipi_ep_mtx), NULL)
#define SCTP_INP_INFO_RLOCK() \
	(void)pthread_mutex_lock(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_TRYLOCK()	\
        (!(pthread_mutex_trylock(&SCTP_BASE_INFO(ipi_ep_mtx))))
#define SCTP_INP_INFO_WLOCK() \
	(void)pthread_mutex_lock(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_RUNLOCK()	\
	(void)pthread_mutex_unlock(&SCTP_BASE_INFO(ipi_ep_mtx))
#define SCTP_INP_INFO_WUNLOCK()	\
	(void)pthread_mutex_unlock(&SCTP_BASE_INFO(ipi_ep_mtx))

#define SCTP_IP_PKTLOG_INIT() \
        (void)pthread_mutex_init(&SCTP_BASE_INFO(ipi_pktlog_mtx), NULL)
#define SCTP_IP_PKTLOG_DESTROY() \
	(void)pthread_mutex_destroy(&SCTP_BASE_INFO(ipi_pktlog_mtx))
#define SCTP_IP_PKTLOG_LOCK() \
        (void)pthread_mutex_lock(&SCTP_BASE_INFO(ipi_pktlog_mtx))
#define SCTP_IP_PKTLOG_UNLOCK()	\
        (void)pthread_mutex_unlock(&SCTP_BASE_INFO(ipi_pktlog_mtx))








#define SCTP_INP_READ_INIT(_inp) \
	(void)pthread_mutex_init(&(_inp)->inp_rdata_mtx, NULL)

#define SCTP_INP_READ_DESTROY(_inp) \
	(void)pthread_mutex_destroy(&(_inp)->inp_rdata_mtx)

#define SCTP_INP_READ_LOCK(_inp)	do { \
	(void)pthread_mutex_lock(&(_inp)->inp_rdata_mtx);    \
} while (0)


#define SCTP_INP_READ_UNLOCK(_inp) \
	(void)pthread_mutex_unlock(&(_inp)->inp_rdata_mtx)

#define SCTP_INP_LOCK_INIT(_inp) \
	(void)pthread_mutex_init(&(_inp)->inp_mtx, NULL)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp) \
	(void)pthread_mutex_init(&(_inp)->inp_create_mtx, NULL)

#define SCTP_INP_LOCK_DESTROY(_inp) \
	(void)pthread_mutex_destroy(&(_inp)->inp_mtx)

#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp) \
	(void)pthread_mutex_destroy(&(_inp)->inp_create_mtx)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_INP_RLOCK(_inp)	do { 					\
	if(SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOCK_LOGGING_ENABLE) sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
	(void)pthread_mutex_lock(&(_inp)->inp_mtx);			\
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
	(void)pthread_mutex_lock(&(_inp)->inp_mtx);			\
} while (0)

#else

#define SCTP_INP_RLOCK(_inp)	do { 					\
	(void)pthread_mutex_lock(&(_inp)->inp_mtx);			\
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	(void)pthread_mutex_lock(&(_inp)->inp_mtx);			\
} while (0)
#endif


#define SCTP_TCB_SEND_LOCK_INIT(_tcb) \
	(void)pthread_mutex_init(&(_tcb)->tcb_send_mtx, NULL)

#define SCTP_TCB_SEND_LOCK_DESTROY(_tcb) \
	(void)pthread_mutex_destroy(&(_tcb)->tcb_send_mtx)

#define SCTP_TCB_SEND_LOCK(_tcb) do { \
	(void)pthread_mutex_lock(&(_tcb)->tcb_send_mtx); \
} while (0)

#define SCTP_TCB_SEND_UNLOCK(_tcb) \
	(void)pthread_mutex_unlock(&(_tcb)->tcb_send_mtx)

#define SCTP_INP_INCR_REF(_inp) atomic_add_int(&((_inp)->refcount), 1)
#define SCTP_INP_DECR_REF(_inp) atomic_add_int(&((_inp)->refcount), -1)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_ASOC_CREATE_LOCK(_inp) do {				\
	if(SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOCK_LOGGING_ENABLE) sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_CREATE); \
	(void)pthread_mutex_lock(&(_inp)->inp_create_mtx);		\
} while (0)
#else
#define SCTP_ASOC_CREATE_LOCK(_inp) do {				\
	(void)pthread_mutex_lock(&(_inp)->inp_create_mtx);		\
} while (0)
#endif

#define SCTP_INP_RUNLOCK(_inp) \
	(void)pthread_mutex_unlock(&(_inp)->inp_mtx)
#define SCTP_INP_WUNLOCK(_inp) \
	(void)pthread_mutex_unlock(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_UNLOCK(_inp) \
	(void)pthread_mutex_unlock(&(_inp)->inp_create_mtx)









#define SCTP_TCB_LOCK_INIT(_tcb) \
	(void)pthread_mutex_init(&(_tcb)->tcb_mtx, NULL)

#define SCTP_TCB_LOCK_DESTROY(_tcb) \
	(void)pthread_mutex_destroy(&(_tcb)->tcb_mtx)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_TCB_LOCK(_tcb)  do {					\
	if(SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOCK_LOGGING_ENABLE) sctp_log_lock(_tcb->sctp_ep, _tcb, SCTP_LOG_LOCK_TCB);		\
	(void)pthread_mutex_lock(&(_tcb)->tcb_mtx);			\
} while (0)

#else
#define SCTP_TCB_LOCK(_tcb)  do {					\
	(void)pthread_mutex_lock(&(_tcb)->tcb_mtx);			\
} while (0)
#endif

#define SCTP_TCB_TRYLOCK(_tcb) 	(!(pthread_mutex_trylock(&(_tcb)->tcb_mtx)))

#define SCTP_TCB_UNLOCK(_tcb)	(void)pthread_mutex_unlock(&(_tcb)->tcb_mtx)

#define SCTP_TCB_LOCK_ASSERT(_tcb)
#endif

#endif 







#define SCTP_INP_LOCK_CONTENDED(_inp) (0) /* Don't know if this is possible */
#define SCTP_INP_READ_CONTENDED(_inp) (0) /* Don't know if this is possible */
#define SCTP_ASOC_CREATE_LOCK_CONTENDED(_inp) (0) /* Don't know if this is possible */




#if defined(__Userspace__)
#if defined(__Userspace_os_Windows)
#define SOCKBUF_LOCK_ASSERT(_so_buf)
#define SOCKBUF_LOCK(_so_buf) EnterCriticalSection(&(_so_buf)->sb_mtx)
#define SOCKBUF_UNLOCK(_so_buf) LeaveCriticalSection(&(_so_buf)->sb_mtx)
#define SOCK_LOCK(_so)  SOCKBUF_LOCK(&(_so)->so_rcv)
#define SOCK_UNLOCK(_so)  SOCKBUF_UNLOCK(&(_so)->so_rcv)
#else
#define SOCKBUF_LOCK_ASSERT(_so_buf) KASSERT(pthread_mutex_trylock(SOCKBUF_MTX(_so_buf)) == EBUSY, ("%s: socket buffer not locked", __func__))
#define SOCKBUF_LOCK(_so_buf)   pthread_mutex_lock(SOCKBUF_MTX(_so_buf))
#define SOCKBUF_UNLOCK(_so_buf) pthread_mutex_unlock(SOCKBUF_MTX(_so_buf))
#define	SOCK_LOCK(_so)		SOCKBUF_LOCK(&(_so)->so_rcv)
#define	SOCK_UNLOCK(_so)	SOCKBUF_UNLOCK(&(_so)->so_rcv)
#endif
#else
#define SOCK_LOCK(_so)
#define SOCK_UNLOCK(_so)
#define SOCKBUF_LOCK(_so_buf)
#define SOCKBUF_UNLOCK(_so_buf)
#define SOCKBUF_LOCK_ASSERT(_so_buf)
#endif

#define SCTP_STATLOG_INIT_LOCK()
#define SCTP_STATLOG_LOCK()
#define SCTP_STATLOG_UNLOCK()
#define SCTP_STATLOG_DESTROY()

#if defined(__Userspace_os_Windows)

#define SCTP_IPI_ADDR_INIT() \
	InitializeCriticalSection(&SCTP_BASE_INFO(ipi_addr_mtx))
#define SCTP_IPI_ADDR_DESTROY() \
	DeleteCriticalSection(&SCTP_BASE_INFO(ipi_addr_mtx))

#define SCTP_IPI_ADDR_RLOCK() 						\
	do { 								\
		EnterCriticalSection(&SCTP_BASE_INFO(ipi_addr_mtx));	\
	} while (0)
#define SCTP_IPI_ADDR_RUNLOCK() \
	LeaveCriticalSection(&SCTP_BASE_INFO(ipi_addr_mtx))

#define SCTP_IPI_ADDR_WLOCK() 						\
	do { 								\
		EnterCriticalSection(&SCTP_BASE_INFO(ipi_addr_mtx));	\
	} while (0)
#define SCTP_IPI_ADDR_WUNLOCK() \
	LeaveCriticalSection(&SCTP_BASE_INFO(ipi_addr_mtx))



#define SCTP_ITERATOR_LOCK_INIT() \
	InitializeCriticalSection(&sctp_it_ctl.it_mtx)

#define SCTP_ITERATOR_LOCK() 						\
	do {								\
		EnterCriticalSection(&sctp_it_ctl.it_mtx);		\
	} while (0)

#define SCTP_ITERATOR_UNLOCK() \
	LeaveCriticalSection(&sctp_it_ctl.it_mtx)

#define SCTP_ITERATOR_LOCK_DESTROY() \
	DeleteCriticalSection(&sctp_it_ctl.it_mtx)


#define SCTP_IPI_ITERATOR_WQ_INIT() \
	InitializeCriticalSection(&sctp_it_ctl.ipi_iterator_wq_mtx)

#define SCTP_IPI_ITERATOR_WQ_DESTROY() \
	DeleteCriticalSection(&sctp_it_ctl.ipi_iterator_wq_mtx)

#define SCTP_IPI_ITERATOR_WQ_LOCK() \
	do { \
		EnterCriticalSection(&sctp_it_ctl.ipi_iterator_wq_mtx); \
	} while (0)

#define SCTP_IPI_ITERATOR_WQ_UNLOCK() \
	LeaveCriticalSection(&sctp_it_ctl.ipi_iterator_wq_mtx)

#else 

#define SCTP_IPI_ADDR_INIT() \
	(void)pthread_mutex_init(&SCTP_BASE_INFO(ipi_addr_mtx), NULL)
#define SCTP_IPI_ADDR_DESTROY() \
	(void)pthread_mutex_destroy(&SCTP_BASE_INFO(ipi_addr_mtx))

#define SCTP_IPI_ADDR_RLOCK() 						\
	do { 								\
		(void)pthread_mutex_lock(&SCTP_BASE_INFO(ipi_addr_mtx));	\
	} while (0)
#define SCTP_IPI_ADDR_RUNLOCK() \
	(void)pthread_mutex_unlock(&SCTP_BASE_INFO(ipi_addr_mtx))

#define SCTP_IPI_ADDR_WLOCK() 						\
	do { 								\
		(void)pthread_mutex_lock(&SCTP_BASE_INFO(ipi_addr_mtx));	\
	} while (0)
#define SCTP_IPI_ADDR_WUNLOCK() \
	(void)pthread_mutex_unlock(&SCTP_BASE_INFO(ipi_addr_mtx))



#define SCTP_ITERATOR_LOCK_INIT() \
	(void)pthread_mutex_init(&sctp_it_ctl.it_mtx, NULL)

#define SCTP_ITERATOR_LOCK() 						\
	do {								\
		(void)pthread_mutex_lock(&sctp_it_ctl.it_mtx);		\
	} while (0)

#define SCTP_ITERATOR_UNLOCK() \
	(void)pthread_mutex_unlock(&sctp_it_ctl.it_mtx)

#define SCTP_ITERATOR_LOCK_DESTROY() \
	(void)pthread_mutex_destroy(&sctp_it_ctl.it_mtx)


#define SCTP_IPI_ITERATOR_WQ_INIT() \
	(void)pthread_mutex_init(&sctp_it_ctl.ipi_iterator_wq_mtx, NULL)

#define SCTP_IPI_ITERATOR_WQ_DESTROY() \
	(void)pthread_mutex_destroy(&sctp_it_ctl.ipi_iterator_wq_mtx)

#define SCTP_IPI_ITERATOR_WQ_LOCK() \
	do { \
		(void)pthread_mutex_lock(&sctp_it_ctl.ipi_iterator_wq_mtx); \
	} while (0)

#define SCTP_IPI_ITERATOR_WQ_UNLOCK() \
	(void)pthread_mutex_unlock(&sctp_it_ctl.ipi_iterator_wq_mtx)
#endif

#define SCTP_INCR_EP_COUNT() \
	do { \
		atomic_add_int(&SCTP_BASE_INFO(ipi_count_ep), 1); \
	} while (0)

#define SCTP_DECR_EP_COUNT() \
	do { \
	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_ep), 1); \
	} while (0)

#define SCTP_INCR_ASOC_COUNT() \
	do { \
	       atomic_add_int(&SCTP_BASE_INFO(ipi_count_asoc), 1); \
	} while (0)

#define SCTP_DECR_ASOC_COUNT() \
	do { \
	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_asoc), 1); \
	} while (0)

#define SCTP_INCR_LADDR_COUNT() \
	do { \
	       atomic_add_int(&SCTP_BASE_INFO(ipi_count_laddr), 1); \
	} while (0)

#define SCTP_DECR_LADDR_COUNT() \
	do { \
	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_laddr), 1); \
	} while (0)

#define SCTP_INCR_RADDR_COUNT() \
	do { \
 	       atomic_add_int(&SCTP_BASE_INFO(ipi_count_raddr), 1); \
	} while (0)

#define SCTP_DECR_RADDR_COUNT() \
	do { \
 	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_raddr), 1); \
	} while (0)

#define SCTP_INCR_CHK_COUNT() \
	do { \
  	       atomic_add_int(&SCTP_BASE_INFO(ipi_count_chunk), 1); \
	} while (0)

#define SCTP_DECR_CHK_COUNT() \
	do { \
  	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_chunk), 1); \
	} while (0)

#define SCTP_INCR_READQ_COUNT() \
	do { \
	       atomic_add_int(&SCTP_BASE_INFO(ipi_count_readq), 1); \
	} while (0)

#define SCTP_DECR_READQ_COUNT() \
	do { \
	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_readq), 1); \
	} while (0)

#define SCTP_INCR_STRMOQ_COUNT() \
	do { \
	       atomic_add_int(&SCTP_BASE_INFO(ipi_count_strmoq), 1); \
	} while (0)

#define SCTP_DECR_STRMOQ_COUNT() \
	do { \
	       atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_strmoq), 1); \
	} while (0)

#endif
