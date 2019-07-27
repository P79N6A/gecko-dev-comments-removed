


























#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "hb-private.hh"

#include "hb-object-private.hh"

#ifdef HAVE_SYS_MMAN_H
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif 
#include <sys/mman.h>
#endif 

#include <stdio.h>
#include <errno.h>



#ifndef HB_DEBUG_BLOB
#define HB_DEBUG_BLOB (HB_DEBUG+0)
#endif


struct hb_blob_t {
  hb_object_header_t header;
  ASSERT_POD ();

  bool immutable;

  const char *data;
  unsigned int length;
  hb_memory_mode_t mode;

  void *user_data;
  hb_destroy_func_t destroy;
};


static bool _try_writable (hb_blob_t *blob);

static void
_hb_blob_destroy_user_data (hb_blob_t *blob)
{
  if (blob->destroy) {
    blob->destroy (blob->user_data);
    blob->user_data = NULL;
    blob->destroy = NULL;
  }
}

















hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       length,
		hb_memory_mode_t   mode,
		void              *user_data,
		hb_destroy_func_t  destroy)
{
  hb_blob_t *blob;

  if (!length || !(blob = hb_object_create<hb_blob_t> ())) {
    if (destroy)
      destroy (user_data);
    return hb_blob_get_empty ();
  }

  blob->data = data;
  blob->length = length;
  blob->mode = mode;

  blob->user_data = user_data;
  blob->destroy = destroy;

  if (blob->mode == HB_MEMORY_MODE_DUPLICATE) {
    blob->mode = HB_MEMORY_MODE_READONLY;
    if (!_try_writable (blob)) {
      hb_blob_destroy (blob);
      return hb_blob_get_empty ();
    }
  }

  return blob;
}





















hb_blob_t *
hb_blob_create_sub_blob (hb_blob_t    *parent,
			 unsigned int  offset,
			 unsigned int  length)
{
  hb_blob_t *blob;

  if (!length || offset >= parent->length)
    return hb_blob_get_empty ();

  hb_blob_make_immutable (parent);

  blob = hb_blob_create (parent->data + offset,
			 MIN (length, parent->length - offset),
			 HB_MEMORY_MODE_READONLY,
			 hb_blob_reference (parent),
			 (hb_destroy_func_t) hb_blob_destroy);

  return blob;
}












hb_blob_t *
hb_blob_get_empty (void)
{
  static const hb_blob_t _hb_blob_nil = {
    HB_OBJECT_HEADER_STATIC,

    true, 

    NULL, 
    0, 
    HB_MEMORY_MODE_READONLY, 

    NULL, 
    NULL  
  };

  return const_cast<hb_blob_t *> (&_hb_blob_nil);
}













hb_blob_t *
hb_blob_reference (hb_blob_t *blob)
{
  return hb_object_reference (blob);
}













void
hb_blob_destroy (hb_blob_t *blob)
{
  if (!hb_object_destroy (blob)) return;

  _hb_blob_destroy_user_data (blob);

  free (blob);
}













hb_bool_t
hb_blob_set_user_data (hb_blob_t          *blob,
		       hb_user_data_key_t *key,
		       void *              data,
		       hb_destroy_func_t   destroy,
		       hb_bool_t           replace)
{
  return hb_object_set_user_data (blob, key, data, destroy, replace);
}












void *
hb_blob_get_user_data (hb_blob_t          *blob,
		       hb_user_data_key_t *key)
{
  return hb_object_get_user_data (blob, key);
}










void
hb_blob_make_immutable (hb_blob_t *blob)
{
  if (hb_object_is_inert (blob))
    return;

  blob->immutable = true;
}











hb_bool_t
hb_blob_is_immutable (hb_blob_t *blob)
{
  return blob->immutable;
}












unsigned int
hb_blob_get_length (hb_blob_t *blob)
{
  return blob->length;
}












const char *
hb_blob_get_data (hb_blob_t *blob, unsigned int *length)
{
  if (length)
    *length = blob->length;

  return blob->data;
}

















char *
hb_blob_get_data_writable (hb_blob_t *blob, unsigned int *length)
{
  if (!_try_writable (blob)) {
    if (length)
      *length = 0;

    return NULL;
  }

  if (length)
    *length = blob->length;

  return const_cast<char *> (blob->data);
}


static hb_bool_t
_try_make_writable_inplace_unix (hb_blob_t *blob)
{
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MPROTECT)
  uintptr_t pagesize = -1, mask, length;
  const char *addr;

#if defined(HAVE_SYSCONF) && defined(_SC_PAGE_SIZE)
  pagesize = (uintptr_t) sysconf (_SC_PAGE_SIZE);
#elif defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
  pagesize = (uintptr_t) sysconf (_SC_PAGESIZE);
#elif defined(HAVE_GETPAGESIZE)
  pagesize = (uintptr_t) getpagesize ();
#endif

  if ((uintptr_t) -1L == pagesize) {
    DEBUG_MSG_FUNC (BLOB, blob, "failed to get pagesize: %s", strerror (errno));
    return false;
  }
  DEBUG_MSG_FUNC (BLOB, blob, "pagesize is %lu", (unsigned long) pagesize);

  mask = ~(pagesize-1);
  addr = (const char *) (((uintptr_t) blob->data) & mask);
  length = (const char *) (((uintptr_t) blob->data + blob->length + pagesize-1) & mask)  - addr;
  DEBUG_MSG_FUNC (BLOB, blob,
		  "calling mprotect on [%p..%p] (%lu bytes)",
		  addr, addr+length, (unsigned long) length);
  if (-1 == mprotect ((void *) addr, length, PROT_READ | PROT_WRITE)) {
    DEBUG_MSG_FUNC (BLOB, blob, "mprotect failed: %s", strerror (errno));
    return false;
  }

  blob->mode = HB_MEMORY_MODE_WRITABLE;

  DEBUG_MSG_FUNC (BLOB, blob,
		  "successfully made [%p..%p] (%lu bytes) writable\n",
		  addr, addr+length, (unsigned long) length);
  return true;
#else
  return false;
#endif
}

static bool
_try_writable_inplace (hb_blob_t *blob)
{
  DEBUG_MSG_FUNC (BLOB, blob, "making writable inplace\n");

  if (_try_make_writable_inplace_unix (blob))
    return true;

  DEBUG_MSG_FUNC (BLOB, blob, "making writable -> FAILED\n");

  
  blob->mode = HB_MEMORY_MODE_READONLY;
  return false;
}

static bool
_try_writable (hb_blob_t *blob)
{
  if (blob->immutable)
    return false;

  if (blob->mode == HB_MEMORY_MODE_WRITABLE)
    return true;

  if (blob->mode == HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE && _try_writable_inplace (blob))
    return true;

  if (blob->mode == HB_MEMORY_MODE_WRITABLE)
    return true;


  DEBUG_MSG_FUNC (BLOB, blob, "current data is -> %p\n", blob->data);

  char *new_data;

  new_data = (char *) malloc (blob->length);
  if (unlikely (!new_data))
    return false;

  DEBUG_MSG_FUNC (BLOB, blob, "dupped successfully -> %p\n", blob->data);

  memcpy (new_data, blob->data, blob->length);
  _hb_blob_destroy_user_data (blob);
  blob->mode = HB_MEMORY_MODE_WRITABLE;
  blob->data = new_data;
  blob->user_data = new_data;
  blob->destroy = free;

  return true;
}
