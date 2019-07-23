
























#ifdef MEMORY_DEBUG
# error "The MEMORY_DEBUG macro is obsolete.  Use SQLITE_DEBUG instead."
#endif






#ifdef SQLITE_TEST
unsigned int sqlite3_pending_byte = 0x40000000;
#endif

int sqlite3_os_trace = 0;
#ifdef SQLITE_DEBUG
static int last_page = 0;
#define SEEK(X)           last_page=(X)
#define TRACE1(X)         if( sqlite3_os_trace ) sqlite3DebugPrintf(X)
#define TRACE2(X,Y)       if( sqlite3_os_trace ) sqlite3DebugPrintf(X,Y)
#define TRACE3(X,Y,Z)     if( sqlite3_os_trace ) sqlite3DebugPrintf(X,Y,Z)
#define TRACE4(X,Y,Z,A)   if( sqlite3_os_trace ) sqlite3DebugPrintf(X,Y,Z,A)
#define TRACE5(X,Y,Z,A,B) if( sqlite3_os_trace ) sqlite3DebugPrintf(X,Y,Z,A,B)
#define TRACE6(X,Y,Z,A,B,C) if(sqlite3_os_trace) sqlite3DebugPrintf(X,Y,Z,A,B,C)
#define TRACE7(X,Y,Z,A,B,C,D) \
    if(sqlite3_os_trace) sqlite3DebugPrintf(X,Y,Z,A,B,C,D)
#else
#define SEEK(X)
#define TRACE1(X)
#define TRACE2(X,Y)
#define TRACE3(X,Y,Z)
#define TRACE4(X,Y,Z,A)
#define TRACE5(X,Y,Z,A,B)
#define TRACE6(X,Y,Z,A,B,C)
#define TRACE7(X,Y,Z,A,B,C,D)
#endif





#ifdef SQLITE_PERFORMANCE_TRACE
__inline__ unsigned long long int hwtime(void){
  unsigned long long int x;
  __asm__("rdtsc\n\t"
          "mov %%edx, %%ecx\n\t"
          :"=A" (x));
  return x;
}
static unsigned long long int g_start;
static unsigned int elapse;
#define TIMER_START       g_start=hwtime()
#define TIMER_END         elapse=hwtime()-g_start
#define TIMER_ELAPSED     elapse
#else
#define TIMER_START
#define TIMER_END
#define TIMER_ELAPSED     0
#endif






#ifdef SQLITE_TEST
int sqlite3_io_error_hit = 0;
int sqlite3_io_error_pending = 0;
int sqlite3_diskfull_pending = 0;
int sqlite3_diskfull = 0;
#define SimulateIOError(A)  \
   if( sqlite3_io_error_pending ) \
     if( sqlite3_io_error_pending-- == 1 ){ local_ioerr(); return A; }
static void local_ioerr(){
  sqlite3_io_error_hit = 1;  
}
#define SimulateDiskfullError \
   if( sqlite3_diskfull_pending ){ \
     if( sqlite3_diskfull_pending == 1 ){ \
       local_ioerr(); \
       sqlite3_diskfull = 1; \
       return SQLITE_FULL; \
     }else{ \
       sqlite3_diskfull_pending--; \
     } \
   }
#else
#define SimulateIOError(A)
#define SimulateDiskfullError
#endif




#ifdef SQLITE_TEST
int sqlite3_open_file_count = 0;
#define OpenCounter(X)  sqlite3_open_file_count+=(X)
#else
#define OpenCounter(X)
#endif



















#if defined(SQLITE_ENABLE_MEMORY_MANAGEMENT) || defined (SQLITE_MEMDEBUG)
void *sqlite3GenericMalloc(int n){
  char *p = (char *)malloc(n+8);
  assert(n>0);
  assert(sizeof(int)<=8);
  if( p ){
    *(int *)p = n;
    p += 8;
  }
  return (void *)p;
}
void *sqlite3GenericRealloc(void *p, int n){
  char *p2 = ((char *)p - 8);
  assert(n>0);
  p2 = (char*)realloc(p2, n+8);
  if( p2 ){
    *(int *)p2 = n;
    p2 += 8;
  }
  return (void *)p2;
}
void sqlite3GenericFree(void *p){
  assert(p);
  free((void *)((char *)p - 8));
}
int sqlite3GenericAllocationSize(void *p){
  return p ? *(int *)((char *)p - 8) : 0;
}
#else
void *sqlite3GenericMalloc(int n){
  char *p = (char *)malloc(n);
  return (void *)p;
}
void *sqlite3GenericRealloc(void *p, int n){
  assert(n>0);
  p = realloc(p, n);
  return p;
}
void sqlite3GenericFree(void *p){
  assert(p);
  free(p);
}

int sqlite3GenericAllocationSize(void *p){ return 0; }
#endif
