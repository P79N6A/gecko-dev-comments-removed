























#include "unwind_i.h"
#include "ucontext_i.h"
#include <signal.h>
#include <limits.h>

#pragma weak pthread_once
#pragma weak pthread_key_create
#pragma weak pthread_getspecific
#pragma weak pthread_setspecific


#define HASH_MIN_BITS 14

typedef struct
{
  unw_tdep_frame_t *frames;
  size_t log_size;
  size_t used;
  size_t dtor_count;  

} unw_trace_cache_t;

static const unw_tdep_frame_t empty_frame = { 0, UNW_X86_64_FRAME_OTHER, -1, -1, 0, -1, -1 };
static pthread_mutex_t trace_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t trace_cache_once = PTHREAD_ONCE_INIT;
static sig_atomic_t trace_cache_once_happen;
static pthread_key_t trace_cache_key;
static struct mempool trace_cache_pool;
static __thread  unw_trace_cache_t *tls_cache;
static __thread  int tls_cache_destroyed;


static void
trace_cache_free (void *arg)
{
  unw_trace_cache_t *cache = arg;
  if (++cache->dtor_count < PTHREAD_DESTRUCTOR_ITERATIONS)
  {
    
    pthread_setspecific(trace_cache_key, cache);
    Debug(5, "delayed freeing cache %p (%zx to go)\n", cache,
	  PTHREAD_DESTRUCTOR_ITERATIONS - cache->dtor_count);
    return;
  }
  tls_cache_destroyed = 1;
  tls_cache = NULL;
  munmap (cache->frames, (1u << cache->log_size) * sizeof(unw_tdep_frame_t));
  mempool_free (&trace_cache_pool, cache);
  Debug(5, "freed cache %p\n", cache);
}


static void
trace_cache_init_once (void)
{
  pthread_key_create (&trace_cache_key, &trace_cache_free);
  mempool_init (&trace_cache_pool, sizeof (unw_trace_cache_t), 0);
  trace_cache_once_happen = 1;
}

static unw_tdep_frame_t *
trace_cache_buckets (size_t n)
{
  unw_tdep_frame_t *frames;
  size_t i;

  GET_MEMORY(frames, n * sizeof (unw_tdep_frame_t));
  if (likely(frames != 0))
    for (i = 0; i < n; ++i)
      frames[i] = empty_frame;

  return frames;
}




static unw_trace_cache_t *
trace_cache_create (void)
{
  unw_trace_cache_t *cache;

  if (tls_cache_destroyed)
  {
    

    Debug(5, "refusing to reallocate cache: "
	     "thread-locals are being deallocated\n");
    return NULL;
  }

  if (! (cache = mempool_alloc(&trace_cache_pool)))
  {
    Debug(5, "failed to allocate cache\n");
    return NULL;
  }

  if (! (cache->frames = trace_cache_buckets(1u << HASH_MIN_BITS)))
  {
    Debug(5, "failed to allocate buckets\n");
    mempool_free(&trace_cache_pool, cache);
    return NULL;
  }

  cache->log_size = HASH_MIN_BITS;
  cache->used = 0;
  cache->dtor_count = 0;
  tls_cache_destroyed = 0;  
  Debug(5, "allocated cache %p\n", cache);
  return cache;
}



static int
trace_cache_expand (unw_trace_cache_t *cache)
{
  size_t old_size = (1u << cache->log_size);
  size_t new_log_size = cache->log_size + 2;
  unw_tdep_frame_t *new_frames = trace_cache_buckets (1u << new_log_size);

  if (unlikely(! new_frames))
  {
    Debug(5, "failed to expand cache to 2^%lu buckets\n", new_log_size);
    return -UNW_ENOMEM;
  }

  Debug(5, "expanded cache from 2^%lu to 2^%lu buckets\n", cache->log_size, new_log_size);
  munmap(cache->frames, old_size * sizeof(unw_tdep_frame_t));
  cache->frames = new_frames;
  cache->log_size = new_log_size;
  cache->used = 0;
  return 0;
}

static unw_trace_cache_t *
trace_cache_get_unthreaded (void)
{
  unw_trace_cache_t *cache;
  intrmask_t saved_mask;
  static unw_trace_cache_t *global_cache = 0;
  lock_acquire (&trace_init_lock, saved_mask);
  if (! global_cache)
  {
    mempool_init (&trace_cache_pool, sizeof (unw_trace_cache_t), 0);
    global_cache = trace_cache_create ();
  }
  cache = global_cache;
  lock_release (&trace_init_lock, saved_mask);
  Debug(5, "using cache %p\n", cache);
  return cache;
}


static unw_trace_cache_t *
trace_cache_get (void)
{
  unw_trace_cache_t *cache;
  if (likely (pthread_once != 0))
  {
    pthread_once(&trace_cache_once, &trace_cache_init_once);
    if (!trace_cache_once_happen)
    {
      return trace_cache_get_unthreaded();
    }
    if (! (cache = tls_cache))
    {
      cache = trace_cache_create();
      pthread_setspecific(trace_cache_key, cache);
      tls_cache = cache;
    }
    Debug(5, "using cache %p\n", cache);
    return cache;
  }
  else
  {
    return trace_cache_get_unthreaded();
  }
}









static unw_tdep_frame_t *
trace_init_addr (unw_tdep_frame_t *f,
		 unw_cursor_t *cursor,
		 unw_word_t cfa,
		 unw_word_t rip,
		 unw_word_t rbp,
		 unw_word_t rsp)
{
  struct cursor *c = (struct cursor *) cursor;
  struct dwarf_cursor *d = &c->dwarf;
  int ret = -UNW_EINVAL;

  
  f->virtual_address = rip;
  f->frame_type = UNW_X86_64_FRAME_OTHER;
  f->last_frame = 0;
  f->cfa_reg_rsp = -1;
  f->cfa_reg_offset = 0;
  f->rbp_cfa_offset = -1;
  f->rsp_cfa_offset = -1;

  



  d->ip = rip + d->use_prev_instr;
  d->cfa = cfa;
  d->loc[UNW_X86_64_RIP] = DWARF_REG_LOC (d, UNW_X86_64_RIP);
  d->loc[UNW_X86_64_RBP] = DWARF_REG_LOC (d, UNW_X86_64_RBP);
  d->loc[UNW_X86_64_RSP] = DWARF_REG_LOC (d, UNW_X86_64_RSP);
  c->frame_info = *f;

  if (likely(dwarf_put (d, d->loc[UNW_X86_64_RIP], rip) >= 0)
      && likely(dwarf_put (d, d->loc[UNW_X86_64_RBP], rbp) >= 0)
      && likely(dwarf_put (d, d->loc[UNW_X86_64_RSP], rsp) >= 0)
      && likely((ret = unw_step (cursor)) >= 0))
    *f = c->frame_info;

  





  if (ret == 0)
    f->last_frame = -1;

  Debug (3, "frame va %lx type %d last %d cfa %s+%d rbp @ cfa%+d rsp @ cfa%+d\n",
	 f->virtual_address, f->frame_type, f->last_frame,
	 f->cfa_reg_rsp ? "rsp" : "rbp", f->cfa_reg_offset,
	 f->rbp_cfa_offset, f->rsp_cfa_offset);

  return f;
}





static unw_tdep_frame_t *
trace_lookup (unw_cursor_t *cursor,
	      unw_trace_cache_t *cache,
	      unw_word_t cfa,
	      unw_word_t rip,
	      unw_word_t rbp,
	      unw_word_t rsp)
{
  




  uint64_t i, addr;
  uint64_t cache_size = 1u << cache->log_size;
  uint64_t slot = ((rip * 0x9e3779b97f4a7c16) >> 43) & (cache_size-1);
  unw_tdep_frame_t *frame;

  for (i = 0; i < 16; ++i)
  {
    frame = &cache->frames[slot];
    addr = frame->virtual_address;

    
    if (likely(addr == rip))
    {
      Debug (4, "found address after %ld steps\n", i);
      return frame;
    }

    
    if (likely(! addr))
      break;

    
    if (++slot >= cache_size)
      slot -= cache_size;
  }

  



  Debug (4, "updating slot %lu after %ld steps, replacing 0x%lx\n", slot, i, addr);
  if (unlikely(addr || cache->used >= cache_size / 2))
  {
    if (unlikely(trace_cache_expand (cache) < 0))
      return NULL;

    cache_size = 1u << cache->log_size;
    slot = ((rip * 0x9e3779b97f4a7c16) >> 43) & (cache_size-1);
    frame = &cache->frames[slot];
    addr = frame->virtual_address;
  }

  if (! addr)
    ++cache->used;

  return trace_init_addr (frame, cursor, cfa, rip, rbp, rsp);
}
































































HIDDEN int
tdep_trace (unw_cursor_t *cursor, void **buffer, int *size)
{
  struct cursor *c = (struct cursor *) cursor;
  struct dwarf_cursor *d = &c->dwarf;
  unw_trace_cache_t *cache;
  unw_word_t rbp, rsp, rip, cfa;
  int maxdepth = 0;
  int depth = 0;
  int ret;

  
  if (unlikely(! cursor || ! buffer || ! size || (maxdepth = *size) <= 0))
    return -UNW_EINVAL;

  Debug (1, "begin ip 0x%lx cfa 0x%lx\n", d->ip, d->cfa);

  
  d->stash_frames = 1;

  

  rip = d->ip;
  rsp = cfa = d->cfa;
  ACCESS_MEM_FAST(ret, 0, d, DWARF_GET_LOC(d->loc[UNW_X86_64_RBP]), rbp);
  assert(ret == 0);

  
  if (unlikely(! (cache = trace_cache_get())))
  {
    Debug (1, "returning %d, cannot get trace cache\n", -UNW_ENOMEM);
    *size = 0;
    d->stash_frames = 0;
    return -UNW_ENOMEM;
  }

  



  while (depth < maxdepth)
  {
    rip -= d->use_prev_instr;
    Debug (2, "depth %d cfa 0x%lx rip 0x%lx rsp 0x%lx rbp 0x%lx\n",
	   depth, cfa, rip, rsp, rbp);

    




    unw_tdep_frame_t *f = trace_lookup (cursor, cache, cfa, rip, rbp, rsp);

    
    if (unlikely(! f))
    {
      ret = -UNW_ENOINFO;
      break;
    }

    Debug (3, "frame va %lx type %d last %d cfa %s+%d rbp @ cfa%+d rsp @ cfa%+d\n",
           f->virtual_address, f->frame_type, f->last_frame,
           f->cfa_reg_rsp ? "rsp" : "rbp", f->cfa_reg_offset,
           f->rbp_cfa_offset, f->rsp_cfa_offset);

    assert (f->virtual_address == rip);

    




    if (f->last_frame)
      break;

    
    switch (f->frame_type)
    {
    case UNW_X86_64_FRAME_GUESSED:
      
      c->validate = 1;

    case UNW_X86_64_FRAME_STANDARD:
      
      cfa = (f->cfa_reg_rsp ? rsp : rbp) + f->cfa_reg_offset;
      ACCESS_MEM_FAST(ret, c->validate, d, cfa - 8, rip);
      if (likely(ret >= 0) && likely(f->rbp_cfa_offset != -1))
	ACCESS_MEM_FAST(ret, c->validate, d, cfa + f->rbp_cfa_offset, rbp);

      
      rsp = cfa;

      
      d->use_prev_instr = 1;
      break;

    case UNW_X86_64_FRAME_SIGRETURN:
      cfa = cfa + f->cfa_reg_offset; 

      ACCESS_MEM_FAST(ret, c->validate, d, cfa + UC_MCONTEXT_GREGS_RIP, rip);
      if (likely(ret >= 0))
        ACCESS_MEM_FAST(ret, c->validate, d, cfa + UC_MCONTEXT_GREGS_RBP, rbp);
      if (likely(ret >= 0))
        ACCESS_MEM_FAST(ret, c->validate, d, cfa + UC_MCONTEXT_GREGS_RSP, rsp);

      

      cfa = rsp;

      
      d->use_prev_instr = 0;
      break;

    default:
      


      ret = -UNW_ESTOPUNWIND;
      break;
    }

    Debug (4, "new cfa 0x%lx rip 0x%lx rsp 0x%lx rbp 0x%lx\n",
	   cfa, rip, rsp, rbp);

    
    if (unlikely(ret < 0 || rip < 0x4000))
      break;

    
    buffer[depth++] = (void *) (rip - d->use_prev_instr);
  }

#if UNW_DEBUG
  Debug (1, "returning %d, depth %d\n", ret, depth);
#endif
  *size = depth;
  return ret;
}
