






























#ifndef HB_MUTEX_PRIVATE_HH
#define HB_MUTEX_PRIVATE_HH

#include "hb-private.hh"







#ifdef HAVE_GLIB

#include <glib.h>

typedef GStaticMutex hb_mutex_impl_t;
#define HB_MUTEX_IMPL_INIT	G_STATIC_MUTEX_INIT
#define hb_mutex_impl_init(M)	g_static_mutex_init (M)
#define hb_mutex_impl_lock(M)	g_static_mutex_lock (M)
#define hb_mutex_impl_unlock(M)	g_static_mutex_unlock (M)
#define hb_mutex_impl_free(M)	g_static_mutex_free (M)


#elif defined(_MSC_VER) || defined(__MINGW32__)

#include <windows.h>

typedef CRITICAL_SECTION hb_mutex_impl_t;
#define HB_MUTEX_IMPL_INIT	{ NULL, 0, 0, NULL, NULL, 0 }
#define hb_mutex_impl_init(M)	InitializeCriticalSection (M)
#define hb_mutex_impl_lock(M)	EnterCriticalSection (M)
#define hb_mutex_impl_unlock(M)	LeaveCriticalSection (M)
#define hb_mutex_impl_free(M)	DeleteCriticalSection (M)


#else

#warning "Could not find any system to define platform macros, library will NOT be thread-safe"

typedef volatile int hb_mutex_impl_t;
#define HB_MUTEX_IMPL_INIT	0
#define hb_mutex_impl_init(M)	((void) (*(M) = 0))
#define hb_mutex_impl_lock(M)	((void) (*(M) = 1))
#define hb_mutex_impl_unlock(M)	((void) (*(M) = 0))
#define hb_mutex_impl_free(M)	((void) (*(M) = 2))


#endif


struct hb_mutex_t
{
  hb_mutex_impl_t m;

  inline void init   (void) { hb_mutex_impl_init   (&m); }
  inline void lock   (void) { hb_mutex_impl_lock   (&m); }
  inline void unlock (void) { hb_mutex_impl_unlock (&m); }
  inline void free   (void) { hb_mutex_impl_free   (&m); }
};

#define HB_MUTEX_INIT		{HB_MUTEX_IMPL_INIT}
#define hb_mutex_init(M)	(M)->init ()
#define hb_mutex_lock(M)	(M)->lock ()
#define hb_mutex_unlock(M)	(M)->unlock ()
#define hb_mutex_free(M)	(M)->free ()


struct hb_static_mutex_t : hb_mutex_t
{
  hb_static_mutex_t (void)  { this->init (); }
  ~hb_static_mutex_t (void) { this->free (); }

  private:
  NO_COPY (hb_static_mutex_t);
};



#endif 
