





































#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mar.h"
#include "mar_private.h"

#ifdef XP_WIN
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <unistd.h>
#endif

struct MarItemStack {
  void *head;
  PRUint32 size_used;
  PRUint32 size_allocated;
  PRUint32 last_offset;
};





static int mar_push(struct MarItemStack *stack, PRUint32 length, PRUint32 flags,
                    const char *name) {
  int namelen;
  PRUint32 n_offset, n_length, n_flags;
  PRUint32 size;
  char *data;
  
  namelen = strlen(name);
  size = MAR_ITEM_SIZE(namelen);

  if (stack->size_allocated - stack->size_used < size) {
    
    size_t size_needed = ROUND_UP(stack->size_used + size, BLOCKSIZE);
    stack->head = realloc(stack->head, size_needed);
    if (!stack->head)
      return -1;
    stack->size_allocated = size_needed;
  }

  data = (((char *) stack->head) + stack->size_used);

  n_offset = htonl(stack->last_offset);
  n_length = htonl(length);
  n_flags = htonl(flags);

  memcpy(data, &n_offset, sizeof(n_offset));
  data += sizeof(n_offset);

  memcpy(data, &n_length, sizeof(n_length));
  data += sizeof(n_length);

  memcpy(data, &n_flags, sizeof(n_flags));
  data += sizeof(n_flags);

  memcpy(data, name, namelen + 1);
  
  stack->size_used += size;
  stack->last_offset += length;
  return 0;
}

static int mar_concat_file(FILE *fp, const char *path) {
  FILE *in;
  char buf[BLOCKSIZE];
  size_t len;
  int rv = 0;

  in = fopen(path, "rb");
  if (!in)
    return -1;

  while ((len = fread(buf, 1, BLOCKSIZE, in)) > 0) {
    if (fwrite(buf, len, 1, fp) != 1) {
      rv = -1;
      break;
    }
  }

  fclose(in);
  return rv;
}

int mar_create(const char *dest, int num_files, char **files) {
  struct MarItemStack stack;
  PRUint32 offset_to_index = 0, size_of_index;
  struct stat st;
  FILE *fp;
  int i, rv = -1;

  memset(&stack, 0, sizeof(stack));

  fp = fopen(dest, "wb");
  if (!fp) {
    fprintf(stderr, "ERROR: could not create target file: %s\n", dest);
    return -1;
  }

  if (fwrite(MAR_ID, MAR_ID_SIZE, 1, fp) != 1)
    goto failure;
  if (fwrite(&offset_to_index, sizeof(PRUint32), 1, fp) != 1)
    goto failure;

  stack.last_offset = MAR_ID_SIZE + sizeof(PRUint32);

  for (i = 0; i < num_files; ++i) {
    if (stat(files[i], &st)) {
      fprintf(stderr, "ERROR: file not found: %s\n", files[i]);
      goto failure;
    }

    if (mar_push(&stack, st.st_size, st.st_mode & 0777, files[i]))
      goto failure;

    
    if (mar_concat_file(fp, files[i]))
      goto failure;
  }

  
  size_of_index = htonl(stack.size_used);
  if (fwrite(&size_of_index, sizeof(size_of_index), 1, fp) != 1)
    goto failure;
  if (fwrite(stack.head, stack.size_used, 1, fp) != 1)
    goto failure;

  
  offset_to_index = htonl(stack.last_offset);
  if (fseek(fp, MAR_ID_SIZE, SEEK_SET))
    goto failure;
  if (fwrite(&offset_to_index, sizeof(offset_to_index), 1, fp) != 1)
    goto failure;

  rv = 0;
failure: 
  if (stack.head)
    free(stack.head);
  fclose(fp);
  if (rv)
    remove(dest);
  return rv;
}
