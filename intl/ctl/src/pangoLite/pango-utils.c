







































#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "pango-utils.h"

#ifdef HAVE_FRIBIDI
#include <fribidi/fribidi.h>
#endif

#ifndef HAVE_FLOCKFILE
#define flockfile(f) (void)1
#define funlockfile(f) (void)1
#define getc_unlocked(f) getc(f)
#endif 

#ifdef G_OS_WIN32
#include <sys/types.h>
#define STRICT
#include <windows.h>
#endif

#define UTF8_COMPUTE(Char, Mask, Len)	\
  if (Char < 128)	{ \
    Len = 1; \
    Mask = 0x7f;	\
  }	\
  else if ((Char & 0xe0) == 0xc0)	{ \
    Len = 2;	\
    Mask = 0x1f;	\
  }	\
  else if ((Char & 0xf0) == 0xe0)	{ \
    Len = 3; \
    Mask = 0x0f; \
  }	\
  else if ((Char & 0xf8) == 0xf0)	{ \
    Len = 4; \
    Mask = 0x07; \
  }	\
  else if ((Char & 0xfc) == 0xf8)	{ \
    Len = 5; \
    Mask = 0x03; \
  }	\
  else if ((Char & 0xfe) == 0xfc)	{ \
    Len = 6; \
    Mask = 0x01; \
  }	\
  else \
   Len = -1;

#define UTF8_LENGTH(Char) \
  ((Char) < 0x80 ? 1 : \
   ((Char) < 0x800 ? 2 :  \
    ((Char) < 0x10000 ? 3 : \
     ((Char) < 0x200000 ? 4 : \
      ((Char) < 0x4000000 ? 5 : 6)))))

#define UTF8_GET(Result, Chars, Count, Mask, Len)	\
  (Result) = (Chars)[0] & (Mask);	\
  for ((Count) = 1; (Count) < (Len); ++(Count))	{ \
    if (((Chars)[(Count)] & 0xc0) != 0x80) { \
	     (Result) = -1; \
	     break; \
	  }	\
    (Result) <<= 6;	\
    (Result) |= ((Chars)[(Count)] & 0x3f); \
  }

#define UNICODE_VALID(Char) \
  ((Char) < 0x110000 && ((Char) < 0xD800 || (Char) >= 0xE000) && \
   (Char) != 0xFFFE && (Char) != 0xFFFF)









char *
pangolite_trim_string (const char *str)
{
  int len;

  g_return_val_if_fail (str != NULL, NULL);
  
  while (*str && isspace (*str))
    str++;

  len = strlen (str);
  while (len > 0 && isspace (str[len-1]))
    len--;

  return g_strndup (str, len);
}










char **
pangolite_split_file_list (const char *str)
{
  int i = 0;
  int j;
  char **files;

  files = g_strsplit (str, G_SEARCHPATH_SEPARATOR_S, -1);

  while (files[i])
    {
      char *file = pangolite_trim_string (files[i]);

      
      if (file[0] == '\0')
	{
	  g_free(file);
	  g_free (files[i]);
	  
	  for (j = i + 1; files[j]; j++)
	    files[j - 1] = files[j];
	  
	  files[j - 1] = NULL;

	  continue;
	}
#ifndef G_OS_WIN32
      





      if (file[0] == '~' && file[1] == G_DIR_SEPARATOR)
	{
	  char *tmp = g_strconcat (g_get_home_dir(), file + 1, NULL);
	  g_free (file);
	  file = tmp;
	}
#endif
      g_free (files[i]);
      files[i] = file;
	
      i++;
    }

  return files;
}


















gint
pangolite_read_line (FILE *stream, GString *str)
{
  gboolean quoted = FALSE;
  gboolean comment = FALSE;
  int n_read = 0;
  int lines = 1;
  
  flockfile (stream);

  g_string_truncate (str, 0);
  
  while (1)
    {
      int c;
      
      c = getc_unlocked (stream);

      if (c == EOF)
	{
	  if (quoted)
	    g_string_append_c (str, '\\');
	  
	  goto done;
	}
      else
	n_read++;

      if (quoted)
	{
	  quoted = FALSE;
	  
	  switch (c)
	    {
	    case '#':
	      g_string_append_c (str, '#');
	      break;
	    case '\r':
	    case '\n':
	      {
		int next_c = getc_unlocked (stream);

		if (!(next_c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);

		lines++;
		
		break;
	      }
	    default:
	      g_string_append_c (str, '\\');	      
	      g_string_append_c (str, c);
	    }
	}
      else
	{
	  switch (c)
	    {
	    case '#':
	      comment = TRUE;
	      break;
	    case '\\':
	      if (!comment)
		quoted = TRUE;
	      break;
	    case '\n':
	      {
		int next_c = getc_unlocked (stream);

		if (!(c == EOF ||
		      (c == '\r' && next_c == '\n') ||
		      (c == '\n' && next_c == '\r')))
		  ungetc (next_c, stream);

		goto done;
	      }
	    default:
	      if (!comment)
		g_string_append_c (str, c);
	    }
	}
    }

 done:

  funlockfile (stream);

  return (n_read > 0) ? lines : 0;
}










gboolean
pangolite_skip_space (const char **pos)
{
  const char *p = *pos;
  
  while (isspace (*p))
    p++;

  *pos = p;

  return !(*p == '\0');
}












gboolean
pangolite_scan_word (const char **pos, GString *out)
{
  const char *p = *pos;

  while (isspace (*p))
    p++;
  
  if (!((*p >= 'A' && *p <= 'Z') ||
	(*p >= 'a' && *p <= 'z') ||
	*p == '_'))
    return FALSE;

  g_string_truncate (out, 0);
  g_string_append_c (out, *p);
  p++;

  while ((*p >= 'A' && *p <= 'Z') ||
	 (*p >= 'a' && *p <= 'z') ||
	 (*p >= '0' && *p <= '9') ||
	 *p == '_')
    {
      g_string_append_c (out, *p);
      p++;
    }

  *pos = p;

  return TRUE;
}













gboolean
pangolite_scan_string (const char **pos, GString *out)
{
  const char *p = *pos;
  
  while (isspace (*p))
    p++;

  if (!*p)
    return FALSE;
  else if (*p == '"')
    {
      gboolean quoted = FALSE;
      g_string_truncate (out, 0);

      p++;

      while (TRUE)
	{
	  if (quoted)
	    {
	      int c = *p;
	      
	      switch (c)
		{
		case '\0':
		  return FALSE;
		case 'n':
		  c = '\n';
		  break;
		case 't':
		  c = '\t';
		  break;
		}
	      
	      quoted = FALSE;
	      g_string_append_c (out, c);
	    }
	  else
	    {
	      switch (*p)
		{
		case '\0':
		  return FALSE;
		case '\\':
		  quoted = TRUE;
		  break;
		case '"':
		  p++;
		  goto done;
		default:
		  g_string_append_c (out, *p);
		  break;
		}
	    }
	  p++;
	}
    done:
      ;
    }
  else
    {
      g_string_truncate (out, 0);

      while (*p && !isspace (*p))
	{
	  g_string_append_c (out, *p);
	  p++;
	}
    }

  *pos = p;

  return TRUE;
}

gboolean
pangolite_scan_int (const char **pos, int *out)
{
  int i = 0;
  char buf[32];
  const char *p = *pos;

  while (isspace (*p))
    p++;
  
  if (*p < '0' || *p > '9')
    return FALSE;

  while ((*p >= '0') && (*p <= '9') && i < sizeof(buf))
    {
      buf[i] = *p;
      i++;
      p++;
    }

  if (i == sizeof(buf))
    return FALSE;
  else
    buf[i] = '\0';

  *out = atoi (buf);

  return TRUE;
}

static GHashTable *config_hash = NULL;

static void
read_config_file (const char *filename, gboolean enoent_error)
{
  FILE *file;
    
  GString *line_buffer;
  GString *tmp_buffer1;
  GString *tmp_buffer2;
  char *errstring = NULL;
  const char *pos;
  char *section = NULL;
  int line = 0;

  file = fopen (filename, "r");
  if (!file)
    {
      if (errno != ENOENT || enoent_error)
	fprintf (stderr, "Pangolite:%s: Error opening config file: %s\n",
		 filename, g_strerror (errno));
      return;
    }
  
  line_buffer = g_string_new (NULL);
  tmp_buffer1 = g_string_new (NULL);
  tmp_buffer2 = g_string_new (NULL);

  while (pangolite_read_line (file, line_buffer))
    {
      line++;

      pos = line_buffer->str;
      if (!pangolite_skip_space (&pos))
	continue;

      if (*pos == '[')	
	{
	  pos++;
	  if (!pangolite_skip_space (&pos) ||
	      !pangolite_scan_word (&pos, tmp_buffer1) ||
	      !pangolite_skip_space (&pos) ||
	      *(pos++) != ']' ||
	      pangolite_skip_space (&pos))
	    {
	      errstring = g_strdup ("Error parsing [SECTION] declaration");
	      goto error;
	    }

	  section = g_strdup (tmp_buffer1->str);
	}
      else			
	{
	  gboolean empty = FALSE;
	  gboolean append = FALSE;
	  char *k, *v;

	  if (!section)
	    {
	      errstring = g_strdup ("A [SECTION] declaration must occur first");
	      goto error;
	    }

	  if (!pangolite_scan_word (&pos, tmp_buffer1) ||
	      !pangolite_skip_space (&pos))
	    {
	      errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
	      goto error;
	    }
	  if (*pos == '+')
	    {
	      append = TRUE;
	      pos++;
	    }

	  if (*(pos++) != '=')
	    {
	      errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
	      goto error;
	    }
	    
	  if (!pangolite_skip_space (&pos))
	    {
	      empty = TRUE;
	    }
	  else
	    {
	      if (!pangolite_scan_string (&pos, tmp_buffer2))
		{
		  errstring = g_strdup ("Error parsing value string");
		  goto error;
		}
	      if (pangolite_skip_space (&pos))
		{
		  errstring = g_strdup ("Junk after value string");
		  goto error;
		}
	    }

	  g_string_prepend_c (tmp_buffer1, '/');
	  g_string_prepend (tmp_buffer1, section);

	  
	  if (g_hash_table_lookup_extended (config_hash, tmp_buffer1->str,
					    (gpointer *)&k, (gpointer *)&v))
	    {
	      g_free (k);
	      if (append)
		{
		  g_string_prepend (tmp_buffer2, v);
		  g_free (v);
		}
	    }
	      
	  if (!empty)
	    {
	      g_hash_table_insert (config_hash,
				   g_strdup (tmp_buffer1->str),
				   g_strdup (tmp_buffer2->str));
	    }
	}
    }
      
  if (ferror (file))
    errstring = g_strdup ("g_strerror(errno)");
  
 error:

  if (errstring)
    {
      fprintf (stderr, "Pangolite:%s:%d: %s\n", filename, line, errstring);
      g_free (errstring);
    }
      
  g_free (section);
  g_string_free (line_buffer, TRUE);
  g_string_free (tmp_buffer1, TRUE);
  g_string_free (tmp_buffer2, TRUE);

  fclose (file);
}

static void
read_config ()
{
  if (!config_hash)
    {
      char *filename;
      char *home;
      
      config_hash = g_hash_table_new (g_str_hash, g_str_equal);
      filename = g_strconcat (pangolite_get_sysconf_subdirectory (),
			      G_DIR_SEPARATOR_S "pangoliterc",
			      NULL);
      read_config_file (filename, FALSE);
      g_free (filename);

      home = g_get_home_dir ();
      if (home && *home)
	{
	  filename = g_strconcat (home,
				  G_DIR_SEPARATOR_S ".pangoliterc",
				  NULL);
	  read_config_file (filename, FALSE);
	  g_free (filename);
	}

      filename = g_getenv ("PANGO_RC_FILE");
      if (filename)
	read_config_file (filename, TRUE);
    }
}












char *
pangolite_config_key_get (const char *key)
{
  g_return_val_if_fail (key != NULL, NULL);
  
  read_config ();

  return g_strdup (g_hash_table_lookup (config_hash, key));
}

G_CONST_RETURN char *
pangolite_get_sysconf_subdirectory (void)
{
#ifdef G_OS_WIN32
  static gchar *result = NULL;

  if (result == NULL)
    result = g_win32_get_package_installation_subdirectory
      ("pangolite", g_strdup_printf ("pangolite-%s.dll", PANGO_VERSION), "etc\\pangolite");

  return result;
  
  
  
#else
  char *tmp = getenv("MOZILLA_FIVE_HOME");
  return tmp;
#endif
}

G_CONST_RETURN char *
pangolite_get_lib_subdirectory (void)
{
#ifdef G_OS_WIN32
  static gchar *result = NULL;

  if (result == NULL)
    result = g_win32_get_package_installation_subdirectory
      ("pangolite", g_strdup_printf ("pangolite-%s.dll", PANGO_VERSION), "lib\\pangolite");

  return result;
  
  
  
#else
  char *tmp = getenv("MOZILLA_FIVE_HOME");
  return tmp;
  
#endif
}










gunichar
g_utf8_get_char (const gchar *p)
{
  int i, mask = 0, len;
  gunichar result;
  unsigned char c = (unsigned char) *p;

  UTF8_COMPUTE (c, mask, len);
  if (len == -1)
    return (gunichar)-1;
  UTF8_GET (result, p, i, mask, len);

  return result;
}


#ifdef HAVE_FRIBIDI
void 
pangolite_log2vis_get_embedding_levels (gunichar       *str,
                                    int            len,
                                    PangoliteDirection *pbase_dir,
                                    guint8         *embedding_level_list)
{
  FriBidiCharType fribidi_base_dir;
  
  fribidi_base_dir = (*pbase_dir == PANGO_DIRECTION_LTR) ?
    FRIBIDI_TYPE_L : FRIBIDI_TYPE_R;
  
  fribidi_log2vis_get_embedding_levels(str, len, &fribidi_base_dir,
                                       embedding_level_list);
  
  *pbase_dir = (fribidi_base_dir == FRIBIDI_TYPE_L) ?  
    PANGO_DIRECTION_LTR : PANGO_DIRECTION_RTL;
}

gboolean 
pangolite_get_mirror_char (gunichar ch, gunichar *mirrored_ch)
{
  return fribidi_get_mirror_char (ch, mirrored_ch); 
}
#endif 
