






























#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)db.c	8.4 (Berkeley) 2/21/94";
#endif 

#ifndef __DBINTERFACE_PRIVATE
#define __DBINTERFACE_PRIVATE
#endif
#ifdef macintosh
#include <unix.h>
#else
#include <sys/types.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>

#include "mcom_db.h"


int all_databases_locked_closed = 0;




void 
dbSetOrClearDBLock(DBLockFlagEnum type)
{
	if(type == LockOutDatabase)
		all_databases_locked_closed = 1;
	else
		all_databases_locked_closed = 0;
}

DB *
dbopen(const char *fname, int flags,int mode, DBTYPE type, const void *openinfo)
{

	

	if(all_databases_locked_closed && fname)
	  {
		errno = EINVAL;
		return(NULL);
	  }

#define	DB_FLAGS	(DB_LOCK | DB_SHMEM | DB_TXN)


#if 0  
#define	USE_OPEN_FLAGS							\
	(O_CREAT | O_EXCL | O_EXLOCK | O_NONBLOCK | O_RDONLY |		\
	 O_RDWR | O_SHLOCK | O_TRUNC)
#else
#define	USE_OPEN_FLAGS							\
	(O_CREAT | O_EXCL  | O_RDONLY |		\
	 O_RDWR | O_TRUNC)
#endif

	if ((flags & ~(USE_OPEN_FLAGS | DB_FLAGS)) == 0)
		switch (type) {
	
#if 0
		case DB_BTREE:
			return (__bt_open(fname, flags & USE_OPEN_FLAGS,
			    mode, openinfo, flags & DB_FLAGS));
		case DB_RECNO:
			return (__rec_open(fname, flags & USE_OPEN_FLAGS,
			    mode, openinfo, flags & DB_FLAGS));
#endif

		case DB_HASH:
			return (__hash_open(fname, flags & USE_OPEN_FLAGS,
			    mode, (const HASHINFO *)openinfo, flags & DB_FLAGS));
		default:
			break;
		}
	errno = EINVAL;
	return (NULL);
}

static int
__dberr()
{
	return (RET_ERROR);
}







void
__dbpanic(DB *dbp)
{
	
	dbp->del = (int (*)(const struct __db *, const DBT *, uint))__dberr;
	dbp->fd = (int (*)(const struct __db *))__dberr;
	dbp->get = (int (*)(const struct __db *, const DBT *, DBT *, uint))__dberr;
	dbp->put = (int (*)(const struct __db *, DBT *, const DBT *, uint))__dberr;
	dbp->seq = (int (*)(const struct __db *, DBT *, DBT *, uint))__dberr;
	dbp->sync = (int (*)(const struct __db *, uint))__dberr;
}
