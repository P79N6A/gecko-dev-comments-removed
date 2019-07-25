
























#include "libunwind_i.h"

#define MAX_ALIGN	(sizeof (long double))

static char sos_memory[SOS_MEMORY_SIZE];
static char *sos_memp;
static size_t pg_size;

HIDDEN void *
sos_alloc (size_t size)
{
  char *mem;

#ifdef HAVE_CMPXCHG
  char *old_mem;

  size = (size + MAX_ALIGN - 1) & -MAX_ALIGN;
  if (!sos_memp)
    cmpxchg_ptr (&sos_memp, 0, sos_memory);
  do
    {
      old_mem = sos_memp;

      mem = (char *) (((unsigned long) old_mem + MAX_ALIGN - 1) & -MAX_ALIGN);
      mem += size;
      assert (mem < sos_memory + sizeof (sos_memory));
    }
  while (!cmpxchg_ptr (&sos_memp, old_mem, mem));
#else
  static define_lock (sos_lock);
  intrmask_t saved_mask;

  size = (size + MAX_ALIGN - 1) & -MAX_ALIGN;

  lock_acquire (&sos_lock, saved_mask);
  {
    if (!sos_memp)
      sos_memp = sos_memory;

    mem = (char *) (((unsigned long) sos_memp + MAX_ALIGN - 1) & -MAX_ALIGN);
    mem += size;
    assert (mem < sos_memory + sizeof (sos_memory));
    sos_memp = mem;
  }
  lock_release (&sos_lock, saved_mask);
#endif
  return mem;
}



static void
free_object (struct mempool *pool, void *object)
{
  struct object *obj = object;

  obj->next = pool->free_list;
  pool->free_list = obj;
  ++pool->num_free;
}

static void
add_memory (struct mempool *pool, char *mem, size_t size, size_t obj_size)
{
  char *obj;

  for (obj = mem; obj <= mem + size - obj_size; obj += obj_size)
    free_object (pool, obj);
}

static void
expand (struct mempool *pool)
{
  size_t size;
  char *mem;

  size = pool->chunk_size;
  GET_MEMORY (mem, size);
  if (!mem)
    {
      size = (pool->obj_size + pg_size - 1) & -pg_size;
      GET_MEMORY (mem, size);
      if (!mem)
	{
	  
	  size = pool->obj_size;
	  mem = sos_alloc (size);
	}
    }
  add_memory (pool, mem, size, pool->obj_size);
}

HIDDEN void
mempool_init (struct mempool *pool, size_t obj_size, size_t reserve)
{
  if (pg_size == 0)
    pg_size = getpagesize ();

  memset (pool, 0, sizeof (*pool));

  lock_init (&pool->lock);

  
  obj_size = (obj_size + MAX_ALIGN - 1) & -MAX_ALIGN;

  if (!reserve)
    {
      reserve = pg_size / obj_size / 4;
      if (!reserve)
	reserve = 16;
    }

  pool->obj_size = obj_size;
  pool->reserve = reserve;
  pool->chunk_size = (2*reserve*obj_size + pg_size - 1) & -pg_size;

  expand (pool);
}

HIDDEN void *
mempool_alloc (struct mempool *pool)
{
  intrmask_t saved_mask;
  struct object *obj;

  lock_acquire (&pool->lock, saved_mask);
  {
    if (pool->num_free <= pool->reserve)
      expand (pool);

    assert (pool->num_free > 0);

    --pool->num_free;
    obj = pool->free_list;
    pool->free_list = obj->next;
  }
  lock_release (&pool->lock, saved_mask);
  return obj;
}

HIDDEN void
mempool_free (struct mempool *pool, void *object)
{
  intrmask_t saved_mask;

  lock_acquire (&pool->lock, saved_mask);
  {
    free_object (pool, object);
  }
  lock_release (&pool->lock, saved_mask);
}
