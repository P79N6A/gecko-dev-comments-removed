








#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#define PURE_H_VERSION 1






int __cdecl PurifyIsRunning(void)			;



int __cdecl PurePrintf(const char *fmt, ...)		;
int __cdecl PurifyPrintf(const char *fmt, ...)		;



int __cdecl PurifyNewInuse(void)			;
int __cdecl PurifyAllInuse(void) 			;
int __cdecl PurifyClearInuse(void)			;
int __cdecl PurifyNewLeaks(void)			;
int __cdecl PurifyAllLeaks(void)			;
int __cdecl PurifyClearLeaks(void)			;



int __cdecl PurifyAllHandlesInuse(void)			;
int __cdecl PurifyNewHandlesInuse(void)			;



int __cdecl PurifyDescribe(void *addr)			;
int __cdecl PurifyWhatColors(void *addr, int size) 	;





int __cdecl PurifyAssertIsReadable(const void *addr, int size)	;
int __cdecl PurifyAssertIsWritable(const void *addr, int size)	;




int __cdecl PurifyIsReadable(const void *addr, int size)	;
int __cdecl PurifyIsWritable(const void *addr, int size)	;
int __cdecl PurifyIsInitialized(const void *addr, int size)	;



void __cdecl PurifyMarkAsInitialized(void *addr, int size)	;
void __cdecl PurifyMarkAsUninitialized(void *addr, int size)	;



#define PURIFY_HEAP_CRT 					0xfffffffe
#define PURIFY_HEAP_ALL 					0xfffffffd
#define PURIFY_HEAP_BLOCKS_LIVE 			0x80000000
#define PURIFY_HEAP_BLOCKS_DEFERRED_FREE 	0x40000000
#define PURIFY_HEAP_BLOCKS_ALL 				(PURIFY_HEAP_BLOCKS_LIVE|PURIFY_HEAP_BLOCKS_DEFERRED_FREE)
int __cdecl PurifyHeapValidate(unsigned int hHeap, unsigned int dwFlags, const void *addr)	;
int __cdecl PurifySetLateDetectScanCounter(int counter);
int __cdecl PurifySetLateDetectScanInterval(int seconds);







int __cdecl QuantifyIsRunning(void)			;




int __cdecl QuantifyDisableRecordingData(void)		;
int __cdecl QuantifyStartRecordingData(void)		;
int __cdecl QuantifyStopRecordingData(void)		;
int __cdecl QuantifyClearData(void)			;
int __cdecl QuantifyIsRecordingData(void)		;


int __cdecl QuantifyAddAnnotation(char *)		;


int __cdecl QuantifySaveData(void)			;






int __cdecl CoverageIsRunning(void)			;



int __cdecl CoverageDisableRecordingData(void)		;
int __cdecl CoverageStartRecordingData(void)		;
int __cdecl CoverageStopRecordingData(void)		;
int __cdecl CoverageClearData(void)			;
int __cdecl CoverageIsRecordingData(void)		;

int __cdecl CoverageAddAnnotation(char *)		;


int __cdecl CoverageSaveData(void)			;









int __cdecl PurelockIsRunning(void)			;




#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
