


























#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "hb.h"

#ifdef HAVE_GLIB
#include <glib.h>
#endif
#include <stdlib.h>
#include <stdio.h>

HB_BEGIN_DECLS


int
main (int argc, char **argv)
{
  hb_blob_t *blob = NULL;
  hb_face_t *face = NULL;

  if (argc != 2) {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    exit (1);
  }

  
  {
    const char *font_data;
    unsigned int len;
    hb_destroy_func_t destroy;
    void *user_data;

#ifdef HAVE_GLIB
    GMappedFile *mf = g_mapped_file_new (argv[1], FALSE, NULL);
    font_data = g_mapped_file_get_contents (mf);
    len = g_mapped_file_get_length (mf);
    destroy = (hb_destroy_func_t) g_mapped_file_unref;
    user_data = (void *) mf;
#else
    FILE *f = fopen (argv[1], "rb");
    fseek (f, 0, SEEK_END);
    len = ftell (f);
    fseek (f, 0, SEEK_SET);
    font_data = (const char *) malloc (len);
    if (!font_data) len = 0;
    len = fread ((char *) font_data, 1, len, f);
    destroy = free;
    user_data = (void *) font_data;
    fclose (f);
#endif

    blob = hb_blob_create (font_data, len,
			   HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE,
			   destroy, user_data);
  }

  
  face = hb_face_create_for_data (blob, 0 );

  

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
