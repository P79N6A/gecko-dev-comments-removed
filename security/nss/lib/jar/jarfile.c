









































#define JAR_SIZE 256

#include "jar.h"

#include "jarint.h"
#include "jarfile.h"


#include "jzlib.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include "sys/stat.h"
#endif

#include "sechash.h"	



static int jar_guess_jar (char *filename, JAR_FILE fp);

static int jar_inflate_memory 
   (unsigned int method, long *length, long expected_out_len, char ZHUGEP **data);

static int jar_physical_extraction 
   (JAR_FILE fp, char *outpath, long offset, long length);

static int jar_physical_inflate
   (JAR_FILE fp, char *outpath, long offset, 
         long length, unsigned int method);

static int jar_verify_extract 
   (JAR *jar, char *path, char *physical_path);

static JAR_Physical *jar_get_physical (JAR *jar, char *pathname);

static int jar_extract_manifests (JAR *jar, jarArch format, JAR_FILE fp);

static int jar_extract_mf (JAR *jar, jarArch format, JAR_FILE fp, char *ext);




static int jar_gen_index (JAR *jar, jarArch format, JAR_FILE fp);

static int jar_listtar (JAR *jar, JAR_FILE fp);

static int jar_listzip (JAR *jar, JAR_FILE fp);




static int dosdate (char *date, char *s);

static int dostime (char *time, char *s);

static unsigned int xtoint (unsigned char *ii);

static unsigned long xtolong (unsigned char *ll);

static long atoo (char *s);










int JAR_pass_archive
    (JAR *jar, jarArch format, char *filename, const char *url)
  {
  JAR_FILE fp;
  int status = 0;

  if (filename == NULL)
    return JAR_ERR_GENERAL;

  if ((fp = JAR_FOPEN (filename, "rb")) != NULL)
    {
    if (format == jarArchGuess)
      format = (jarArch)jar_guess_jar (filename, fp);

    jar->format = format;
    jar->url = url ? PORT_Strdup (url) : NULL;
    jar->filename = PORT_Strdup (filename);

    status = jar_gen_index (jar, format, fp);

    if (status == 0)
      status = jar_extract_manifests (jar, format, fp);

    JAR_FCLOSE (fp);

    if (status < 0)
      return status;

    
    return jar->valid;
    }
  else
    {
    
    return JAR_ERR_FNF;
    }
  }







int JAR_pass_archive_unverified
        (JAR *jar, jarArch format, char *filename, const char *url)
{
        JAR_FILE fp;
        int status = 0;

        if (filename == NULL) {
                return JAR_ERR_GENERAL;
        }

        if ((fp = JAR_FOPEN (filename, "rb")) != NULL) {
                if (format == jarArchGuess) {
                        format = (jarArch)jar_guess_jar (filename, fp);
                }

                jar->format = format;
                jar->url = url ? PORT_Strdup (url) : NULL;
                jar->filename = PORT_Strdup (filename);

                status = jar_gen_index (jar, format, fp);

                if (status == 0) {
                        status = jar_extract_mf(jar, format, fp, "mf");
                }

                JAR_FCLOSE (fp);

                if (status < 0) {
                        return status;
                }

                
                return jar->valid;
        } else {
                
                return JAR_ERR_FNF;
        }
}










int JAR_verified_extract
    (JAR *jar, char *path, char *outpath)
  {
  int status;

  status = JAR_extract (jar, path, outpath);

  if (status >= 0)
    return jar_verify_extract (jar, path, outpath);
  else
    return status;
  }

int JAR_extract
    (JAR *jar, char *path, char *outpath)
  {
  int result;

  JAR_Physical *phy;

  if (jar->fp == NULL && jar->filename)
    {
    jar->fp = (FILE*)JAR_FOPEN (jar->filename, "rb");
    }

  if (jar->fp == NULL)
    {
    
    return JAR_ERR_FNF;
    }

  phy = jar_get_physical (jar, path);

  if (phy)
    {
    if (phy->compression != 0 && phy->compression != 8)
      {
      
      result = JAR_ERR_CORRUPT;
      }

    if (phy->compression == 0)
      {
      result = jar_physical_extraction 
         ((PRFileDesc*)jar->fp, outpath, phy->offset, phy->length);
      }
    else
      {
      result = jar_physical_inflate 
         ((PRFileDesc*)jar->fp, outpath, phy->offset, phy->length, 
            (unsigned int) phy->compression);
      }

#if defined(XP_UNIX) || defined(XP_BEOS)
    if (phy->mode)
      chmod (outpath, 0400 | (mode_t) phy->mode);
#endif
    }
  else
    {
    
    result = JAR_ERR_PNF;
    }

  return result;
  }










#define CHUNK 32768

static int jar_physical_extraction 
     (JAR_FILE fp, char *outpath, long offset, long length)
  {
  JAR_FILE out;

  char *buffer;
  long at, chunk;

  int status = 0;

  buffer = (char *) PORT_ZAlloc (CHUNK);

  if (buffer == NULL)
    return JAR_ERR_MEMORY;

  if ((out = JAR_FOPEN (outpath, "wb")) != NULL)
    {
    at = 0;

    JAR_FSEEK (fp, offset, (PRSeekWhence)0);

    while (at < length)
      {
      chunk = (at + CHUNK <= length) ? CHUNK : length - at;

      if (JAR_FREAD (fp, buffer, chunk) != chunk)
        {
        status = JAR_ERR_DISK;
        break;
        }

      at += chunk;

      if (JAR_FWRITE (out, buffer, chunk) < chunk)
        {
        
        status = JAR_ERR_DISK;
        break;
        }
      }
    JAR_FCLOSE (out);
    }
  else
    {
    
    status = JAR_ERR_DISK;
    }

  PORT_Free (buffer);
  return status;
  }











#define ICHUNK 8192
#define OCHUNK 32768

static int jar_physical_inflate
     (JAR_FILE fp, char *outpath, long offset, 
          long length, unsigned int method)
  {
  z_stream zs;

  JAR_FILE out;

  long at, chunk;
  char *inbuf, *outbuf;

  int status = 0;

  unsigned long prev_total, ochunk, tin;

  
  if ((inbuf = (char *) PORT_ZAlloc (ICHUNK + 1)) == NULL)
    return JAR_ERR_MEMORY;

  if ((outbuf = (char *) PORT_ZAlloc (OCHUNK)) == NULL)
    {
    PORT_Free (inbuf);
    return JAR_ERR_MEMORY;
    }

  PORT_Memset (&zs, 0, sizeof (zs));
  status = inflateInit2 (&zs, -MAX_WBITS);

  if (status != Z_OK)
    {
    PORT_Free (inbuf);
    PORT_Free (outbuf);
    return JAR_ERR_GENERAL;
    }

  if ((out = JAR_FOPEN (outpath, "wb")) != NULL)
    {
    at = 0;

    JAR_FSEEK (fp, offset, (PRSeekWhence)0);

    while (at < length)
      {
      chunk = (at + ICHUNK <= length) ? ICHUNK : length - at;

      if (JAR_FREAD (fp, inbuf, chunk) != chunk)
        {
        
        JAR_FCLOSE (out);
        PORT_Free (inbuf);
        PORT_Free (outbuf);
        return JAR_ERR_CORRUPT;
        }

      at += chunk;

      if (at == length)
        {
        
        inbuf[chunk++] = 0xDD;
        }

      zs.next_in = (Bytef *) inbuf;
      zs.avail_in = chunk;
      zs.avail_out = OCHUNK;

      tin = zs.total_in;

      while ((zs.total_in - tin < chunk) || (zs.avail_out == 0))
        {
        prev_total = zs.total_out;

        zs.next_out = (Bytef *) outbuf;
        zs.avail_out = OCHUNK;

        status = inflate (&zs, Z_NO_FLUSH);

        if (status != Z_OK && status != Z_STREAM_END)
          {
          
          JAR_FCLOSE (out);
          PORT_Free (inbuf);
          PORT_Free (outbuf);
          return JAR_ERR_CORRUPT;
          }

	ochunk = zs.total_out - prev_total;

        if (JAR_FWRITE (out, outbuf, ochunk) < ochunk)
          {
          
          status = JAR_ERR_DISK;
          break;
          }

        if (status == Z_STREAM_END)
          break;
        }
      }

    JAR_FCLOSE (out);
    status = inflateEnd (&zs);
    }
  else
    {
    
    status = JAR_ERR_DISK;
    }

  PORT_Free (inbuf);
  PORT_Free (outbuf);

  return status;
  }









static int jar_inflate_memory 
     (unsigned int method, long *length, long expected_out_len, char ZHUGEP **data)
  {
  int status;
  z_stream zs;

  long insz, outsz;

  char *inbuf, *outbuf;

  inbuf =  *data;
  insz = *length;

  outsz = expected_out_len;
  outbuf = (char*)PORT_ZAlloc (outsz);

  if (outbuf == NULL)
    return JAR_ERR_MEMORY;

  PORT_Memset (&zs, 0, sizeof (zs));

  status = inflateInit2 (&zs, -MAX_WBITS);

  if (status < 0)
    {
    
    PORT_Free (outbuf);
    return JAR_ERR_GENERAL;
    }

  zs.next_in = (Bytef *) inbuf;
  zs.next_out = (Bytef *) outbuf;

  zs.avail_in = insz;
  zs.avail_out = outsz;

  status = inflate (&zs, Z_FINISH);

  if (status != Z_OK && status != Z_STREAM_END)
    {
    
    PORT_Free (outbuf);
    return JAR_ERR_GENERAL; 
    }

  status = inflateEnd (&zs);

  if (status != Z_OK)
    {
    
    PORT_Free (outbuf);
    return JAR_ERR_GENERAL;
    }

  PORT_Free (*data);

  *data = outbuf;
  *length = zs.total_out;

  return 0;
  }








static int jar_verify_extract (JAR *jar, char *path, char *physical_path)
  {
  int status;
  JAR_Digest dig;

  PORT_Memset (&dig, 0, sizeof (JAR_Digest));
  status = JAR_digest_file (physical_path, &dig);

  if (!status)
    status = JAR_verify_digest (jar, path, &dig);

  return status;
  }









static JAR_Physical *jar_get_physical (JAR *jar, char *pathname)
  {
  JAR_Item *it;

  JAR_Physical *phy;

  ZZLink *link;
  ZZList *list;

  list = jar->phy;

  if (ZZ_ListEmpty (list))
    return NULL;

  for (link = ZZ_ListHead (list);
       !ZZ_ListIterDone (list, link);
       link = link->next)
    {
    it = link->thing;
    if (it->type == jarTypePhy 
          && it->pathname && !PORT_Strcmp (it->pathname, pathname))
      {
      phy = (JAR_Physical *) it->data;
      return phy;
      }
    }

  return NULL;
  }









static int jar_extract_manifests (JAR *jar, jarArch format, JAR_FILE fp)
  {
  int status, signatures;

  if (format != jarArchZip && format != jarArchTar)
    return JAR_ERR_CORRUPT;

  if ((status = jar_extract_mf (jar, format, fp, "mf")) < 0)
    return status;
  if (!status) 
    return JAR_ERR_ORDER;
  if ((status = jar_extract_mf (jar, format, fp, "sf")) < 0)
    return status;
  if (!status) 
    return JAR_ERR_ORDER;
  if ((status = jar_extract_mf (jar, format, fp, "rsa")) < 0)
    return status;
  signatures = status;
  if ((status = jar_extract_mf (jar, format, fp, "dsa")) < 0)
    return status;
  if (!(signatures += status)) 
    return JAR_ERR_SIG;
  return 0;
  }










static int jar_extract_mf (JAR *jar, jarArch format, JAR_FILE fp, char *ext)
  {
  JAR_Item *it;

  JAR_Physical *phy;

  ZZLink *link;
  ZZList *list;

  char *fn, *e;
  char ZHUGEP *manifest;

  long length;
  int status, ret = 0, num;

  list = jar->phy;

  if (ZZ_ListEmpty (list))
    return JAR_ERR_PNF;

  for (link = ZZ_ListHead (list);
       ret >= 0 && !ZZ_ListIterDone (list, link);
       link = link->next)
    {
    it = link->thing;
    if (it->type == jarTypePhy 
          && !PORT_Strncmp (it->pathname, "META-INF", 8))
      {
      phy = (JAR_Physical *) it->data;

      if (PORT_Strlen (it->pathname) < 8)
        continue;

      fn = it->pathname + 8;
      if (*fn == '/' || *fn == '\\') fn++;

      if (*fn == 0)
        {
        
        continue;
        }

      
      for (e = fn; *e && *e != '.'; e++)
         ;

      
      if (*e == '.') e++;

      if (PORT_Strcasecmp (ext, e))
        {
        
        continue;
        }

      if (phy->length == 0 || phy->length > 0xFFFF)
        {
        
        
        return JAR_ERR_CORRUPT;
        }

      
      
      manifest = (char ZHUGEP *) PORT_ZAlloc (phy->length + 1);
      if (manifest)
        {
        JAR_FSEEK (fp, phy->offset, (PRSeekWhence)0);
        num = JAR_FREAD (fp, manifest, phy->length);

        if (num != phy->length)
          {
          
          PORT_Free (manifest);
          return JAR_ERR_CORRUPT;
          }

        if (phy->compression == 8)
          {
          length = phy->length;
          
          manifest[length++] = 0xDD;

          status = jar_inflate_memory ((unsigned int) phy->compression, &length,  phy->uncompressed_length, &manifest);

          if (status < 0)
            {
            PORT_Free (manifest);
            return status;
            }
          }
        else if (phy->compression)
          {
          
          PORT_Free (manifest);
          return JAR_ERR_CORRUPT;
          }
        else
          length = phy->length;

        status = JAR_parse_manifest 
           (jar, manifest, length, it->pathname, "url");

        PORT_Free (manifest);

        if (status < 0) 
	  ret = status;
	else
	  ++ret;
        }
      else
        return JAR_ERR_MEMORY;
      }
    else if (it->type == jarTypePhy)
      {
      
      }
    }

  return ret;
  }









static int jar_gen_index (JAR *jar, jarArch format, JAR_FILE fp)
  {
  int result = JAR_ERR_CORRUPT;
  JAR_FSEEK (fp, 0, (PRSeekWhence)0);

  switch (format)
    {
    case jarArchZip:
      result = jar_listzip (jar, fp);
      break;

    case jarArchTar:
      result = jar_listtar (jar, fp);
      break;

    case jarArchGuess:
    case jarArchNone:
      return JAR_ERR_GENERAL;
    }

  JAR_FSEEK (fp, 0, (PRSeekWhence)0);
  return result;
  }










static int jar_listzip (JAR *jar, JAR_FILE fp)
  {
  int err = 0;

  long pos = 0L;
  char filename [JAR_SIZE];

  char date [9], time [9];
  char sig [4];

  unsigned int compression;
  unsigned int filename_len, extra_len;

  struct ZipLocal *Local;
  struct ZipCentral *Central;
  struct ZipEnd *End;

  

  ZZLink  *ent;
  JAR_Item *it;
  JAR_Physical *phy;

  Local = (struct ZipLocal *) PORT_ZAlloc (30);
  Central = (struct ZipCentral *) PORT_ZAlloc (46);
  End = (struct ZipEnd *) PORT_ZAlloc (22);

  if (!Local || !Central || !End)
    {
    
    err = JAR_ERR_MEMORY;
    goto loser;
    }

  while (1)
    {
    JAR_FSEEK (fp, pos, (PRSeekWhence)0);

    if (JAR_FREAD (fp, (char *) sig, 4) != 4)
      {
      
      err = JAR_ERR_CORRUPT;
      goto loser;
      }

    JAR_FSEEK (fp, pos, (PRSeekWhence)0);

    if (xtolong ((unsigned char *)sig) == LSIG)
      {
      JAR_FREAD (fp, (char *) Local, 30);

      filename_len = xtoint ((unsigned char *) Local->filename_len);
      extra_len = xtoint ((unsigned char *) Local->extrafield_len);

      if (filename_len >= JAR_SIZE)
        {
        
        err = JAR_ERR_CORRUPT;
        goto loser;
        }

      if (JAR_FREAD (fp, filename, filename_len) != filename_len)
        {
        
        err = JAR_ERR_CORRUPT;
        goto loser;
        }

      filename [filename_len] = 0;

      

      phy = (JAR_Physical *) PORT_ZAlloc (sizeof (JAR_Physical));

      if (phy == NULL)
        {
        err = JAR_ERR_MEMORY;
        goto loser;
        }

      


      compression = xtoint ((unsigned char *) Local->method);
      phy->compression = compression >= 0 && 
              compression <= 255 ? compression : 222;

      phy->offset = pos + 30 + filename_len + extra_len;
      phy->length = xtolong ((unsigned char *) Local->size);
      phy->uncompressed_length = xtolong((unsigned char *) Local->orglen);

      dosdate (date, Local->date);
      dostime (time, Local->time);

      it = (JAR_Item*)PORT_ZAlloc (sizeof (JAR_Item));
      if (it == NULL)
        {
        err = JAR_ERR_MEMORY;
        goto loser;
        }

      it->pathname = PORT_Strdup (filename);

      it->type = jarTypePhy;

      it->data = (unsigned char *) phy;
      it->size = sizeof (JAR_Physical);

      ent = ZZ_NewLink (it);

      if (ent == NULL)
        {
        err = JAR_ERR_MEMORY;
        goto loser;
        }

      ZZ_AppendLink (jar->phy, ent);
 
      pos = phy->offset + phy->length;
      }
    else if (xtolong ( (unsigned char *)sig) == CSIG)
      {
      if (JAR_FREAD (fp, (char *) Central, 46) != 46)
        {
        
        err = JAR_ERR_CORRUPT;
        goto loser;
        }

#if defined(XP_UNIX) || defined(XP_BEOS)
      

        {
        unsigned int attr;

        
        attr = Central->external_attributes [2];

        if (attr)
          {
          

          filename_len = xtoint ((unsigned char *) Central->filename_len);

          if (filename_len >= JAR_SIZE)
            {
            
            err = JAR_ERR_CORRUPT;
            goto loser;
            }

          if (JAR_FREAD (fp, filename, filename_len) != filename_len)
            {
            
            err = JAR_ERR_CORRUPT;
            goto loser;
            }

          filename [filename_len] = 0;

          
          phy = jar_get_physical (jar, filename);

          if (phy)
            {
            
            phy->mode = 0400 | attr;
            }
          }
        }
#endif

      pos += 46 + xtoint ( (unsigned char *)Central->filename_len)
                + xtoint ( (unsigned char *)Central->commentfield_len)
                + xtoint ( (unsigned char *)Central->extrafield_len);
      }
    else if (xtolong ( (unsigned char *)sig) == ESIG)
      {
      if (JAR_FREAD (fp, (char *) End, 22) != 22)
        {
        err = JAR_ERR_CORRUPT;
        goto loser;
        }
      else
        break;
      }
    else
      {
      
      err = JAR_ERR_CORRUPT;
      goto loser;
      }
    }

loser:

  if (Local) PORT_Free (Local);
  if (Central) PORT_Free (Central);
  if (End) PORT_Free (End);

  return err;
  }









static int jar_listtar (JAR *jar, JAR_FILE fp)
  {
  long pos = 0L;

  long sz, mode;
  time_t when;
  union TarEntry tarball;

  char *s;

  

  JAR_Physical *phy;

  while (1)
    {
    JAR_FSEEK (fp, pos, (PRSeekWhence)0);

    if (JAR_FREAD (fp, (char *) &tarball, 512) < 512)
      break;

    if (!*tarball.val.filename)
      break;

    when = atoo (tarball.val.time);
    sz = atoo (tarball.val.size);
    mode = atoo (tarball.val.mode);


    

    s = tarball.val.filename;
    while (*s && *s != ' ') s++;
    *s = 0;


    

    phy = (JAR_Physical *) PORT_ZAlloc (sizeof (JAR_Physical));

    if (phy == NULL)
      return JAR_ERR_MEMORY;

    phy->compression = 0;
    phy->offset = pos + 512;
    phy->length = sz;

    ADDITEM (jar->phy, jarTypePhy, 
       tarball.val.filename, phy, sizeof (JAR_Physical));


    

    sz += 511;
    sz = (sz / 512) * 512;

    pos += sz + 512;
    }

  return 0;
  }









static int dosdate (char *date, char *s)
  {
  int num = xtoint ( (unsigned char *)s);

  PR_snprintf (date, 9, "%02d-%02d-%02d",
     ((num >> 5) & 0x0F), (num & 0x1F), ((num >> 9) + 80));

  return 0;
  }









static int dostime (char *time, char *s)
  {
  int num = xtoint ( (unsigned char *)s);

  PR_snprintf (time, 6, "%02d:%02d",
     ((num >> 11) & 0x1F), ((num >> 5) & 0x3F));

  return 0;
  }









static unsigned int xtoint (unsigned char *ii)
  {
  return (int) (ii [0]) | ((int) ii [1] << 8);
  }









static unsigned long xtolong (unsigned char *ll)
  {
  unsigned long ret;

  ret =  (
         (((unsigned long) ll [0]) <<  0) |
         (((unsigned long) ll [1]) <<  8) |
         (((unsigned long) ll [2]) << 16) |
         (((unsigned long) ll [3]) << 24)
         );

  return ret;
  }









static long atoo (char *s)
  {
  long num = 0L;

  while (*s == ' ') s++;

  while (*s >= '0' && *s <= '7')
    {
    num <<= 3;
    num += *s++ - '0';
    }

  return num;
  }










static int jar_guess_jar (char *filename, JAR_FILE fp)
  {
  char *ext;

  ext = filename + PORT_Strlen (filename) - 4;

  if (!PORT_Strcmp (ext, ".tar"))
    return jarArchTar;

  return jarArchZip;
  }
