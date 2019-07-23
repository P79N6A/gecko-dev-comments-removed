































#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_compat.h"

typedef int (*FishSoundFunc) (void * data);
typedef int (*FishSoundCmpFunc) (const void * data1, const void * data2);

typedef struct _FishSoundVector FishSoundVector;

struct _FishSoundVector {
  int max_elements;
  int nr_elements;
  FishSoundCmpFunc cmp;
  void ** data;
};





FishSoundVector *
fs_vector_new (FishSoundCmpFunc cmp)
{
  FishSoundVector * vector;

  vector = fs_malloc (sizeof (FishSoundVector));

  vector->max_elements = 0;
  vector->nr_elements = 0;
  vector->cmp = cmp;
  vector->data = NULL;

  return vector;
}

static void
fs_vector_clear (FishSoundVector * vector)
{
  fs_free (vector->data);
  vector->data = NULL;
  vector->nr_elements = 0;
  vector->max_elements = 0;
}

void
fs_vector_delete (FishSoundVector * vector)
{
  fs_vector_clear (vector);
  fs_free (vector);
}

int
fs_vector_size (FishSoundVector * vector)
{
  if (vector == NULL) return 0;

  return vector->nr_elements;
}

void *
fs_vector_nth (FishSoundVector * vector, int n)
{
  if (vector == NULL) return NULL;

  if (n >= vector->nr_elements) return NULL;

  return vector->data[n];
}

int
fs_vector_find_index (FishSoundVector * vector, const void * data)
{
  void * v_data;
  int i;

  for (i = 0; i < vector->nr_elements; i++) {
    v_data = vector->data[i];
    if (vector->cmp (v_data, data))
      return i;
  }

  return -1;
}

void *
fs_vector_find (FishSoundVector * vector, const void * data)
{
  void * v_data;
  int i;

  for (i = 0; i < vector->nr_elements; i++) {
    v_data = vector->data[i];
    if (vector->cmp (v_data, data))
      return v_data;
  }

  return NULL;
}

int
fs_vector_foreach (FishSoundVector * vector, FishSoundFunc func)
{
  int i;

  for (i = 0; i < vector->nr_elements; i++) {
    func (vector->data[i]);
  }

  return 0;
}

static FishSoundVector *
fs_vector_grow (FishSoundVector * vector)
{
  void * new_elements;
  int new_max_elements;

  vector->nr_elements++;

  if (vector->nr_elements > vector->max_elements) {
    if (vector->max_elements == 0) {
      new_max_elements = 1;
    } else {
      new_max_elements = vector->max_elements * 2;
    }

    new_elements =
      fs_realloc (vector->data, (size_t)new_max_elements * sizeof (void *));

    if (new_elements == NULL) {
      vector->nr_elements--;
      return NULL;
    }

    vector->max_elements = new_max_elements;
    vector->data = new_elements;
  }

  return vector;
}

void *
fs_vector_insert (FishSoundVector * vector, void * data)
{
  if (fs_vector_grow (vector) == NULL)
    return NULL;

  vector->data[vector->nr_elements-1] = data;

  return data;

}

static void *
fs_vector_remove_nth (FishSoundVector * vector, int n)
{
  int i;
  void * new_elements;
  int new_max_elements;

  vector->nr_elements--;

  if (vector->nr_elements == 0) {
    fs_vector_clear (vector);
  } else {
    for (i = n; i < vector->nr_elements; i++) {
      vector->data[i] = vector->data[i+1];
    }

    if (vector->nr_elements < vector->max_elements/2) {
      new_max_elements = vector->max_elements/2;

      new_elements =
	fs_realloc (vector->data,
		    (size_t)new_max_elements * sizeof (void *));
      
      if (new_elements == NULL)
	return NULL;

      vector->max_elements = new_max_elements;
      vector->data = new_elements;
    }
  }

  return vector;
}

FishSoundVector *
fs_vector_remove (FishSoundVector * vector, void * data)
{
  int i;

  for (i = 0; i < vector->nr_elements; i++) {
    if (vector->data[i] == data) {
      return fs_vector_remove_nth (vector, i);
    }
  }

  return vector;
}
