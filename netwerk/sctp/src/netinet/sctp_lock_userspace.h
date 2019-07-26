
































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#endif

#ifndef _NETINET_SCTP_LOCK_EMPTY_H_
#define _NETINET_SCTP_LOCK_EMPTY_H_








#define SCTP_IPI_COUNT_INIT()

#define SCTP_STATLOG_INIT_LOCK()
#define SCTP_STATLOG_LOCK()
#define SCTP_STATLOG_UNLOCK()
#define SCTP_STATLOG_DESTROY()

#define SCTP_INP_INFO_LOCK_DESTROY()

#define SCTP_INP_INFO_LOCK_INIT()
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_WLOCK()
#define SCTP_INP_INFO_TRYLOCK() 1
#define SCTP_INP_INFO_RUNLOCK()
#define SCTP_INP_INFO_WUNLOCK()

#define SCTP_WQ_ADDR_INIT()
#define SCTP_WQ_ADDR_DESTROY()
#define SCTP_WQ_ADDR_LOCK()
#define SCTP_WQ_ADDR_UNLOCK()


#define SCTP_IPI_ADDR_INIT()
#define SCTP_IPI_ADDR_DESTROY()
#define SCTP_IPI_ADDR_RLOCK()
#define SCTP_IPI_ADDR_WLOCK()
#define SCTP_IPI_ADDR_RUNLOCK()
#define SCTP_IPI_ADDR_WUNLOCK()

#define SCTP_IPI_ITERATOR_WQ_INIT()
#define SCTP_IPI_ITERATOR_WQ_DESTROY()
#define SCTP_IPI_ITERATOR_WQ_LOCK()
#define SCTP_IPI_ITERATOR_WQ_UNLOCK()


#define SCTP_IP_PKTLOG_INIT()
#define SCTP_IP_PKTLOG_LOCK()
#define SCTP_IP_PKTLOG_UNLOCK()
#define SCTP_IP_PKTLOG_DESTROY()



#define SCTP_INP_READ_INIT(_inp)
#define SCTP_INP_READ_DESTROY(_inp)
#define SCTP_INP_READ_LOCK(_inp)
#define SCTP_INP_READ_UNLOCK(_inp)

#define SCTP_INP_LOCK_INIT(_inp)
#define SCTP_ASOC_CREATE_LOCK_INIT(_inp)
#define SCTP_INP_LOCK_DESTROY(_inp)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)


#define SCTP_INP_RLOCK(_inp)
#define SCTP_INP_WLOCK(_inp)

#define SCTP_INP_LOCK_CONTENDED(_inp) (0) /* Don't know if this is possible */

#define SCTP_INP_READ_CONTENDED(_inp) (0) /* Don't know if this is possible */

#define SCTP_ASOC_CREATE_LOCK_CONTENDED(_inp) (0) /* Don't know if this is possible */


#define SCTP_TCB_SEND_LOCK_INIT(_tcb)
#define SCTP_TCB_SEND_LOCK_DESTROY(_tcb)
#define SCTP_TCB_SEND_LOCK(_tcb)
#define SCTP_TCB_SEND_UNLOCK(_tcb)

#define SCTP_INP_INCR_REF(_inp)
#define SCTP_INP_DECR_REF(_inp)

#define SCTP_ASOC_CREATE_LOCK(_inp)

#define SCTP_INP_RUNLOCK(_inp)
#define SCTP_INP_WUNLOCK(_inp)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)


#define SCTP_TCB_LOCK_INIT(_tcb)
#define SCTP_TCB_LOCK_DESTROY(_tcb)
#define SCTP_TCB_LOCK(_tcb)
#define SCTP_TCB_TRYLOCK(_tcb) 1
#define SCTP_TCB_UNLOCK(_tcb)
#define SCTP_TCB_UNLOCK_IFOWNED(_tcb)
#define SCTP_TCB_LOCK_ASSERT(_tcb)



#define SCTP_ITERATOR_LOCK_INIT()
#define SCTP_ITERATOR_LOCK()
#define SCTP_ITERATOR_UNLOCK()
#define SCTP_ITERATOR_LOCK_DESTROY()



#define SCTP_INCR_EP_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_ep++; \
	        } while (0)

#define SCTP_DECR_EP_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_ep--; \
	        } while (0)

#define SCTP_INCR_ASOC_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_asoc++; \
	        } while (0)

#define SCTP_DECR_ASOC_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_asoc--; \
	        } while (0)

#define SCTP_INCR_LADDR_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_laddr++; \
	        } while (0)

#define SCTP_DECR_LADDR_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_laddr--; \
	        } while (0)

#define SCTP_INCR_RADDR_COUNT() \
                do { \
 	               sctppcbinfo.ipi_count_raddr++; \
	        } while (0)

#define SCTP_DECR_RADDR_COUNT() \
                do { \
 	               sctppcbinfo.ipi_count_raddr--; \
	        } while (0)

#define SCTP_INCR_CHK_COUNT() \
                do { \
  	               sctppcbinfo.ipi_count_chunk++; \
	        } while (0)

#define SCTP_DECR_CHK_COUNT() \
                do { \
  	               sctppcbinfo.ipi_count_chunk--; \
	        } while (0)

#define SCTP_INCR_READQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_readq++; \
	        } while (0)

#define SCTP_DECR_READQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_readq--; \
	        } while (0)

#define SCTP_INCR_STRMOQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_strmoq++; \
	        } while (0)

#define SCTP_DECR_STRMOQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_strmoq--; \
	        } while (0)



#if defined(SCTP_SO_LOCK_TESTING)
#define SCTP_INP_SO(sctpinp)	(sctpinp)->ip_inp.inp.inp_socket
#define SCTP_SOCKET_LOCK(so, refcnt)
#define SCTP_SOCKET_UNLOCK(so, refcnt)
#endif



#if 0
#define SCTP_IPI_ADDR_LOCK()
#define SCTP_IPI_ADDR_UNLOCK()
#endif















#if 1
#define SOCK_LOCK(_so)
#define SOCK_UNLOCK(_so)
#define SOCKBUF_LOCK(_so_buf)
#define SOCKBUF_UNLOCK(_so_buf)
#define SOCKBUF_LOCK_ASSERT(_so_buf)
#endif

#endif
