








































#define JAR_SIZE 256

#include "jar.h"
#include "jarint.h"
#include "jarfile.h"


#include "jzlib.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include "sys/stat.h"
#endif

#include "sechash.h"	

PR_STATIC_ASSERT(46 == sizeof(struct ZipCentral));
PR_STATIC_ASSERT(30 == sizeof(struct ZipLocal));
PR_STATIC_ASSERT(22 == sizeof(struct ZipEnd));
PR_STATIC_ASSERT(512 == sizeof(union TarEntry));


static int 
jar_guess_jar(const char *filename, JAR_FILE fp);

static int 
jar_inflate_memory(unsigned int method, long *length, long expected_out_len, 
                   char **data);

static int 
jar_physical_extraction(JAR_FILE fp, char *outpath, long offset, long length);

static int 
jar_physical_inflate(JAR_FILE fp, char *outpath, long offset, long length, 
                     unsigned int method);

static int 
jar_verify_extract(JAR *jar, char *path, char *physical_path);

static JAR_Physical *
jar_get_physical(JAR *jar, char *pathname);

static int 
jar_extract_manifests(JAR *jar, jarArch format, JAR_FILE fp);

static int 
jar_extract_mf(JAR *jar, jarArch format, JAR_FILE fp, char *ext);



static int 
jar_gen_index(JAR *jar, jarArch format, JAR_FILE fp);

static int 
jar_listtar(JAR *jar, JAR_FILE fp);

static int 
jar_listzip(JAR *jar, JAR_FILE fp);



static int 
dosdate(char *date, const char *s);

static int 
dostime(char *time, const char *s);

#ifdef NSS_X86_OR_X64
#define x86ShortToUint32(ii)   ((const PRUint32)*((const PRUint16 *)(ii)))
#define x86LongToUint32(ii)    (*(const PRUint32 *)(ii))
#else
static PRUint32
x86ShortToUint32(const void *ii);

static PRUint32
x86LongToUint32(const void *ll);
#endif

static long 
octalToLong(const char *s);









int 
JAR_pass_archive(JAR *jar, jarArch format, char *filename, const char *url)
{
    JAR_FILE fp;
    int status = 0;

    if (filename == NULL)
	return JAR_ERR_GENERAL;

    if ((fp = JAR_FOPEN (filename, "rb")) != NULL) {
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
    
    return JAR_ERR_FNF;
}







int 
JAR_pass_archive_unverified(JAR *jar, jarArch format, char *filename, 
                            const char *url)
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
    }
    
    return JAR_ERR_FNF;
}










int 
JAR_verified_extract(JAR *jar, char *path, char *outpath)
{
    int status = JAR_extract (jar, path, outpath);

    if (status >= 0)
	return jar_verify_extract(jar, path, outpath);
    return status;
}

int 
JAR_extract(JAR *jar, char *path, char *outpath)
{
    int result;
    JAR_Physical *phy;

    if (jar->fp == NULL && jar->filename) {
	jar->fp = (FILE*)JAR_FOPEN (jar->filename, "rb");
    }
    if (jar->fp == NULL) {
	
	return JAR_ERR_FNF;
    }

    phy = jar_get_physical (jar, path);
    if (phy) {
	if (phy->compression != 0 && phy->compression != 8) {
	    
	    result = JAR_ERR_CORRUPT;
	}
	if (phy->compression == 0) {
	    result = jar_physical_extraction
		((PRFileDesc*)jar->fp, outpath, phy->offset, phy->length);
	} else {
	    result = jar_physical_inflate((PRFileDesc*)jar->fp, outpath, 
	                                  phy->offset, phy->length,
	                                  (unsigned int) phy->compression);
	}

#if defined(XP_UNIX) || defined(XP_BEOS)
	if (phy->mode)
	    chmod (outpath, 0400 | (mode_t) phy->mode);
#endif
    } else {
	
	result = JAR_ERR_PNF;
    }
    return result;
}










#define CHUNK 32768

static int 
jar_physical_extraction(JAR_FILE fp, char *outpath, long offset, long length)
{
    JAR_FILE out;
    char *buffer = (char *)PORT_ZAlloc(CHUNK);
    int status = 0;

    if (buffer == NULL)
	return JAR_ERR_MEMORY;

    if ((out = JAR_FOPEN (outpath, "wb")) != NULL) {
	long at = 0;

	JAR_FSEEK (fp, offset, (PRSeekWhence)0);
	while (at < length) {
	    long chunk = (at + CHUNK <= length) ? CHUNK : length - at;
	    if (JAR_FREAD (fp, buffer, chunk) != chunk) {
		status = JAR_ERR_DISK;
		break;
	    }
	    at += chunk;
	    if (JAR_FWRITE (out, buffer, chunk) < chunk) {
		
		status = JAR_ERR_DISK;
		break;
	    }
	}
	JAR_FCLOSE (out);
    } else {
	
	status = JAR_ERR_DISK;
    }
    PORT_Free (buffer);
    return status;
}










#define ICHUNK 8192
#define OCHUNK 32768

static int 
jar_physical_inflate(JAR_FILE fp, char *outpath, long offset, long length, 
                     unsigned int method)
{
    char *inbuf, *outbuf;
    int status = 0;
    z_stream zs;
    JAR_FILE out;

    
    if ((inbuf = (char *)PORT_ZAlloc(ICHUNK + 1)) == NULL)
	return JAR_ERR_MEMORY;

    if ((outbuf = (char *)PORT_ZAlloc(OCHUNK)) == NULL) {
	PORT_Free (inbuf);
	return JAR_ERR_MEMORY;
    }

    PORT_Memset (&zs, 0, sizeof (zs));
    status = inflateInit2 (&zs, -MAX_WBITS);
    if (status != Z_OK) {
	PORT_Free (inbuf);
	PORT_Free (outbuf);
	return JAR_ERR_GENERAL;
    }

    if ((out = JAR_FOPEN (outpath, "wb")) != NULL) {
	long at = 0;

	JAR_FSEEK (fp, offset, (PRSeekWhence)0);
	while (at < length) {
	    long chunk = (at + ICHUNK <= length) ? ICHUNK : length - at;
	    unsigned long tin;

	    if (JAR_FREAD (fp, inbuf, chunk) != chunk) {
		
		JAR_FCLOSE (out);
		PORT_Free (inbuf);
		PORT_Free (outbuf);
		return JAR_ERR_CORRUPT;
	    }
	    at += chunk;
	    if (at == length) {
		
		inbuf[chunk++] = 0xDD;
	    }
	    zs.next_in = (Bytef *) inbuf;
	    zs.avail_in = chunk;
	    zs.avail_out = OCHUNK;
	    tin = zs.total_in;
	    while ((zs.total_in - tin < chunk) || (zs.avail_out == 0)) {
		unsigned long prev_total = zs.total_out;
		unsigned long ochunk;

		zs.next_out = (Bytef *) outbuf;
		zs.avail_out = OCHUNK;
		status = inflate (&zs, Z_NO_FLUSH);
		if (status != Z_OK && status != Z_STREAM_END) {
		    
		    JAR_FCLOSE (out);
		    PORT_Free (inbuf);
		    PORT_Free (outbuf);
		    return JAR_ERR_CORRUPT;
		}
		ochunk = zs.total_out - prev_total;
		if (JAR_FWRITE (out, outbuf, ochunk) < ochunk) {
		    
		    status = JAR_ERR_DISK;
		    break;
		}
		if (status == Z_STREAM_END)
		    break;
	    }
	}
	JAR_FCLOSE (out);
	status = inflateEnd (&zs);
    } else {
	
	status = JAR_ERR_DISK;
    }
    PORT_Free (inbuf);
    PORT_Free (outbuf);
    return status;
}








static int 
jar_inflate_memory(unsigned int method, long *length, long expected_out_len, 
                   char **data)
{
    char *inbuf  = *data;
    char *outbuf = (char*)PORT_ZAlloc(expected_out_len);
    long insz    = *length;
    int status;
    z_stream zs;

    if (outbuf == NULL)
	return JAR_ERR_MEMORY;

    PORT_Memset(&zs, 0, sizeof zs);
    status = inflateInit2 (&zs, -MAX_WBITS);
    if (status < 0) {
	
	PORT_Free (outbuf);
	return JAR_ERR_GENERAL;
    }

    zs.next_in = (Bytef *) inbuf;
    zs.next_out = (Bytef *) outbuf;
    zs.avail_in = insz;
    zs.avail_out = expected_out_len;

    status = inflate (&zs, Z_FINISH);
    if (status != Z_OK && status != Z_STREAM_END) {
	
	PORT_Free (outbuf);
	return JAR_ERR_GENERAL;
    }

    status = inflateEnd (&zs);
    if (status != Z_OK) {
	
	PORT_Free (outbuf);
	return JAR_ERR_GENERAL;
    }
    PORT_Free(*data);
    *data = outbuf;
    *length = zs.total_out;
    return 0;
}







static int 
jar_verify_extract(JAR *jar, char *path, char *physical_path)
{
    int status;
    JAR_Digest dig;

    PORT_Memset (&dig, 0, sizeof dig);
    status = JAR_digest_file (physical_path, &dig);
    if (!status)
	status = JAR_verify_digest (jar, path, &dig);
    return status;
}








static JAR_Physical *
jar_get_physical(JAR *jar, char *pathname)
{
    ZZLink *link;
    ZZList *list = jar->phy;

    if (ZZ_ListEmpty (list))
	return NULL;

    for (link = ZZ_ListHead (list);
         !ZZ_ListIterDone (list, link);
         link = link->next) {
	JAR_Item *it = link->thing;

	if (it->type == jarTypePhy && 
	    it->pathname && !PORT_Strcmp (it->pathname, pathname)) {
	    JAR_Physical *phy = (JAR_Physical *)it->data;
	    return phy;
	}
    }
    return NULL;
}








static int 
jar_extract_manifests(JAR *jar, jarArch format, JAR_FILE fp)
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









static int 
jar_extract_mf(JAR *jar, jarArch format, JAR_FILE fp, char *ext)
{
    ZZLink *link;
    ZZList *list = jar->phy;
    int ret = 0;

    if (ZZ_ListEmpty (list))
	return JAR_ERR_PNF;

    for (link = ZZ_ListHead (list);
         ret >= 0 && !ZZ_ListIterDone (list, link);
         link = link->next) {
	JAR_Item *it = link->thing;

	if (it->type == jarTypePhy && 
	    !PORT_Strncmp (it->pathname, "META-INF", 8))
	{
	    JAR_Physical *phy = (JAR_Physical *) it->data;
	    char *fn = it->pathname + 8;
	    char *e;
	    char *manifest;
	    long length;
	    int num, status;

	    if (PORT_Strlen (it->pathname) < 8)
		continue;

	    if (*fn == '/' || *fn == '\\') 
	    	fn++;
	    if (*fn == 0) {
		
		continue;
	    }

	    
	    for (e = fn; *e && *e != '.'; e++)
		 ;

	    
	    if (*e == '.') 
	    	e++;
	    if (PORT_Strcasecmp (ext, e)) {
		
		continue;
	    }
	    if (phy->length == 0 || phy->length > 0xFFFF) {
		
		
		return JAR_ERR_CORRUPT;
	    }

	    
	    
	    manifest = (char *)PORT_ZAlloc(phy->length + 1);
	    if (!manifest) 
		return JAR_ERR_MEMORY;

	    JAR_FSEEK (fp, phy->offset, (PRSeekWhence)0);
	    num = JAR_FREAD (fp, manifest, phy->length);
	    if (num != phy->length) {
		
		PORT_Free (manifest);
		return JAR_ERR_CORRUPT;
	    }

	    if (phy->compression == 8) {
		length = phy->length;
		
		manifest[length++] = 0xDD;
		status = jar_inflate_memory((unsigned int)phy->compression, 
					     &length,  
					     phy->uncompressed_length, 
					     &manifest);
		if (status < 0) {
		    PORT_Free (manifest);
		    return status;
		}
	    } else if (phy->compression) {
		
		PORT_Free (manifest);
		return JAR_ERR_CORRUPT;
	    } else
		length = phy->length;

	    status = JAR_parse_manifest(jar, manifest, length, 
					it->pathname, "url");
	    PORT_Free (manifest);
	    if (status < 0)
		ret = status;
	    else
		++ret;
	} else if (it->type == jarTypePhy) {
	    
	}
    }
    return ret;
}








static int 
jar_gen_index(JAR *jar, jarArch format, JAR_FILE fp)
{
    int result = JAR_ERR_CORRUPT;

    JAR_FSEEK (fp, 0, (PRSeekWhence)0);
    switch (format) {
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








static int 
jar_listzip(JAR *jar, JAR_FILE fp)
{
    ZZLink  *ent;
    JAR_Item *it;
    JAR_Physical *phy;
    struct ZipLocal *Local     = PORT_ZNew(struct ZipLocal);
    struct ZipCentral *Central = PORT_ZNew(struct ZipCentral);
    struct ZipEnd *End         = PORT_ZNew(struct ZipEnd);

    int err = 0;
    long pos = 0L;
    unsigned int compression;
    unsigned int filename_len, extra_len;

    char filename[JAR_SIZE];
    char date[9], time[9];
    char sig[4];

    if (!Local || !Central || !End) {
	
	err = JAR_ERR_MEMORY;
	goto loser;
    }

    while (1) {
	PRUint32 sigVal;
	JAR_FSEEK (fp, pos, (PRSeekWhence)0);

	if (JAR_FREAD(fp, sig, sizeof sig) != sizeof sig) {
	    
	    err = JAR_ERR_CORRUPT;
	    goto loser;
	}

	JAR_FSEEK (fp, pos, (PRSeekWhence)0);
	sigVal = x86LongToUint32(sig);
	if (sigVal == LSIG) {
	    JAR_FREAD (fp, Local, sizeof *Local);

	    filename_len = x86ShortToUint32(Local->filename_len);
	    extra_len    = x86ShortToUint32(Local->extrafield_len);
	    if (filename_len >= JAR_SIZE) {
		
		err = JAR_ERR_CORRUPT;
		goto loser;
	    }

	    if (JAR_FREAD (fp, filename, filename_len) != filename_len) {
		
		err = JAR_ERR_CORRUPT;
		goto loser;
	    }
	    filename [filename_len] = 0;
	    
	    phy = PORT_ZNew(JAR_Physical);
	    if (phy == NULL) {
		err = JAR_ERR_MEMORY;
		goto loser;
	    }

	    

	    compression = x86ShortToUint32(Local->method);
	    phy->compression = 
	    	(compression >= 0 && compression <= 255) ? compression : 222;
		

	    phy->offset = pos + (sizeof *Local) + filename_len + extra_len;
	    phy->length = x86LongToUint32(Local->size);
	    phy->uncompressed_length = x86LongToUint32(Local->orglen);

	    dosdate (date, Local->date);
	    dostime (time, Local->time);

	    it = PORT_ZNew(JAR_Item);
	    if (it == NULL) {
		err = JAR_ERR_MEMORY;
		goto loser;
	    }

	    it->pathname = PORT_Strdup(filename);
	    it->type = jarTypePhy;
	    it->data = (unsigned char *) phy;
	    it->size = sizeof (JAR_Physical);

	    ent = ZZ_NewLink (it);
	    if (ent == NULL) {
		err = JAR_ERR_MEMORY;
		goto loser;
	    }

	    ZZ_AppendLink (jar->phy, ent);
	    pos = phy->offset + phy->length;
	} else if (sigVal == CSIG) {
	    unsigned int attr = 0;
	    if (JAR_FREAD(fp, Central, sizeof *Central) != sizeof *Central) {
		
		err = JAR_ERR_CORRUPT;
		goto loser;
	    }

#if defined(XP_UNIX) || defined(XP_BEOS)
	    

	    attr = Central->external_attributes [2]; 
	    if (attr) {
		
		filename_len = x86ShortToUint32(Central->filename_len);
		if (filename_len >= JAR_SIZE) {
		    
		    err = JAR_ERR_CORRUPT;
		    goto loser;
		}

		if (JAR_FREAD(fp, filename, filename_len) != filename_len) {
		    
		    err = JAR_ERR_CORRUPT;
		    goto loser;
		}
		filename [filename_len] = 0;

		
		phy = jar_get_physical (jar, filename);
		if (phy) {
		    
		    phy->mode = 0400 | attr;
		}
	    }
#endif
	    pos += sizeof(struct ZipCentral) 
	         + x86ShortToUint32(Central->filename_len)
		 + x86ShortToUint32(Central->commentfield_len)
		 + x86ShortToUint32(Central->extrafield_len);
	} else if (sigVal == ESIG) {
	    if (JAR_FREAD(fp, End, sizeof *End) != sizeof *End) {
		err = JAR_ERR_CORRUPT;
		goto loser;
	    }
	    break;
	} else {
	    
	    err = JAR_ERR_CORRUPT;
	    goto loser;
	}
    }

loser:
    if (Local) 
    	PORT_Free(Local);
    if (Central) 
    	PORT_Free(Central);
    if (End) 
    	PORT_Free(End);
    return err;
}








static int 
jar_listtar(JAR *jar, JAR_FILE fp)
{
    char *s;
    JAR_Physical *phy;
    long pos = 0L;
    long sz, mode;
    time_t when;
    union TarEntry tarball;

    while (1) {
	JAR_FSEEK (fp, pos, (PRSeekWhence)0);

	if (JAR_FREAD (fp, &tarball, sizeof tarball) < sizeof tarball)
	    break;

	if (!*tarball.val.filename)
	    break;

	when = octalToLong (tarball.val.time);
	sz   = octalToLong (tarball.val.size);
	mode = octalToLong (tarball.val.mode);

	
	s = tarball.val.filename;
	while (*s && *s != ' ') 
	    s++;
	*s = 0;

	
	phy = PORT_ZNew(JAR_Physical);
	if (phy == NULL)
	    return JAR_ERR_MEMORY;

	phy->compression = 0;
	phy->offset = pos + sizeof tarball;
	phy->length = sz;

	ADDITEM(jar->phy, jarTypePhy, tarball.val.filename, phy, 
	        sizeof *phy);

	
	sz = PR_ROUNDUP(sz,sizeof tarball);
	pos += sz + sizeof tarball;
    }

    return 0;
}








static int 
dosdate(char *date, const char *s)
{
    PRUint32 num = x86ShortToUint32(s);

    PR_snprintf(date, 9, "%02d-%02d-%02d", ((num >> 5) & 0x0F), (num & 0x1F), 
                                           ((num >> 9) + 80));
    return 0;
}








static int 
dostime (char *time, const char *s)
{
    PRUint32 num = x86ShortToUint32(s);

    PR_snprintf (time, 6, "%02d:%02d", ((num >> 11) & 0x1F), 
                                       ((num >>  5) & 0x3F));
    return 0;
}

#ifndef NSS_X86_OR_X64



static PRUint32
x86ShortToUint32(const void * v)
{
    const unsigned char *ii = (const unsigned char *)v;
    PRUint32 ret = (PRUint32)(ii[0]) | ((PRUint32)(ii[1]) << 8);
    return ret;
}




static PRUint32
x86LongToUint32(const void *v)
{
    const unsigned char *ll = (const unsigned char *)v;
    PRUint32 ret;

    ret = ((((PRUint32)(ll[0])) <<  0) |
	   (((PRUint32)(ll[1])) <<  8) |
	   (((PRUint32)(ll[2])) << 16) |
	   (((PRUint32)(ll[3])) << 24));
    return ret;
}
#endif






static long 
octalToLong(const char *s)
{
    long num = 0L;

    while (*s == ' ') 
    	s++;
    while (*s >= '0' && *s <= '7') {
	num <<= 3;
	num += *s++ - '0';
    }
    return num;
}









static int 
jar_guess_jar(const char *filename, JAR_FILE fp)
{
    PRInt32 len = PORT_Strlen(filename);
    const char *ext = filename + len - 4; 

    if (len >= 4 && !PL_strcasecmp(ext, ".tar"))
	return jarArchTar;
    return jarArchZip;
}
