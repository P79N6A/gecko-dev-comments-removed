































#include "config.h"

#include <stdlib.h>
#include "oggz_macros.h"
#include "oggz_vector.h"

typedef struct _OggzTable OggzTable;

struct _OggzTable {
  OggzVector * keys;
  OggzVector * data;
};

OggzTable *
oggz_table_new (void)
{
  OggzTable * table;

  table = oggz_malloc (sizeof (OggzTable));
  if (table == NULL) return NULL;

  table->keys = oggz_vector_new ();
  table->data = oggz_vector_new ();

  return table;
}

void
oggz_table_delete (OggzTable * table)
{
  if (table == NULL) return;

  oggz_vector_delete (table->keys);
  oggz_vector_delete (table->data);
  oggz_free (table);
}

void *
oggz_table_lookup (OggzTable * table, long key)
{
  int i, size;

  if (table == NULL) return NULL;

  size = oggz_vector_size (table->keys);
  for (i = 0; i < size; i++) {
    if (oggz_vector_nth_l (table->keys, i) == key) {
      return oggz_vector_nth_p (table->data, i);
    }
  }

  return NULL;
}

void *
oggz_table_insert (OggzTable * table, long key, void * data)
{
  void * old_data;

  if ((old_data = oggz_table_lookup (table, key)) != NULL) {
    if (oggz_vector_remove_l (table->keys, key) == NULL)
      return NULL;

    if (oggz_vector_remove_p (table->data, old_data) == NULL) {
      


      return NULL;
    }
  }

  if (oggz_vector_insert_l (table->keys, key) == -1)
    return NULL;
  
  if (oggz_vector_insert_p (table->data, data) == NULL) {
    oggz_vector_remove_l (table->keys, key);
    return NULL;
  }

  return data;
}

int
oggz_table_remove (OggzTable * table, long key)
{
  void * old_data;

  if ((old_data = oggz_table_lookup (table, key)) != NULL) {
    if (oggz_vector_remove_l (table->keys, key) == NULL)
      return -1;

    if (oggz_vector_remove_p (table->data, old_data) == NULL) {
      


      return -1;
    }
  }
  
  return 0;
}

int
oggz_table_size (OggzTable * table)
{
  if (table == NULL) return 0;
  return oggz_vector_size (table->data);
}

void *
oggz_table_nth (OggzTable * table, int n, long * key)
{
  if (table == NULL) return NULL;
  if (key) *key = oggz_vector_nth_l (table->keys, n);
  return oggz_vector_nth_p (table->data, n);
}
