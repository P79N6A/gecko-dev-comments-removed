




































#include "primpl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <sys/locking.h>






#define _PR_MD_WIN16_DELAY 1



















PRStdinRead   _pr_md_read_stdin = 0;
PRStdoutWrite _pr_md_write_stdout = 0;
PRStderrWrite _pr_md_write_stderr = 0;

PRStatus
PR_MD_RegisterW16StdioCallbacks( PRStdinRead inReadf, PRStdoutWrite outWritef, PRStderrWrite errWritef )
{
    _pr_md_write_stdout = outWritef;
    _pr_md_write_stderr = errWritef;
    _pr_md_read_stdin   = inReadf;
    
    return(PR_SUCCESS);
} 









PRInt32
_PR_MD_OPEN(const char *name, PRIntn osflags, int mode)
{
    PRInt32 file;
    int     access = O_BINARY;
    int     rights = 0;
    
    
    


    if (osflags & PR_RDONLY )
        access |= O_RDONLY;
    if (osflags & PR_WRONLY )
        access |= O_WRONLY;
    if (osflags & PR_RDWR )
        access |= O_RDWR;
    if (osflags & PR_CREATE_FILE )
    {
        access |= O_CREAT;
        rights |= S_IRWXU;
    }
    if (osflags & PR_TRUNCATE)
        access |= O_TRUNC;
    if (osflags & PR_APPEND)
        access |= O_APPEND;
    else
        access |= O_RDONLY;
        
    

        
    file = (PRInt32) sopen( name, access, SH_DENYNO, rights );
    if ( -1 == (PRInt32)file )
    {
        _PR_MD_MAP_OPEN_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return file;
}







PRInt32
_PR_MD_READ(PRFileDesc *fd, void *buf, PRInt32 len)
{
    PRInt32     rv;
    
    if ( (PR_GetDescType(fd) == PR_DESC_FILE) &&
         ( fd->secret->md.osfd == PR_StandardInput ) &&
         ( _pr_md_write_stdout ))
    {
        rv = (*_pr_md_read_stdin)( buf, len);    
    }
    else
    {
        rv = read( fd->secret->md.osfd, buf, len );
    }
    
    if ( rv == -1)
    {
        _PR_MD_MAP_READ_ERROR( errno );
    }
    
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return rv;
}











PRInt32
_PR_MD_WRITE(PRFileDesc *fd, const void *buf, PRInt32 len)
{
    PRInt32     rv;

    if ( (PR_GetDescType(fd) == PR_DESC_FILE))
    {
        switch ( fd->secret->md.osfd )
        {
            case  PR_StandardOutput :
                if ( _pr_md_write_stdout )
                    rv = (*_pr_md_write_stdout)( (void *)buf, len);
                else
                    rv = len; 
                break;
                
            case  PR_StandardError  :
                if ( _pr_md_write_stderr )
                    rv = (*_pr_md_write_stderr)( (void *)buf, len);    
                else
                    rv = len; 
                break;
                
            default:
                rv = write( fd->secret->md.osfd, buf, len );
                if ( rv == -1 )
                {
                    _PR_MD_MAP_WRITE_ERROR( errno );
                }
                break;
        }
    }
    else
    {
        rv = write( fd->secret->md.osfd, buf, len );
        if ( rv == -1 )
        {
            _PR_MD_MAP_WRITE_ERROR( errno );
        }
    }
    
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return rv;
} 









PRInt32
_PR_MD_LSEEK(PRFileDesc *fd, PRInt32 offset, int whence)
{
    PRInt32     rv;
    
    rv = lseek( fd->secret->md.osfd, offset, whence );
    if ( rv == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return( rv );
}





PRInt64
_PR_MD_LSEEK64( PRFileDesc *fd, PRInt64 offset, int whence )
{
    PRInt64 test;
    PRInt32 rv, off;
    LL_SHR(test, offset, 32);
    if (!LL_IS_ZERO(test))
    {
        PR_SetError(PR_FILE_TOO_BIG_ERROR, 0);
        LL_I2L(test, -1);
        return test;
    }
    LL_L2I(off, offset);
    rv = _PR_MD_LSEEK(fd, off, whence);
    LL_I2L(test, rv);
    return test;
} 








PRInt32
_PR_MD_FSYNC(PRFileDesc *fd)
{
    PRInt32     rv;
    
    rv = (PRInt32) fsync( fd->secret->md.osfd );
    if ( rv == -1 )
    {
        _PR_MD_MAP_FSYNC_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return(rv);
}








PRInt32
_PR_MD_CLOSE_FILE(PRInt32 osfd)
{
    PRInt32     rv;
    
    rv = (PRInt32) close( osfd );
    if ( rv == -1 )
    {
        _PR_MD_MAP_CLOSE_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return(rv);
} 



#define GetFileFromDIR(d)       (d)->d_entry.cFileName








void FlipSlashes(char *cp, int len)
{
    while (--len >= 0) {
    if (cp[0] == '/') {
        cp[0] = PR_DIRECTORY_SEPARATOR;
    }
    cp++;
    }
}









PRStatus
_PR_MD_OPEN_DIR(_MDDir *d, const char *name)
{
    d->dir = opendir( name );
    
    if ( d->dir == NULL )
    {
        _PR_MD_MAP_OPENDIR_ERROR( errno );
        return( PR_FAILURE );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return( PR_SUCCESS );
}







char *
_PR_MD_READ_DIR(_MDDir *d, PRIntn flags)
{
    struct dirent *de;
    int err;

	for (;;) 
    {
		de = readdir( d->dir );
		if ( de == NULL ) {
			_PR_MD_MAP_READDIR_ERROR( errno);
			return 0;
		}		
		if ((flags & PR_SKIP_DOT) &&
		    (de->d_name[0] == '.') && (de->d_name[1] == 0))
			continue;
		if ((flags & PR_SKIP_DOT_DOT) &&
		    (de->d_name[0] == '.') && (de->d_name[1] == '.') &&
		    (de->d_name[2] == 0))
			continue;
		break;
	}
    PR_Sleep( _PR_MD_WIN16_DELAY );    
	return de->d_name;
}






PRInt32
_PR_MD_CLOSE_DIR(_MDDir *d)
{
    PRInt32     rv;
    
    if ( d->dir ) 
    {
        rv = closedir( d->dir );
        if (rv != 0) 
        {
            _PR_MD_MAP_CLOSEDIR_ERROR( errno );
        }
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return rv;
}









PRInt32
_PR_MD_DELETE(const char *name)
{
    PRInt32     rv;
    
    rv = (PRInt32) remove( name );
    if ( rv != 0 )
    {
        _PR_MD_MAP_DELETE_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return(rv);
}









PRInt32
_PR_MD_STAT(const char *fn, struct stat *info)
{
    PRInt32     rv;
    
    rv = _stat(fn, (struct _stat *)info);
    if ( rv == -1 )
    {
        _PR_MD_MAP_STAT_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return( rv );
}








PRInt32
_PR_MD_GETFILEINFO(const char *fn, PRFileInfo *info)
{
    struct _stat sb;
    PRInt32 rv;
 
    if ( (rv = _stat(fn, &sb)) == 0 ) {
        if (info) {
            if (S_IFREG & sb.st_mode)
                info->type = PR_FILE_FILE ;
            else if (S_IFDIR & sb.st_mode)
                info->type = PR_FILE_DIRECTORY;
            else
                info->type = PR_FILE_OTHER;
            info->size = sb.st_size;
            LL_I2L(info->modifyTime, sb.st_mtime);
            LL_I2L(info->creationTime, sb.st_ctime);
        }
    }
    else
    {
        _PR_MD_MAP_STAT_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return rv;
}

PRInt32
_PR_MD_GETFILEINFO64(const char *fn, PRFileInfo64 *info)
{
    PRFileInfo info32;
    
    PRInt32 rv = _PR_MD_GETFILEINFO(fn, &info32);
    if (0 == rv)
    {
        info->type = info32.type;
        info->modifyTime = info32.modifyTime;
        info->creationTime = info32.creationTime;
        LL_I2L(info->size, info32.size);
    }
    return(rv);
}








PRInt32
_PR_MD_GETOPENFILEINFO(const PRFileDesc *fd, PRFileInfo *info)
{
    struct stat statBuf;
    PRInt32 rv = PR_SUCCESS;
    
    rv = fstat( fd->secret->md.osfd, &statBuf );
    if ( rv == 0)
    {
        if (statBuf.st_mode & S_IFREG )
            info->type = PR_FILE_FILE;
        else if ( statBuf.st_mode & S_IFDIR )
            info->type = PR_FILE_DIRECTORY;
        else
            info->type = PR_FILE_OTHER;
        info->size = statBuf.st_size;
        LL_I2L(info->modifyTime, statBuf.st_mtime);
        LL_I2L(info->creationTime, statBuf.st_ctime);
        
    }
    else
    {
        _PR_MD_MAP_FSTAT_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return(rv);
}

PRInt32
_PR_MD_GETOPENFILEINFO64(const PRFileDesc *fd, PRFileInfo64 *info)
{
    PRFileInfo info32;
    
    PRInt32 rv = _PR_MD_GETOPENFILEINFO(fd, &info32);
    if (0 == rv)
    {
        info->type = info32.type;
        info->modifyTime = info32.modifyTime;
        info->creationTime = info32.creationTime;
        LL_I2L(info->size, info32.size);
    }
    return(rv);
}








PRInt32
_PR_MD_RENAME(const char *from, const char *to)
{
    PRInt32 rv;
    
    rv = rename( from, to );
    if ( rv == -1 )
    {
        _PR_MD_MAP_RENAME_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return( rv );
}








PRInt32
_PR_MD_ACCESS(const char *name, PRIntn how)
{
    PRInt32     rv;
    int         mode = 0;
    
    if ( how & PR_ACCESS_WRITE_OK )
        mode |= W_OK;
    if ( how & PR_ACCESS_READ_OK )
        mode |= R_OK;
        
    rv = (PRInt32) access( name, mode );        
    if ( rv == -1 )
    {
        _PR_MD_MAP_ACCESS_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return(rv);
}








PRInt32
_PR_MD_MKDIR(const char *name, PRIntn mode)
{
    PRInt32 rv;
        
    rv = mkdir( name );
    if ( rv == 0 )
    {
        PR_Sleep( _PR_MD_WIN16_DELAY );    
        return PR_SUCCESS;
    }
    else
    {
        _PR_MD_MAP_MKDIR_ERROR( errno );
        PR_Sleep( _PR_MD_WIN16_DELAY );    
        return PR_FAILURE;
    }
}








PRInt32
_PR_MD_RMDIR(const char *name)
{
    PRInt32 rv;
    
    rv = (PRInt32) rmdir( name );
    if ( rv == -1 )
    {
        _PR_MD_MAP_RMDIR_ERROR( errno );
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return(rv);
}














PRStatus
_PR_MD_LOCKFILE(PRInt32 f)
{
    PRInt32 rv = PR_SUCCESS;    
    long    seekOrigin;         
    PRInt32 rc;                 

    

    
    seekOrigin = lseek( f, 0l, SEEK_SET );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        return( PR_FAILURE );
    }
    
    



    for( rc = -1; rc != 0; )
    {
        rc = _locking( f, _LK_NBLCK , 0x7fffffff );
        if ( rc == -1 )
        {
            if ( errno == EACCES )
            {
                PR_Sleep( 100 );
                continue;
            }
            else
            {
                _PR_MD_MAP_LOCKF_ERROR( errno );
                rv = PR_FAILURE;
                break;
            }
        }
    } 
    
    




    rc = lseek( f, seekOrigin, SEEK_SET );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        rv = PR_FAILURE;
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return PR_SUCCESS;
} 















PRStatus
_PR_MD_TLOCKFILE(PRInt32 f)
{
    PRInt32 rv = PR_SUCCESS;    
    long    seekOrigin;         
    PRInt32 rc;                 

    

    
    seekOrigin = lseek( f, 0l, SEEK_SET );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        return( PR_FAILURE );
    }
    
    



    rc = _locking( f, _LK_NBLCK , 0x7fffffff );
    if ( rc == -1 )
    {
        if ( errno != EACCES )
            _PR_MD_MAP_LOCKF_ERROR( errno );
        rv = PR_FAILURE;
    }
    
    



    rc = lseek( f, seekOrigin, SEEK_SET );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        rv = PR_FAILURE;
    }
    
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return rv;
} 








PRStatus
_PR_MD_UNLOCKFILE(PRInt32 f)
{
    PRInt32 rv = PR_SUCCESS;    
    long    seekOrigin;         
    PRInt32 rc;                 

    

    
    seekOrigin = lseek( f, 0l, SEEK_SET );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        return( PR_FAILURE );
    }
    
    


    rc = _locking( f, _LK_UNLCK , 0x7fffffff );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LOCKF_ERROR( errno );
        rv = PR_FAILURE;
    }
    
    



    rc = lseek( f, seekOrigin, SEEK_SET );
    if ( rc == -1 )
    {
        _PR_MD_MAP_LSEEK_ERROR( errno );
        rv = PR_FAILURE;
    }
    PR_Sleep( _PR_MD_WIN16_DELAY );    
    return rv;
} 











PR_IMPLEMENT(PRInt32) PR_Stat(const char *name, struct stat *buf)
{
    PRInt32     rv;
    _MDMSStat   *mssb = (_MDMSStat*) buf; 
    struct stat statBuf;   

    


    rv = (PRInt32) _stat( name, &statBuf);
    if (rv == 0l )
    {
        mssb->st_dev = statBuf.st_dev;
        mssb->st_ino = statBuf.st_ino; 
        mssb->st_mode = statBuf.st_mode;
        mssb->st_nlink = 1; 
        mssb->st_uid = statBuf.st_uid;
        mssb->st_gid = statBuf.st_gid;
        mssb->st_rdev = statBuf.st_rdev; 
        mssb->st_size = statBuf.st_size;
        mssb->st_atime = statBuf.st_atime;
        mssb->st_mtime = statBuf.st_mtime;
        mssb->st_ctime = statBuf.st_ctime;
    }
    return rv;
} 




