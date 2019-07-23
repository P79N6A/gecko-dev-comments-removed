




































#include "xprintutil.h"
 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#ifdef XPU_USE_THREADS
#include <time.h>
#ifdef XPU_USE_NSPR
#include <prthread.h>
#else
#include <pthread.h>
#endif 
#endif 
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>


#ifdef DEBUG
static void PrintXPGetDocStatus( XPGetDocStatus status );
#endif
static Bool  XNextEventTimeout( Display *display, XEvent *event_return, struct timeval *timeout );
static void *XpuPrintToFile( Display *pdpy, XPContext pcontext, const char *filename );
static void  MyPrintToFileProc( Display *pdpy, XPContext pcontext, unsigned char *data, unsigned int data_len, XPointer client_data );
static void  MyFinishProc( Display *pdpy, XPContext pcontext, XPGetDocStatus  status, XPointer client_data );
#ifdef XPU_USE_NSPR
static void PrintToFile_Consumer( void *handle );
#else
static void *PrintToFile_Consumer( void *handle );
#endif

void XpuStartJobToSpooler(Display *pdpy)
{
  XpStartJob(pdpy, XPSpool);
}

void *XpuStartJobToFile( Display *pdpy, XPContext pcontext, const char *filename )
{
  void *handle;

  XpStartJob(pdpy, XPGetData);
  handle = XpuPrintToFile(pdpy, pcontext, filename);
  
  if (!handle)
  {
    
    XpCancelJob(pdpy, True);
  }

  return(handle);
}

#ifdef DEBUG

static
void PrintXPGetDocStatus( XPGetDocStatus status )
{
  switch(status)
  {
    case XPGetDocFinished:        puts("PrintXPGetDocStatus: XPGetDocFinished");       break;
    case XPGetDocSecondConsumer:  puts("PrintXPGetDocStatus: XPGetDocSecondConsumer"); break;
    case XPGetDocError:           puts("PrintXPGetDocStatus: XPGetDocError");          break;
    default:                      puts("PrintXPGetDocStatus: <unknown value");         break;
  }
}
#endif 



static
Bool XNextEventTimeout( Display *display, XEvent *event_return, struct timeval *timeout ) 
{
  int      res;
  fd_set   readfds;
  int      display_fd = XConnectionNumber(display);

  
  if( timeout == NULL )
  {
    XNextEvent(display, event_return);
    return(True);
  }
  
  FD_ZERO(&readfds);
  FD_SET(display_fd, &readfds);

  









  while( XEventsQueued(display, QueuedAfterFlush) == 0 )
  {
    res = select(display_fd+1, &readfds, NULL, NULL, timeout);
  
    switch(res)
    {
      case -1:  
          perror("XNextEventTimeout: select() failure"); 
          return(False);
      case  0: 
        return(False);
    }
  }
  
  XNextEvent(display, event_return); 
  return(True);
}


#ifdef XPU_USE_THREADS



















 
typedef struct 
{
#ifdef XPU_USE_NSPR
  PRThread      *prthread;
#else
  pthread_t      tid;
#endif    
  char          *displayname;
  Display       *pdpy;
  Display       *parent_pdpy;
  XPContext      pcontext;
  const char    *file_name;
  FILE          *file;
  XPGetDocStatus status;
  Bool           done;
} MyPrintFileData;


static
void *XpuPrintToFile( Display *pdpy, XPContext pcontext, const char *filename )
{
  MyPrintFileData *mpfd; 
         
  if( (mpfd = malloc(sizeof(MyPrintFileData))) == NULL )
    return(NULL);
  
  mpfd->parent_pdpy = pdpy;
  mpfd->displayname = XDisplayString(pdpy);
  mpfd->pdpy        = NULL;
  mpfd->pcontext    = pcontext;
  mpfd->file_name   = filename;
  mpfd->file        = NULL;      
  mpfd->status      = XPGetDocError;
  
  
  if( (mpfd->file = fopen(mpfd->file_name, "w")) == NULL ) 
  {
    
    free(mpfd);
    return(NULL);
  }
  
  


  XFlush(pdpy);
#ifdef XPU_USE_NSPR
  if( (mpfd->prthread = PR_CreateThread(PR_SYSTEM_THREAD, PrintToFile_Consumer, mpfd, PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0)) == NULL )
#else    
  if( pthread_create(&(mpfd->tid), NULL, PrintToFile_Consumer, mpfd) != 0 ) 
#endif
  {
    
    fclose(mpfd->file);
    free(mpfd);
    return(NULL);
  }
  
  
  XPU_DEBUG_ONLY(printf("### parent started consumer thread.\n" ));
  return(mpfd);
}


XPGetDocStatus XpuWaitForPrintFileChild( void *handle )
{
  MyPrintFileData *mpfd   = (MyPrintFileData *)handle;
  void            *res;
  XPGetDocStatus   status;
  
  

  XFlush(mpfd->parent_pdpy);

#ifdef XPU_USE_NSPR
  if( PR_JoinThread(mpfd->prthread) != PR_SUCCESS  )
    perror("XpuWaitForPrintFileChild: PR_JoinThread() failure"); 
#else   
  if( XPU_TRACE(pthread_join(mpfd->tid, &res)) != 0 )
    perror("XpuWaitForPrintFileChild: pthread_join() failure");
#endif
      
  status = mpfd->status;      
  free(handle);  
              
  XPU_DEBUG_ONLY(PrintXPGetDocStatus(status));
  return(status);
}

#else 



















typedef struct 
{
  pid_t           pid;
  int             pipe[2];  
  const char     *displayname;
  Display        *pdpy;
  Display        *parent_pdpy;
  XPContext       pcontext;
  const char     *file_name;
  FILE           *file;
  XPGetDocStatus  status;
  Bool            done;
} MyPrintFileData;


static
void *XpuPrintToFile( Display *pdpy, XPContext pcontext, const char *filename )
{
  MyPrintFileData *mpfd;

  if( (mpfd = (MyPrintFileData *)malloc(sizeof(MyPrintFileData))) == NULL )
    return(NULL);

  
  if( pipe(mpfd->pipe) == -1 ) 
  {
    
    perror("XpuPrintToFile: cannot create pipe");
    free(mpfd);
    return(NULL);
  }
        
  mpfd->parent_pdpy = pdpy;
  mpfd->displayname = XDisplayString(pdpy);
  mpfd->pcontext    = pcontext;
  mpfd->file_name   = filename;
  mpfd->file        = NULL;
  mpfd->status      = XPGetDocError;
  
  
  if( (mpfd->file = fopen(mpfd->file_name, "w")) == NULL ) 
  {
    
    close(mpfd->pipe[1]);
    close(mpfd->pipe[0]);      
    free(mpfd);
    return(NULL);
  }
  
  


  XFlush(pdpy);     
  
  mpfd->pid = fork();

  if( mpfd->pid == 0 ) 
  {
    
    PrintToFile_Consumer(mpfd);
  }
  else if( mpfd->pid < 0 ) 
  {
    
    close(mpfd->pipe[1]);
    close(mpfd->pipe[0]);
    fclose(mpfd->file);
    free(mpfd);
    return(NULL);
  }
  
  
  XPU_DEBUG_ONLY(printf("### parent fork()'ed consumer child.\n"));
  
  
  fclose(mpfd->file);
  close(mpfd->pipe[1]);
  return(mpfd);      
}


XPGetDocStatus XpuWaitForPrintFileChild( void *handle )
{
  MyPrintFileData *mpfd   = (MyPrintFileData *)handle;
  int              res;
  XPGetDocStatus   status = XPGetDocError; 

  

  XFlush(mpfd->parent_pdpy);
  
  if( XPU_TRACE(waitpid(mpfd->pid, &res, 0)) == -1 )
    perror("XpuWaitForPrintFileChild: waitpid failure");

   
  if( read(mpfd->pipe[0], &status, sizeof(XPGetDocStatus)) != sizeof(XPGetDocStatus) )
  {
    perror("XpuWaitForPrintFileChild: can't read XPGetDocStatus");
  }    
  close(mpfd->pipe[0]);
    
  free(handle);
  
  XPU_DEBUG_ONLY(PrintXPGetDocStatus(status));
  return(status);
}
#endif 


static 
void MyPrintToFileProc( Display       *pdpy,
                        XPContext      pcontext,
                        unsigned char *data, 
                        unsigned int   data_len,
                        XPointer       client_data )
{
  MyPrintFileData *mpfd = (MyPrintFileData *)client_data;
  
  
  XPU_TRACE_CHILD((void)fwrite(data, data_len, 1, mpfd->file)); 
}   


static 
void MyFinishProc( Display        *pdpy,
                   XPContext       pcontext,
                   XPGetDocStatus  status,
                   XPointer        client_data )
{
  MyPrintFileData *mpfd = (MyPrintFileData *)client_data;

  
  if( status != XPGetDocFinished )
  {
    XPU_DEBUG_ONLY(printf("MyFinishProc: error %d\n", (int)status));
    XPU_TRACE_CHILD(remove(mpfd->file_name));
  }  
  
  XPU_TRACE_CHILD((void)fclose(mpfd->file)); 
  
  mpfd->status = status; 
  mpfd->done   = True;    
}


static
#ifdef XPU_USE_NSPR
void PrintToFile_Consumer( void *handle )
#else
void *PrintToFile_Consumer( void *handle )
#endif
{
  MyPrintFileData *mpfd = (MyPrintFileData *)handle;
  XEvent           dummy;
  struct timeval   timeout;
  
  timeout.tv_sec  = 0;
  timeout.tv_usec = 100000; 
        
  XPU_DEBUG_ONLY(printf("### child running, getting data from '%s'.\n", mpfd->displayname));
  
  
  if( (mpfd->pdpy = XPU_TRACE_CHILD(XOpenDisplay(mpfd->displayname))) == NULL )
  {
    perror("child cannot open display");
#ifdef XPU_USE_NSPR
    return;
#else
    return(NULL);
#endif
  }
    
  mpfd->done = False;
    
  
  if( XPU_TRACE_CHILD(XpGetDocumentData(mpfd->pdpy, mpfd->pcontext,
                                        MyPrintToFileProc, MyFinishProc, 
                                        (XPointer)mpfd)) == 0 )
  {
    XPU_DEBUG_ONLY(printf("XpGetDocumentData cannot register callbacks\n"));
#ifdef XPU_USE_NSPR
      return;
#else
      return(NULL);
#endif
  }      
  
  


  while( mpfd->done != True )
  {
    XNextEventTimeout(mpfd->pdpy, &dummy, &timeout);
  }
  
  XCloseDisplay(mpfd->pdpy);

#ifdef XPU_USE_THREADS
#ifdef XPU_USE_NSPR
  return;
#else
  return(NULL);
#endif
#else   
  
  if( XPU_TRACE_CHILD(write(mpfd->pipe[1], &mpfd->status, sizeof(XPGetDocStatus))) != sizeof(XPGetDocStatus) )
  {
    perror("PrintToFile_Consumer: can't write XPGetDocStatus");
  }
  
  




   XPU_TRACE_CHILD(_exit(EXIT_SUCCESS));
#endif            
}


