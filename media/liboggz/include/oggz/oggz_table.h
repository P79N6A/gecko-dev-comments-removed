































#ifndef __OGGZ_TABLE_H__
#define __OGGZ_TABLE_H__











typedef void OggzTable;






OggzTable *
oggz_table_new (void);





void
oggz_table_delete (OggzTable * table);










void *
oggz_table_insert (OggzTable * table, long key, void * data);








int
oggz_table_remove (OggzTable * table, long key);








void *
oggz_table_lookup (OggzTable * table, long key);






int
oggz_table_size (OggzTable * table);










void *
oggz_table_nth (OggzTable * table, int n, long * key);

#endif 
