
























#ifndef _EVENT2_THREAD_H_
#define _EVENT2_THREAD_H_





















#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>








#define EVTHREAD_WRITE	0x04


#define EVTHREAD_READ	0x08



#define EVTHREAD_TRY    0x10


#if !defined(_EVENT_DISABLE_THREAD_SUPPORT) || defined(_EVENT_IN_DOXYGEN)

#define EVTHREAD_LOCK_API_VERSION 1








#define EVTHREAD_LOCKTYPE_RECURSIVE 1


#define EVTHREAD_LOCKTYPE_READWRITE 2






struct evthread_lock_callbacks {
	

	int lock_api_version;
	






	unsigned supported_locktypes;
	

	void *(*alloc)(unsigned locktype);
	

	void (*free)(void *lock, unsigned locktype);
	

	int (*lock)(unsigned mode, void *lock);
	

	int (*unlock)(unsigned mode, void *lock);
};









int evthread_set_lock_callbacks(const struct evthread_lock_callbacks *);

#define EVTHREAD_CONDITION_API_VERSION 1

struct timeval;





struct evthread_condition_callbacks {
	

	int condition_api_version;
	



	void *(*alloc_condition)(unsigned condtype);
	
	void (*free_condition)(void *cond);
	





	int (*signal_condition)(void *cond, int broadcast);
	









	int (*wait_condition)(void *cond, void *lock,
	    const struct timeval *timeout);
};









int evthread_set_condition_callbacks(
	const struct evthread_condition_callbacks *);








void evthread_set_id_callback(
    unsigned long (*id_fn)(void));

#if (defined(WIN32) && !defined(_EVENT_DISABLE_THREAD_SUPPORT)) || defined(_EVENT_IN_DOXYGEN)




int evthread_use_windows_threads(void);



#define EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED 1
#endif

#if defined(_EVENT_HAVE_PTHREADS) || defined(_EVENT_IN_DOXYGEN)





int evthread_use_pthreads(void);

#define EVTHREAD_USE_PTHREADS_IMPLEMENTED 1

#endif







void evthread_enable_lock_debuging(void);

#endif 

struct event_base;





int evthread_make_base_notifiable(struct event_base *base);

#ifdef __cplusplus
}
#endif

#endif
