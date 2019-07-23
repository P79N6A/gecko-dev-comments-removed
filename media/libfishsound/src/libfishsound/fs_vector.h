































#ifndef __FS_VECTOR_H__
#define __FS_VECTOR_H__

typedef void FishSoundVector;

typedef int (*FishSoundFunc) (void * data);
typedef int (*FishSoundCmpFunc) (void * data1, void * data2);

FishSoundVector *
fs_vector_new (FishSoundCmpFunc cmp);

void
fs_vector_delete (FishSoundVector * vector);

void *
fs_vector_nth (FishSoundVector * vector, int n);

int
fs_vector_find_index (FishSoundVector * vector, const void * data);

void *
fs_vector_find (FishSoundVector * vector, const void * data);

int
fs_vector_foreach (FishSoundVector * vector, FishSoundFunc func);

int
fs_vector_size (FishSoundVector * vector);








void *
fs_vector_insert (FishSoundVector * vector, void * data);






FishSoundVector *
fs_vector_remove (FishSoundVector * vector, void * data);

#endif 
