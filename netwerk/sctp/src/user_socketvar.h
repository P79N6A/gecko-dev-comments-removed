































#ifndef _USER_SOCKETVAR_H_
#define _USER_SOCKETVAR_H_

#if defined(__Userspace_os_Darwin)
#include <sys/types.h>
#include <unistd.h>
#endif

 	


 
#if !defined(__Userspace_os_Windows) && !defined(__Userspace_os_FreeBSD)
#include <sys/uio.h>
#endif
#define SOCK_MAXADDRLEN 255
#if !defined(MSG_NOTIFICATION)
#define MSG_NOTIFICATION 0x2000         /* SCTP notification */
#endif
#define SCTP_SO_LINGER     0x0001
#define SCTP_SO_ACCEPTCONN 0x0002
#define SS_CANTRCVMORE 0x020
#define SS_CANTSENDMORE 0x010

#if defined (__Userspace_os_FreeBSD) || defined(__Userspace_os_OpenBSD) || defined(__Userspace_os_Darwin) || defined (__Userspace_os_Windows)
#define UIO_MAXIOV 1024
#define ERESTART (-1)
#endif

#if !defined(__Userspace_os_Darwin) && !defined(__Userspace_os_OpenBSD)
enum	uio_rw { UIO_READ, UIO_WRITE };
#endif

#if !defined(__Userspace_os_OpenBSD)

enum uio_seg {
	UIO_USERSPACE,		
	UIO_SYSSPACE		
};
#endif

struct proc {
    int stub; 
};

MALLOC_DECLARE(M_ACCF);
MALLOC_DECLARE(M_PCB);
MALLOC_DECLARE(M_SONAME);




struct uio {
    struct	iovec *uio_iov;		
    int	        uio_iovcnt;		
    off_t	uio_offset;		
    int 	uio_resid;		
    enum	uio_seg uio_segflg;	
    enum	uio_rw uio_rw;		
};










#if defined (__Userspace_os_Windows)
#define AF_ROUTE  17
typedef __int32 u_quad_t;
typedef __int32 pid_t;
typedef unsigned __int32 uid_t;
enum sigType {
	SIGNAL = 0,
	BROADCAST = 1,
	MAX_EVENTS = 2
};
#endif
typedef	u_quad_t so_gen_t;












struct socket {
	int	so_count;		
	short	so_type;		
	short	so_options;		
	short	so_linger;		
	short	so_state;		
	int	so_qstate;		
	void	*so_pcb;		
	int	so_dom;











	struct	socket *so_head;	
	TAILQ_HEAD(, socket) so_incomp;	
	TAILQ_HEAD(, socket) so_comp;	
	TAILQ_ENTRY(socket) so_list;	
	u_short	so_qlen;		
	u_short	so_incqlen;		

	u_short	so_qlimit;		
	short	so_timeo;		
	userland_cond_t timeo_cond;      

	u_short	so_error;		
	struct	sigio *so_sigio;	

	u_long	so_oobmark;		
	TAILQ_HEAD(, aiocblist) so_aiojobq; 



	struct sockbuf {
		





		 
			
			
		userland_cond_t sb_cond; 
		userland_mutex_t sb_mtx; 
		short	sb_state;	
#define	sb_startzero	sb_mb
		struct	mbuf *sb_mb;	
		struct	mbuf *sb_mbtail; 
		struct	mbuf *sb_lastrecord;	

		struct	mbuf *sb_sndptr; 
		u_int	sb_sndptroff;	
		u_int	sb_cc;		
		u_int	sb_hiwat;	
		u_int	sb_mbcnt;	
		u_int	sb_mbmax;	
		u_int	sb_ctl;		
		int	sb_lowat;	
		int	sb_timeo;	
		short	sb_flags;	
	} so_rcv, so_snd;



#define	SB_MAX		(256*1024)	/* default for max chars in sockbuf */
#define SB_RAW          (64*1024*2)    /*Aligning so->so_rcv.sb_hiwat with the receive buffer size of raw socket*/



#define	SB_WAIT		0x04		/* someone is waiting for data/space */
#define	SB_SEL		0x08		/* someone is selecting */
#define	SB_ASYNC	0x10		/* ASYNC I/O, need signals */
#define	SB_UPCALL	0x20		/* someone wants an upcall */
#define	SB_NOINTR	0x40		/* operations not interruptible */
#define	SB_AIO		0x80		/* AIO operations queued */
#define	SB_KNOTE	0x100		/* kernel note attached */
#define	SB_AUTOSIZE	0x800		/* automatically size socket buffer */

	void	(*so_upcall)(struct socket *, void *, int);
	void	*so_upcallarg;
	struct	ucred *so_cred;		
	struct	label *so_label;	
	struct	label *so_peerlabel;	
	
	so_gen_t so_gencnt;		
	void	*so_emuldata;		
 	struct so_accf {
		struct	accept_filter *so_accept_filter;
		void	*so_accept_filter_arg;	
		char	*so_accept_filter_str;	
	} *so_accf;
};

#define SB_EMPTY_FIXUP(sb) do {						\
	if ((sb)->sb_mb == NULL) {					\
		(sb)->sb_mbtail = NULL;					\
		(sb)->sb_lastrecord = NULL;				\
	}								\
} while (/*CONSTCOND*/0)







#if defined(__Userspace_os_Windows)
extern userland_mutex_t accept_mtx;
extern userland_cond_t accept_cond;
#define ACCEPT_LOCK_ASSERT()
#define	ACCEPT_LOCK() do { \
	EnterCriticalSection(&accept_mtx); \
} while (0)
#define	ACCEPT_UNLOCK()	do { \
	LeaveCriticalSection(&accept_mtx); \
} while (0)
#define	ACCEPT_UNLOCK_ASSERT()
#else
extern userland_mutex_t accept_mtx;
#define	ACCEPT_LOCK_ASSERT()		KASSERT(pthread_mutex_trylock(&accept_mtx) == EBUSY, ("%s: accept_mtx not locked", __func__))
#define	ACCEPT_LOCK()			(void)pthread_mutex_lock(&accept_mtx)
#define	ACCEPT_UNLOCK()			(void)pthread_mutex_unlock(&accept_mtx)
#define	ACCEPT_UNLOCK_ASSERT()	 do{                                                            \
	KASSERT(pthread_mutex_trylock(&accept_mtx) == 0, ("%s: accept_mtx  locked", __func__)); \
	(void)pthread_mutex_unlock(&accept_mtx);                                                \
} while (0)
#endif





#define	SOCKBUF_MTX(_sb) (&(_sb)->sb_mtx)
#if defined (__Userspace_os_Windows)
#define SOCKBUF_LOCK_INIT(_sb, _name) \
	InitializeCriticalSection(SOCKBUF_MTX(_sb))
#define SOCKBUF_LOCK_DESTROY(_sb) DeleteCriticalSection(SOCKBUF_MTX(_sb))
#define SOCKBUF_COND_INIT(_sb) InitializeConditionVariable((&(_sb)->sb_cond))
#define SOCKBUF_COND_DESTROY(_sb) DeleteConditionVariable((&(_sb)->sb_cond))
#define SOCK_COND_INIT(_so) InitializeConditionVariable((&(_so)->timeo_cond))
#define SOCK_COND_DESTROY(_so) DeleteConditionVariable((&(_so)->timeo_cond))
#define SOCK_COND(_so) (&(_so)->timeo_cond)
#else
#define SOCKBUF_LOCK_INIT(_sb, _name) \
	pthread_mutex_init(SOCKBUF_MTX(_sb), NULL)
#define SOCKBUF_LOCK_DESTROY(_sb) pthread_mutex_destroy(SOCKBUF_MTX(_sb))
#define SOCKBUF_COND_INIT(_sb) pthread_cond_init((&(_sb)->sb_cond), NULL)
#define SOCKBUF_COND_DESTROY(_sb) pthread_cond_destroy((&(_sb)->sb_cond))
#define SOCK_COND_INIT(_so) pthread_cond_init((&(_so)->timeo_cond), NULL)
#define SOCK_COND_DESTROY(_so) pthread_cond_destroy((&(_so)->timeo_cond))
#define SOCK_COND(_so) (&(_so)->timeo_cond)
#endif














#define	SOCK_MTX(_so)			SOCKBUF_MTX(&(_so)->so_rcv)





#define	SOCK_LOCK_ASSERT(_so)		SOCKBUF_LOCK_ASSERT(&(_so)->so_rcv)













#define	SS_NOFDREF		0x0001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x0002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x0004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTING	0x0008	/* in process of disconnecting */
#define	SS_NBIO			0x0100	/* non-blocking ops */
#define	SS_ASYNC		0x0200	/* async i/o notify */
#define	SS_ISCONFIRMING		0x0400	/* deciding to accept connection req */
#define	SS_ISDISCONNECTED	0x2000	/* socket disconnected from peer */







#define	SS_PROTOREF		0x4000	/* strong protocol reference */




#define	SBS_CANTSENDMORE	0x0010	/* can't send more data to peer */
#define	SBS_CANTRCVMORE		0x0020	/* can't receive more data from peer */
#define	SBS_RCVATMARK		0x0040	/* at mark on input */




#define	SQ_INCOMP		0x0800	/* unaccepted, incomplete connection */
#define	SQ_COMP			0x1000	/* unaccepted, complete connection */




struct xsocket {
	size_t	xso_len;	
	struct	socket *xso_so;	
	short	so_type;
	short	so_options;
	short	so_linger;
	short	so_state;
	caddr_t	so_pcb;		
	int	xso_protocol;
	int	xso_family;
	u_short	so_qlen;
	u_short	so_incqlen;
	u_short	so_qlimit;
	short	so_timeo;
	u_short	so_error;
	pid_t	so_pgid;
	u_long	so_oobmark;
	struct xsockbuf {
		u_int	sb_cc;
		u_int	sb_hiwat;
		u_int	sb_mbcnt;
		u_int	sb_mbmax;
		int	sb_lowat;
		int	sb_timeo;
		short	sb_flags;
	} so_rcv, so_snd;
	uid_t	so_uid;		
};

#if defined(_KERNEL)









#define	sb_notify(sb)	(((sb)->sb_flags & (SB_WAIT | SB_SEL | SB_ASYNC | \
    SB_UPCALL | SB_AIO | SB_KNOTE)) != 0)







#define	sbspace(sb) \
    ((long) imin((int)((sb)->sb_hiwat - (sb)->sb_cc), \
	 (int)((sb)->sb_mbmax - (sb)->sb_mbcnt)))


#define	sosendallatonce(so) \
    ((so)->so_proto->pr_flags & PR_ATOMIC)


#define	soreadable(so) \
    ((so)->so_rcv.sb_cc >= (so)->so_rcv.sb_lowat || \
	((so)->so_rcv.sb_state & SBS_CANTRCVMORE) || \
	!TAILQ_EMPTY(&(so)->so_comp) || (so)->so_error)


#define	sowriteable(so) \
    ((sbspace(&(so)->so_snd) >= (so)->so_snd.sb_lowat && \
	(((so)->so_state&SS_ISCONNECTED) || \
	  ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0)) || \
     ((so)->so_snd.sb_state & SBS_CANTSENDMORE) || \
     (so)->so_error)


#define	sballoc(sb, m) { \
	(sb)->sb_cc += (m)->m_len; \
	if ((m)->m_type != MT_DATA && (m)->m_type != MT_OOBDATA) \
		(sb)->sb_ctl += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt += (m)->m_ext.ext_size; \
}


#define	sbfree(sb, m) { \
	(sb)->sb_cc -= (m)->m_len; \
	if ((m)->m_type != MT_DATA && (m)->m_type != MT_OOBDATA) \
		(sb)->sb_ctl -= (m)->m_len; \
	(sb)->sb_mbcnt -= MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt -= (m)->m_ext.ext_size; \
	if ((sb)->sb_sndptr == (m)) { \
		(sb)->sb_sndptr = NULL; \
		(sb)->sb_sndptroff = 0; \
	} \
	if ((sb)->sb_sndptroff != 0) \
		(sb)->sb_sndptroff -= (m)->m_len; \
}






#define	soref(so) do {							\
	SOCK_LOCK_ASSERT(so);						\
	++(so)->so_count;						\
} while (0)

#define	sorele(so) do {							\
	ACCEPT_LOCK_ASSERT();						\
	SOCK_LOCK_ASSERT(so);						\
	KASSERT((so)->so_count > 0, ("sorele"));			\
	if (--(so)->so_count == 0)					\
		sofree(so);						\
	else {								\
		SOCK_UNLOCK(so);					\
		ACCEPT_UNLOCK();					\
	}								\
} while (0)

#define	sotryfree(so) do {						\
	ACCEPT_LOCK_ASSERT();						\
	SOCK_LOCK_ASSERT(so);						\
	if ((so)->so_count == 0)					\
		sofree(so);						\
	else {								\
		SOCK_UNLOCK(so);					\
		ACCEPT_UNLOCK();					\
	}								\
} while(0)









#define	sorwakeup_locked(so) do {					\
	SOCKBUF_LOCK_ASSERT(&(so)->so_rcv);				\
	if (sb_notify(&(so)->so_rcv))					\
		sowakeup((so), &(so)->so_rcv);	 			\
	else								\
		SOCKBUF_UNLOCK(&(so)->so_rcv);				\
} while (0)

#define	sorwakeup(so) do {						\
	SOCKBUF_LOCK(&(so)->so_rcv);					\
	sorwakeup_locked(so);						\
} while (0)

#define	sowwakeup_locked(so) do {					\
	SOCKBUF_LOCK_ASSERT(&(so)->so_snd);				\
	if (sb_notify(&(so)->so_snd))					\
		sowakeup((so), &(so)->so_snd); 				\
	else								\
		SOCKBUF_UNLOCK(&(so)->so_snd);				\
} while (0)

#define	sowwakeup(so) do {						\
	SOCKBUF_LOCK(&(so)->so_snd);					\
	sowwakeup_locked(so);						\
} while (0)





enum sopt_dir { SOPT_GET, SOPT_SET };
struct sockopt {
	enum	sopt_dir sopt_dir; 
	int	sopt_level;	
	int	sopt_name;	
	void   *sopt_val;	
	size_t	sopt_valsize;	
	struct	thread *sopt_td; 
};

struct accept_filter {
	char	accf_name[16];
	void	(*accf_callback)
		(struct socket *so, void *arg, int waitflag);
	void *	(*accf_create)
		(struct socket *so, char *arg);
	void	(*accf_destroy)
		(struct socket *so);
	SLIST_ENTRY(accept_filter) accf_next;
};

extern int	maxsockets;
extern u_long	sb_max;
extern struct uma_zone *socket_zone;
extern so_gen_t so_gencnt;

struct mbuf;
struct sockaddr;
struct ucred;
struct uio;




int	do_getopt_accept_filter(struct socket *so, struct sockopt *sopt);
int	do_setopt_accept_filter(struct socket *so, struct sockopt *sopt);
int	so_setsockopt(struct socket *so, int level, int optname,
	    void *optval, size_t optlen);
int	sockargs(struct mbuf **mp, caddr_t buf, int buflen, int type);
int	getsockaddr(struct sockaddr **namp, caddr_t uaddr, size_t len);
void	sbappend(struct sockbuf *sb, struct mbuf *m);
void	sbappend_locked(struct sockbuf *sb, struct mbuf *m);
void	sbappendstream(struct sockbuf *sb, struct mbuf *m);
void	sbappendstream_locked(struct sockbuf *sb, struct mbuf *m);
int	sbappendaddr(struct sockbuf *sb, const struct sockaddr *asa,
	    struct mbuf *m0, struct mbuf *control);
int	sbappendaddr_locked(struct sockbuf *sb, const struct sockaddr *asa,
	    struct mbuf *m0, struct mbuf *control);
int	sbappendcontrol(struct sockbuf *sb, struct mbuf *m0,
	    struct mbuf *control);
int	sbappendcontrol_locked(struct sockbuf *sb, struct mbuf *m0,
	    struct mbuf *control);
void	sbappendrecord(struct sockbuf *sb, struct mbuf *m0);
void	sbappendrecord_locked(struct sockbuf *sb, struct mbuf *m0);
void	sbcheck(struct sockbuf *sb);
void	sbcompress(struct sockbuf *sb, struct mbuf *m, struct mbuf *n);
struct mbuf *
	sbcreatecontrol(caddr_t p, int size, int type, int level);
void	sbdestroy(struct sockbuf *sb, struct socket *so);
void	sbdrop(struct sockbuf *sb, int len);
void	sbdrop_locked(struct sockbuf *sb, int len);
void	sbdroprecord(struct sockbuf *sb);
void	sbdroprecord_locked(struct sockbuf *sb);
void	sbflush(struct sockbuf *sb);
void	sbflush_locked(struct sockbuf *sb);
void	sbrelease(struct sockbuf *sb, struct socket *so);
void	sbrelease_locked(struct sockbuf *sb, struct socket *so);
int	sbreserve(struct sockbuf *sb, u_long cc, struct socket *so,
	    struct thread *td);
int	sbreserve_locked(struct sockbuf *sb, u_long cc, struct socket *so,
	    struct thread *td);
struct mbuf *
	sbsndptr(struct sockbuf *sb, u_int off, u_int len, u_int *moff);
void	sbtoxsockbuf(struct sockbuf *sb, struct xsockbuf *xsb);
int	sbwait(struct sockbuf *sb);
int	sblock(struct sockbuf *sb, int flags);
void	sbunlock(struct sockbuf *sb);
void	soabort(struct socket *so);
int	soaccept(struct socket *so, struct sockaddr **nam);
int	socheckuid(struct socket *so, uid_t uid);
int	sobind(struct socket *so, struct sockaddr *nam, struct thread *td);
void	socantrcvmore(struct socket *so);
void	socantrcvmore_locked(struct socket *so);
void	socantsendmore(struct socket *so);
void	socantsendmore_locked(struct socket *so);
int	soclose(struct socket *so);
int	soconnect(struct socket *so, struct sockaddr *nam, struct thread *td);
int	soconnect2(struct socket *so1, struct socket *so2);
int	socow_setup(struct mbuf *m0, struct uio *uio);
int	socreate(int dom, struct socket **aso, int type, int proto,
	    struct ucred *cred, struct thread *td);
int	sodisconnect(struct socket *so);
struct	sockaddr *sodupsockaddr(const struct sockaddr *sa, int mflags);
void	sofree(struct socket *so);
int	sogetopt(struct socket *so, struct sockopt *sopt);
void	sohasoutofband(struct socket *so);
void	soisconnected(struct socket *so);
void	soisconnecting(struct socket *so);
void	soisdisconnected(struct socket *so);
void	soisdisconnecting(struct socket *so);
int	solisten(struct socket *so, int backlog, struct thread *td);
void	solisten_proto(struct socket *so, int backlog);
int	solisten_proto_check(struct socket *so);
struct socket *
	sonewconn(struct socket *head, int connstatus);
int	sooptcopyin(struct sockopt *sopt, void *buf, size_t len, size_t minlen);
int	sooptcopyout(struct sockopt *sopt, const void *buf, size_t len);


int	soopt_getm(struct sockopt *sopt, struct mbuf **mp);
int	soopt_mcopyin(struct sockopt *sopt, struct mbuf *m);
int	soopt_mcopyout(struct sockopt *sopt, struct mbuf *m);

int	sopoll(struct socket *so, int events, struct ucred *active_cred,
	    struct thread *td);
int	sopoll_generic(struct socket *so, int events,
	    struct ucred *active_cred, struct thread *td);
int	soreceive(struct socket *so, struct sockaddr **paddr, struct uio *uio,
	    struct mbuf **mp0, struct mbuf **controlp, int *flagsp);
int	soreceive_generic(struct socket *so, struct sockaddr **paddr,
	    struct uio *uio, struct mbuf **mp0, struct mbuf **controlp,
	    int *flagsp);
int	soreserve(struct socket *so, u_long sndcc, u_long rcvcc);
void	sorflush(struct socket *so);
int	sosend(struct socket *so, struct sockaddr *addr, struct uio *uio,
	    struct mbuf *top, struct mbuf *control, int flags,
	    struct thread *td);
int	sosend_dgram(struct socket *so, struct sockaddr *addr,
	    struct uio *uio, struct mbuf *top, struct mbuf *control,
	    int flags, struct thread *td);
int	sosend_generic(struct socket *so, struct sockaddr *addr,
	    struct uio *uio, struct mbuf *top, struct mbuf *control,
	    int flags, struct thread *td);
int	sosetopt(struct socket *so, struct sockopt *sopt);
int	soshutdown(struct socket *so, int how);
void	sotoxsocket(struct socket *so, struct xsocket *xso);
void	sowakeup(struct socket *so, struct sockbuf *sb);

#ifdef SOCKBUF_DEBUG
void	sblastrecordchk(struct sockbuf *, const char *, int);
#define	SBLASTRECORDCHK(sb)	sblastrecordchk((sb), __FILE__, __LINE__)

void	sblastmbufchk(struct sockbuf *, const char *, int);
#define	SBLASTMBUFCHK(sb)	sblastmbufchk((sb), __FILE__, __LINE__)
#else
#define	SBLASTRECORDCHK(sb)
#define	SBLASTMBUFCHK(sb)
#endif 




int	accept_filt_add(struct accept_filter *filt);
int	accept_filt_del(char *name);
struct	accept_filter *accept_filt_get(char *name);
#ifdef ACCEPT_FILTER_MOD
#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet_accf);
#endif
int	accept_filt_generic_mod_event(module_t mod, int event, void *data);
#endif

#endif 











#if defined(__Userspace__)




void	soisconnecting(struct socket *so);
void	soisdisconnecting(struct socket *so);
void	soisconnected(struct socket *so);
struct socket * sonewconn(struct socket *head, int connstatus);
void	socantrcvmore(struct socket *so);
void	socantsendmore(struct socket *so);







#define	soref(so) do {							\
	SOCK_LOCK_ASSERT(so);						\
	++(so)->so_count;						\
} while (0)

#define	sorele(so) do {							\
	ACCEPT_LOCK_ASSERT();						\
	SOCK_LOCK_ASSERT(so);						\
	KASSERT((so)->so_count > 0, ("sorele"));			\
	if (--(so)->so_count == 0)					\
		sofree(so);						\
	else {								\
		SOCK_UNLOCK(so);					\
		ACCEPT_UNLOCK();					\
	}								\
} while (0)



#define	sbspace(sb) \
    ((long) min((int)((sb)->sb_hiwat - (sb)->sb_cc), \
	 (int)((sb)->sb_mbmax - (sb)->sb_mbcnt)))


#define	sosendallatonce(so) \
    ((so)->so_proto->pr_flags & PR_ATOMIC)


#define	soreadable(so) \
    ((so)->so_rcv.sb_cc >= (so)->so_rcv.sb_lowat || \
	((so)->so_rcv.sb_state & SBS_CANTRCVMORE) || \
	!TAILQ_EMPTY(&(so)->so_comp) || (so)->so_error)

#if 0  
#define PR_CONNREQUIRED 0x04  /* from sys/protosw.h "needed" for sowriteable */
#define	sowriteable(so) \
    ((sbspace(&(so)->so_snd) >= (so)->so_snd.sb_lowat && \
	(((so)->so_state&SS_ISCONNECTED) || \
	  ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0)) || \
     ((so)->so_snd.sb_state & SBS_CANTSENDMORE) || \
     (so)->so_error)
#else  

#define	sowriteable(so) \
    ((sbspace(&(so)->so_snd) >= (so)->so_snd.sb_lowat && \
      (((so)->so_state&SS_ISCONNECTED))) ||              \
     ((so)->so_snd.sb_state & SBS_CANTSENDMORE) || \
     (so)->so_error)
#endif

extern void solisten_proto(struct socket *so, int backlog);
extern int solisten_proto_check(struct socket *so);
extern int sctp_listen(struct socket *so, int backlog, struct proc *p);
extern void socantrcvmore_locked(struct socket *so);
extern int sctp_bind(struct socket *so, struct sockaddr *addr);
extern int sctp6_bind(struct socket *so, struct sockaddr *addr, void *proc);
#if defined(__Userspace__)
extern int sctpconn_bind(struct socket *so, struct sockaddr *addr);
#endif
extern int sctp_accept(struct socket *so, struct sockaddr **addr);
extern int sctp_attach(struct socket *so, int proto, uint32_t vrf_id);
extern int sctp6_attach(struct socket *so, int proto, uint32_t vrf_id);
extern int sctp_abort(struct socket *so);
extern int sctp6_abort(struct socket *so);
extern void sctp_close(struct socket *so);
extern int soaccept(struct socket *so, struct sockaddr **nam);
extern int solisten(struct socket *so, int backlog);
extern int  soreserve(struct socket *so, u_long sndcc, u_long rcvcc);
extern void sowakeup(struct socket *so, struct sockbuf *sb);
extern void wakeup(void *ident, struct socket *so); 
extern int uiomove(void *cp, int n, struct uio *uio);
extern int sbwait(struct sockbuf *sb);
extern int sodisconnect(struct socket *so);
extern int soconnect(struct socket *so, struct sockaddr *nam);
extern int sctp_disconnect(struct socket *so);
extern int sctp_connect(struct socket *so, struct sockaddr *addr);
extern int sctp6_connect(struct socket *so, struct sockaddr *addr);
#if defined(__Userspace__)
extern int sctpconn_connect(struct socket *so, struct sockaddr *addr);
#endif
extern struct mbuf* mbufalloc(size_t size, void* data, unsigned char fill);
extern void sctp_finish(void);








#define	sb_notify(sb)	(((sb)->sb_flags & (SB_WAIT | SB_SEL | SB_ASYNC | \
    SB_UPCALL | SB_AIO | SB_KNOTE)) != 0)










#define	sorwakeup_locked(so) do {					\
	SOCKBUF_LOCK_ASSERT(&(so)->so_rcv);				\
	if (sb_notify(&(so)->so_rcv))					\
		sowakeup((so), &(so)->so_rcv);	 			\
	else								\
		SOCKBUF_UNLOCK(&(so)->so_rcv);				\
} while (0)

#define	sorwakeup(so) do {						\
	SOCKBUF_LOCK(&(so)->so_rcv);					\
	sorwakeup_locked(so);						\
} while (0)

#define	sowwakeup_locked(so) do {					\
	SOCKBUF_LOCK_ASSERT(&(so)->so_snd);				\
	if (sb_notify(&(so)->so_snd))					\
		sowakeup((so), &(so)->so_snd); 				\
	else								\
		SOCKBUF_UNLOCK(&(so)->so_snd);				\
} while (0)

#define	sowwakeup(so) do {						\
	SOCKBUF_LOCK(&(so)->so_snd);					\
	sowwakeup_locked(so);						\
} while (0)



#endif 

#endif 
