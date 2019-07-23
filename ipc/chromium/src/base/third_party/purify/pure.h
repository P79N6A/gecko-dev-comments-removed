








#if defined(PURIFY) || defined(QUANTIFY)

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif



#ifdef PURIFY_PRIVATE_INCLUDE

#define PURE_H_VERSION 1
#include <stddef.h>






int __cdecl PurifyIsRunning(void)			;



int __cdecl PurePrintf(const char *fmt, ...)		;
int __cdecl PurifyPrintf(const char *fmt, ...)		;



size_t __cdecl PurifyNewInuse(void)			;
size_t __cdecl PurifyAllInuse(void) 			;
size_t __cdecl PurifyClearInuse(void)			;
size_t __cdecl PurifyNewLeaks(void)			;
size_t __cdecl PurifyAllLeaks(void)			;
size_t __cdecl PurifyClearLeaks(void)			;



size_t __cdecl PurifyAllHandlesInuse(void)			;
size_t __cdecl PurifyNewHandlesInuse(void)			;



size_t __cdecl PurifyDescribe(void *addr)			;
size_t __cdecl PurifyWhatColors(void *addr, size_t size) 	;





int __cdecl PurifyAssertIsReadable(const void *addr, size_t size)	;	
int __cdecl PurifyAssertIsWritable(const void *addr, size_t size)	;




int __cdecl PurifyIsReadable(const void *addr, size_t size)	;
int __cdecl PurifyIsWritable(const void *addr, size_t size)	;
int __cdecl PurifyIsInitialized(const void *addr, size_t size)	;



void __cdecl PurifyMarkAsInitialized(void *addr, size_t size)	;
void __cdecl PurifyMarkAsUninitialized(void *addr, size_t size)	;



#define PURIFY_HEAP_CRT 					(HANDLE) ~(__int64) 1 /* 0xfffffffe */
#define PURIFY_HEAP_ALL 					(HANDLE) ~(__int64) 2 /* 0xfffffffd */
#define PURIFY_HEAP_BLOCKS_LIVE 			0x80000000
#define PURIFY_HEAP_BLOCKS_DEFERRED_FREE 	0x40000000
#define PURIFY_HEAP_BLOCKS_ALL 				(PURIFY_HEAP_BLOCKS_LIVE|PURIFY_HEAP_BLOCKS_DEFERRED_FREE)
int __cdecl PurifyHeapValidate(unsigned int hHeap, unsigned int dwFlags, const void *addr)	;
int __cdecl PurifySetLateDetectScanCounter(int counter);
int __cdecl PurifySetLateDetectScanInterval(int seconds);



void   __cdecl   PurifySetPoolId(const void *mem, int id);
int	   __cdecl   PurifyGetPoolId(const void *mem);
void   __cdecl   PurifySetUserData(const void *mem, void *data);
void * __cdecl   PurifyGetUserData(const void *mem);
void   __cdecl   PurifyMapPool(int id, void(*fn)());







int __cdecl QuantifyIsRunning(void)			;




int __cdecl QuantifyDisableRecordingData(void)		;
int __cdecl QuantifyStartRecordingData(void)		;
int __cdecl QuantifyStopRecordingData(void)		;
int __cdecl QuantifyClearData(void)			;
int __cdecl QuantifyIsRecordingData(void)		;


int __cdecl QuantifyAddAnnotation(char *)		;


int __cdecl QuantifySaveData(void)			;


int __cdecl QuantifySetThreadName(char *)		;






int __cdecl CoverageIsRunning(void)			;



int __cdecl CoverageDisableRecordingData(void)		;
int __cdecl CoverageStartRecordingData(void)		;
int __cdecl CoverageStopRecordingData(void)		;
int __cdecl CoverageClearData(void)			;
int __cdecl CoverageIsRecordingData(void)		;

int __cdecl CoverageAddAnnotation(char *)		;


int __cdecl CoverageSaveData(void)			;


#endif 

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif