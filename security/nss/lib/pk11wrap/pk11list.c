







#include "seccomon.h"
#include "nssilock.h"
#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "nssrwlk.h"




SECMODListLock *SECMOD_NewListLock()
{
    return NSSRWLock_New( 10, "moduleListLock");
}




void SECMOD_DestroyListLock(SECMODListLock *lock) 
{
    NSSRWLock_Destroy(lock);
}






void SECMOD_GetReadLock(SECMODListLock *modLock) 
{
    NSSRWLock_LockRead(modLock);
}




void SECMOD_ReleaseReadLock(SECMODListLock *modLock) 
{
    NSSRWLock_UnlockRead(modLock);
}





void SECMOD_GetWriteLock(SECMODListLock *modLock) 
{
    NSSRWLock_LockWrite(modLock);
}






void SECMOD_ReleaseWriteLock(SECMODListLock *modLock) 
{
    NSSRWLock_UnlockWrite(modLock);
}





void
SECMOD_RemoveList(SECMODModuleList **parent, SECMODModuleList *child) 
{
    *parent = child->next;
    child->next = NULL;
}




void
SECMOD_AddList(SECMODModuleList *parent, SECMODModuleList *child, 
							SECMODListLock *lock) 
{
    if (lock) { SECMOD_GetWriteLock(lock); }

    child->next = parent->next;
    parent->next = child;

   if (lock) { SECMOD_ReleaseWriteLock(lock); }
}


