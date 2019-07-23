































#ifndef __OGGZ_VECTOR_H__
#define __OGGZ_VECTOR_H__

typedef void OggzVector;

typedef int (*OggzFunc) (void * data);
typedef int (*OggzFunc1) (void * data, void *arg);
typedef int (*OggzFindFunc) (void * data, long serialno);
typedef int (*OggzCmpFunc) (const void * a, const void * b, void * user_data);

OggzVector *
oggz_vector_new (void);

void
oggz_vector_delete (OggzVector * vector);

void *
oggz_vector_find_p (OggzVector * vector, const void * data);

int
oggz_vector_find_index_p (OggzVector * vector, const void * data);

void *
oggz_vector_find_with (OggzVector * vector, OggzFindFunc func, long serialno);

void *
oggz_vector_nth_p (OggzVector * vector, int n);

long
oggz_vector_nth_l (OggzVector * vector, int n);

int
oggz_vector_foreach (OggzVector * vector, OggzFunc func);

int
oggz_vector_foreach1 (OggzVector * vector, OggzFunc1 func, void *arg);

int
oggz_vector_size (OggzVector * vector);










void *
oggz_vector_insert_p (OggzVector * vector, void * data);

long
oggz_vector_insert_l (OggzVector * vector, long ldata);






OggzVector *
oggz_vector_remove_p (OggzVector * vector, void * data);






OggzVector *
oggz_vector_remove_l (OggzVector * vector, long ldata);

int
oggz_vector_set_cmp (OggzVector * vector, OggzCmpFunc compare,
		     void * user_data);

void *
oggz_vector_pop (OggzVector * vector);

#endif 
