


















































#include <stdio.h>
#include <string.h>
#include <errno.h>

#if defined(XP_MACOSX)
#include <Carbon/Carbon.h>
#endif

#if defined(SUNOS4)
#include <unistd.h>  
#endif 

#include "prerror.h"
#include "prlog.h"

#include "vr_stubs.h"
#include "nr_bufio.h"


#define BUFIO_BUFSIZE_DEFAULT   0x2000

#define STARTS_IN_BUF(f) ((f->fpos >= f->datastart) && \
                         (f->fpos < (f->datastart+f->datasize)))

#define ENDS_IN_BUF(f,c) (((f->fpos + c) > (PRUint32)f->datastart) && \
                         ((f->fpos + c) <= (PRUint32)(f->datastart+f->datasize)))

#if DEBUG_dougt
static num_reads = 0;
#endif


struct BufioFileStruct 
{
    FILE    *fd;        
    PRInt32 fsize;      
    PRInt32 fpos;       
    PRInt32 datastart;  
    PRInt32 datasize;   
    PRInt32 bufsize;	
    PRBool  bufdirty;   
    PRInt32 dirtystart;
    PRInt32 dirtyend;
    PRBool  readOnly;   
#ifdef DEBUG_dveditzbuf
    PRUint32 reads;
    PRUint32 writes;
#endif
    char    *data;      
};


static PRBool _bufio_loadBuf( BufioFile* file, PRUint32 count );
static int    _bufio_flushBuf( BufioFile* file );

#ifdef XP_OS2
#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>
#include <io.h>

FILE* os2_fileopen(const char* name, const char* mode)
{
    int access = O_RDWR;
    int descriptor;
    int pmode = 0;

    
    if (mode[1] == '\0') {
        return NULL;
    }
    
    if (mode[0] == 'w' && mode[1] == 'b') {
        access |= (O_TRUNC | O_CREAT);
        if (mode[2] == '+') {
            access |= O_RDWR;
            pmode = S_IREAD | S_IWRITE;
        } else {
            access |= O_WRONLY;
            pmode = S_IWRITE;
        }
    }
    if (mode[0] == 'r' && mode[1] == 'b') {
        if (mode[2] == '+') {
            access |= O_RDWR;
            pmode = S_IREAD | S_IWRITE;
        } else {
            access = O_RDONLY;
            pmode = S_IREAD;
        }
    }

    descriptor = sopen(name, access, SH_DENYNO, pmode);
    if (descriptor != -1) {
        return fdopen(descriptor, mode);
    }
    return NULL;
}
#endif




BufioFile*  bufio_Open(const char* name, const char* mode)
{
    FILE        *fd;
    BufioFile   *file = NULL;

#ifdef XP_OS2
    fd = os2_fileopen( name, mode );
#else
    fd = fopen( name, mode );
#endif
    
    if ( fd )
    {
        

        file = PR_NEWZAP( BufioFile );
        if ( file )
        {
            file->fd = fd;
            file->bufsize = BUFIO_BUFSIZE_DEFAULT;  

            file->data = (char*)PR_Malloc( file->bufsize );
            if ( file->data )
            {
                
                if ( !fseek( fd, 0, SEEK_END ) )
                {
                    file->fsize = ftell( fd );

                    file->readOnly = strcmp(mode,XP_FILE_READ) == 0 || 
                                     strcmp(mode,XP_FILE_READ_BIN) == 0;
                }
                else
                {
                    PR_Free( file->data );
                    PR_DELETE( file );
                }
            }
            else
                PR_DELETE( file );
        }

        
        if (!file)
        {
            fclose( fd );
            PR_SetError( PR_OUT_OF_MEMORY_ERROR, 0 );
        }
    }
    else
    {
        
        
        switch (errno)
        {
            
#if defined(XP_MACOSX)
            case fnfErr:
#else
            case ENOENT:
#endif
                PR_SetError(PR_FILE_NOT_FOUND_ERROR,0);
                break;

            
#if defined(XP_MACOSX)
            case opWrErr:
#else
            case EACCES:
#endif
                PR_SetError(PR_NO_ACCESS_RIGHTS_ERROR,0);
                break;

            default:
                PR_SetError(PR_UNKNOWN_ERROR,0);
                break;
        }
    }

    return file;
}






int bufio_Close(BufioFile* file)
{
    int retval = -1;

    if ( file )
    {
        if ( file->bufdirty )
            _bufio_flushBuf( file );

        retval = fclose( file->fd );

        if ( file->data )
            PR_Free( file->data );

        PR_DELETE( file );
#if DEBUG_dougt
        printf(" --- > Buffered registry read fs hits (%d)\n", num_reads);
#endif
    }

    return retval;
}






int bufio_Seek(BufioFile* file, PRInt32 offset, int whence)
{
    if (!file)
        return -1;

    switch(whence) 
    {
      case SEEK_SET:
	    file->fpos = offset;
	    break;
	  case SEEK_END:
	    file->fpos = file->fsize + offset;
	    break;
	  case SEEK_CUR:
	    file->fpos = file->fpos + offset;
	    break;
	  default:
	    return -1;
    }

    if ( file->fpos < 0 ) 
        file->fpos = 0;

    return 0;
}






PRInt32 bufio_Tell(BufioFile* file)
{
    if (file)
        return file->fpos;
    else
        return -1;
}



PRUint32 bufio_Read(BufioFile* file, char* dest, PRUint32 count)
{
    PRInt32     startOffset;
    PRInt32     endOffset;
    PRInt32     leftover;
    PRUint32    bytesCopied;
    PRUint32    bytesRead;
    PRUint32    retcount = 0;

    
    if ( !file || !dest || count == 0 || file->fpos >= file->fsize )
        return 0;

    
    if ( (file->fpos + count) > (PRUint32)file->fsize )
        count = file->fsize - file->fpos;


    

    startOffset = file->fpos - file->datastart;
    endOffset = startOffset + count;

    if ( startOffset >= 0 && startOffset < file->datasize )
    {
        
        

        if ( endOffset <= file->datasize )
            bytesCopied = count;
        else
            bytesCopied = file->datasize - startOffset;

        memcpy( dest, file->data + startOffset, bytesCopied );
        retcount = bytesCopied;
        file->fpos += bytesCopied;
#ifdef DEBUG_dveditzbuf
        file->reads++;
#endif

        

        leftover = count - bytesCopied;
        PR_ASSERT( leftover >= 0 );     

        if ( leftover )
        {
            

            
            
            

            if ( _bufio_loadBuf( file, leftover ) )
            {
                startOffset = file->fpos - file->datastart;

                
                if ( startOffset > file->datasize )
                    bytesRead = 0;
                else if ( startOffset+leftover <= file->datasize )
                    bytesRead = leftover;
                else
                    bytesRead = file->datasize - startOffset;

                if ( bytesRead )
                {
                    memcpy( dest+bytesCopied, file->data+startOffset, bytesRead );
                    file->fpos += bytesRead;
                    retcount += bytesRead;
#ifdef DEBUG_dveditzbuf
                    file->reads++;
#endif
                }
            }
            else 
            {
                
                

                if ( fseek( file->fd, file->fpos, SEEK_SET ) == 0 )
                {
#if DEBUG_dougt
                    ++num_reads;
#endif
                    bytesRead = fread(dest+bytesCopied, 1, leftover, file->fd);
                    file->fpos += bytesRead;
                    retcount += bytesRead;
                }
                else 
                {
                    
                    
                }
            }
        }
    }
    else
    {
        
        if ( endOffset > 0 && endOffset <= file->datasize )
            bytesCopied = endOffset;
        else
            bytesCopied = 0;

        leftover = count - bytesCopied;

        if ( bytesCopied )
        {
            
            
            memcpy( dest+leftover, file->data, bytesCopied );
#ifdef DEBUG_dveditzbuf
            file->reads++;
#endif
        }
            
        

        if ( _bufio_loadBuf( file, leftover ) )
        {
            
            startOffset = file->fpos - file->datastart;

            
            if ( startOffset > file->datasize )
                bytesRead = 0;
            else if ( startOffset+leftover <= file->datasize )
                bytesRead = leftover;
            else
                bytesRead = file->datasize - startOffset;

            if ( bytesRead )
            {
                memcpy( dest, file->data+startOffset, bytesRead );
#ifdef DEBUG_dveditzbuf
                file->reads++;
#endif
            }
        }
        else
        {
            
            if ( fseek( file->fd, file->fpos, SEEK_SET ) == 0 )
            {
                bytesRead = fread(dest, 1, leftover, file->fd);
#if DEBUG_dougt
                ++num_reads;
#endif        
            }
            else
                bytesRead = 0;
        }

        
        
        if ( bytesRead == (PRUint32)leftover )
            retcount = bytesCopied + bytesRead;
        else
            retcount = bytesRead;

        file->fpos += retcount;
    }

    return retcount;
}






PRUint32 bufio_Write(BufioFile* file, const char* src, PRUint32 count)
{
    const char* newsrc;
    PRInt32  startOffset;
    PRInt32  endOffset;
    PRUint32 leftover;
    PRUint32 retcount = 0;
    PRUint32 bytesWritten = 0;
    PRUint32 bytesCopied = 0;

    
    if ( !file || !src || count == 0 || file->readOnly )
        return 0;

    

    startOffset = file->fpos - file->datastart;
    endOffset = startOffset + count;

    if ( startOffset >= 0 && startOffset <  file->bufsize )
    {
        

        if ( endOffset <= file->bufsize )
            bytesCopied = count;
        else
            bytesCopied = file->bufsize - startOffset;

        memcpy( file->data + startOffset, src, bytesCopied );
        file->bufdirty = PR_TRUE;
        endOffset = startOffset + bytesCopied;
        file->dirtystart = PR_MIN( startOffset, file->dirtystart );
        file->dirtyend   = PR_MAX( endOffset,   file->dirtyend );
#ifdef DEBUG_dveditzbuf
        file->writes++;
#endif

        if ( endOffset > file->datasize )
            file->datasize = endOffset;

        retcount = bytesCopied;
        file->fpos += bytesCopied;

        
        leftover = count - bytesCopied;
        newsrc = src+bytesCopied;
    }
    else
    {
        
        if ( endOffset > 0 && endOffset <= file->bufsize )
            bytesCopied = endOffset;
        else
            bytesCopied = 0;

        leftover = count - bytesCopied;
        newsrc = src;

        if ( bytesCopied )
        {
            
            memcpy( file->data, src+leftover, bytesCopied );
            file->bufdirty      = PR_TRUE;
            file->dirtystart    = 0;
            file->dirtyend      = PR_MAX( endOffset, file->dirtyend );
#ifdef DEBUG_dveditzbuf
            file->writes++;
#endif

            if ( endOffset > file->datasize )
                file->datasize = endOffset;
        }
    }

    
    if ( leftover )
    {
        
        if ( _bufio_loadBuf( file, leftover ) )
        {
            startOffset = file->fpos - file->datastart;
            endOffset   = startOffset + leftover;

            memcpy( file->data+startOffset, newsrc, leftover );
            file->bufdirty      = PR_TRUE;
            file->dirtystart    = startOffset;
            file->dirtyend      = endOffset;
#ifdef DEBUG_dveditzbuf
            file->writes++;
#endif
            if ( endOffset > file->datasize )
                file->datasize = endOffset;

            bytesWritten = leftover;
        }
        else
        {
            
            if ( fseek( file->fd, file->fpos, SEEK_SET ) == 0 )
                bytesWritten = fwrite( newsrc, 1, leftover, file->fd );
            else
                bytesWritten = 0; 
        }

        if ( retcount )
        {
            
            retcount    += bytesWritten;
            file->fpos  += bytesWritten;
        }
        else
        {
            retcount    = bytesCopied + bytesWritten;
            file->fpos  += retcount;
        }
    }

    if ( file->fpos > file->fsize )
        file->fsize = file->fpos;
    
    return retcount;
}



int bufio_Flush(BufioFile* file)
{
    if ( file->bufdirty )
        _bufio_flushBuf( file );
    
    return fflush(file->fd);
}












static PRBool _bufio_loadBuf( BufioFile* file, PRUint32 count )
{
    PRInt32     startBuf;
    PRInt32     endPos;
    PRInt32     endBuf;
    PRUint32    bytesRead;

    
    if ( count > (PRUint32)file->bufsize )
        return PR_FALSE;

    
    if ( STARTS_IN_BUF(file) && ENDS_IN_BUF(file,count) )
    {   
        PR_ASSERT(0);
        return PR_TRUE;
    }

    
    if ( file->bufdirty && _bufio_flushBuf(file) != 0 )
        return PR_FALSE;

    
    
    startBuf = ( file->fpos / file->bufsize ) * file->bufsize;
    endPos = file->fpos + count;
    endBuf = startBuf + file->bufsize;
    if ( endPos > endBuf )
        startBuf += (endPos - endBuf);

    if ( fseek( file->fd, startBuf, SEEK_SET ) != 0 )
        return PR_FALSE;
    else
    {
#if DEBUG_dougt
        ++num_reads;
#endif
        bytesRead = fread( file->data, 1, file->bufsize, file->fd );
        file->datastart  = startBuf;
        file->datasize   = bytesRead;
        file->bufdirty   = PR_FALSE;
        file->dirtystart = file->bufsize;
        file->dirtyend   = 0;
#ifdef DEBUG_dveditzbuf
        printf("REG: buffer read %d (%d) after %d reads\n",startBuf,file->fpos,file->reads);
        file->reads = 0;
        file->writes = 0;
#endif
        return PR_TRUE;
    }
}



static int _bufio_flushBuf( BufioFile* file )
{
    PRUint32 written;
    PRUint32 dirtyamt;
    PRInt32  startpos;

    PR_ASSERT(file);
    if ( !file || !file->bufdirty )
        return 0;

    startpos = file->datastart + file->dirtystart;
    if ( !fseek( file->fd, startpos, SEEK_SET ) )
    {
        dirtyamt = file->dirtyend - file->dirtystart;
        written = fwrite( file->data+file->dirtystart, 1, dirtyamt, file->fd );
        if ( written == dirtyamt )
        {
#ifdef DEBUG_dveditzbuf
            printf("REG: buffer flush %d - %d after %d writes\n",startpos,startpos+written,file->writes);
            file->writes = 0;
#endif
            file->bufdirty   = PR_FALSE;
            file->dirtystart = file->bufsize;
            file->dirtyend   = 0;
            return 0;
        }
    }
    return -1;
}










int bufio_SetBufferSize(BufioFile* file, int bufsize)
{
    char    *newBuffer;
    int     retVal = -1;

    PR_ASSERT(file);
    if (!file)
        return retVal;

    if (bufsize == -1)
        bufsize = BUFIO_BUFSIZE_DEFAULT;
    if (bufsize == file->bufsize)
        return bufsize;

    newBuffer = (char*)PR_Malloc( bufsize );
    if (newBuffer)
    {
        
        if ( file->bufdirty && _bufio_flushBuf(file) != 0 )
        {
            PR_Free( newBuffer );
            return -1;
        }


        file->bufsize = bufsize;
        if ( file->data )
            PR_Free( file->data );
        file->data = newBuffer;
        file->datasize = 0;
        file->datastart = 0;
        retVal = bufsize;
    }
 
    return retVal;
}



