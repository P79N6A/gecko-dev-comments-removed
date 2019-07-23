




































#if defined(_PPRMWAIT_H)
#else
#define _PPRMWAIT_H

#include "prlock.h"
#include "prcvar.h"
#include "prclist.h"
#include "prthread.h"

#define MAX_POLLING_INTERVAL 100
#define _PR_POLL_COUNT_FUDGE 64
#define _PR_DEFAULT_HASH_LENGTH 59







#define _MW_HASH(a, m) ((((PRUptrdiff)(a) >> 4) ^ ((PRUptrdiff)(a) >> 10)) % (m))
#define _MW_HASH2(a, m) (1 + ((((PRUptrdiff)(a) >> 4) ^ ((PRUptrdiff)(a) >> 10)) % (m - 2)))
#define _MW_ABORTED(_rv) \
    ((PR_FAILURE == (_rv)) && (PR_PENDING_INTERRUPT_ERROR == PR_GetError()))

typedef enum {_prmw_success, _prmw_rehash, _prmw_error} _PR_HashStory;

typedef struct _PRWaiterHash
{
    PRUint16 count;             
    PRUint16 length;            
    PRRecvWait *recv_wait;      
} _PRWaiterHash;

typedef enum {_prmw_running, _prmw_stopping, _prmw_stopped} PRMWGroupState;

struct PRWaitGroup
{
    PRCList group_link;         
    PRCList io_ready;           
    PRMWGroupState state;       

    PRLock *ml;                 
    PRCondVar *io_taken;        
    PRCondVar *io_complete;     
    PRCondVar *new_business;    
    PRCondVar *mw_manage;       
    PRThread* poller;           
    PRUint16 waiting_threads;   
    PRUint16 polling_count;     
    PRUint32 p_timestamp;       
    PRPollDesc *polling_list;   
    PRIntervalTime last_poll;   
    _PRWaiterHash *waiter;      

#ifdef WINNT
    





    _MDLock mdlock;             
    PRCList wait_list;          

#endif 
};






typedef struct _PRGlobalState
{
    PRCList group_list;         
    PRWaitGroup *group;         
} _PRGlobalState;

#ifdef WINNT
extern PRStatus NT_HashRemoveInternal(PRWaitGroup *group, PRFileDesc *fd);
#endif

typedef enum {_PR_ENUM_UNSEALED=0, _PR_ENUM_SEALED=0x0eadface} _PREnumSeal;

struct PRMWaitEnumerator
{
    PRWaitGroup *group;       
    PRThread *thread;               
    _PREnumSeal seal;               
    PRUint32 p_timestamp;           
    PRRecvWait **waiter;            
    PRUintn index;                  
    void *pad[4];                   
};

#endif 


