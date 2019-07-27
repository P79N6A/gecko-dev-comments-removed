



#ifndef _CPR_WIN_DEFINES_H
#define _CPR_WIN_DEFINES_H

#include "cpr_types.h"
#include "cpr_ipc.h"
#include "cpr_locks.h"
#include "cpr_timers.h"
#include "cpr_threads.h"
#include "cpr_debug.h"
#include "cpr_memory.h"

__BEGIN_DECLS




typedef struct {
    uint8_t majorRelease;
    uint8_t minorRelease;
    uint8_t pointRelease;
} cpr_version_t;

typedef void *cprRegion_t;
typedef void *cprPool_t;



typedef void* cprSignal_t;




typedef enum
{
    CPR_SOC_SECURE,
    CPR_SOC_NONSECURE
} cpr_soc_sec_status_e;




typedef enum
{
    CPR_SOC_CONN_OK,
    CPR_SOC_CONN_WAITING,
    CPR_SOC_CONN_FAILED
} cpr_soc_connect_status_e;




cprRegion_t cprCreateRegion (const char *regionName);
cprPool_t cprCreatePool (cprRegion_t region, const char *name, uint32_t initialBuffers, uint32_t bufferSize) ;


void cprDisableSwap (void);
void cprEnableSwap (void);

#define TCP_PORT_RETRY_CNT  5
#define TCP_PORT_MASK           0xfff




#define CIPPORT_EPH_LOW         0xC000
#define CIPPORT_EPH_HI          0xCFFF



__END_DECLS

#endif
