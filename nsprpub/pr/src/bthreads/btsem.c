




































#include <kernel/OS.h>

#include "primpl.h"




PR_IMPLEMENT(PRSemaphore*)
    PR_NewSem (PRUintn value)
{
	PRSemaphore *semaphore;

	if (!_pr_initialized) _PR_ImplicitInitialization();

	semaphore = PR_NEWZAP(PRSemaphore);
	if (NULL != semaphore) {
		if ((semaphore->sem = create_sem(value, "nspr_sem")) < B_NO_ERROR)
			return NULL;
		else 
			return semaphore;
	}
	return NULL;
}





PR_IMPLEMENT(void)
    PR_DestroySem (PRSemaphore *sem)
{
	status_t result;

	PR_ASSERT(sem != NULL);
	result = delete_sem(sem->sem);
	PR_ASSERT(result == B_NO_ERROR);
	PR_DELETE(sem);
} 















PR_IMPLEMENT(PRStatus)
    PR_WaitSem (PRSemaphore *sem)
{
	PR_ASSERT(sem != NULL);
	if (acquire_sem(sem->sem) == B_NO_ERROR)
		return PR_SUCCESS;
	else
		return PR_FAILURE;
}






PR_IMPLEMENT(void)
    PR_PostSem (PRSemaphore *sem)
{
	status_t result;

	PR_ASSERT(sem != NULL);
	result = release_sem_etc(sem->sem, 1, B_DO_NOT_RESCHEDULE);
	PR_ASSERT(result == B_NO_ERROR);
}







PR_IMPLEMENT(PRUintn)
    PR_GetValueSem (PRSemaphore *sem)
{
	sem_info	info;

	PR_ASSERT(sem != NULL);
	get_sem_info(sem->sem, &info);
	return info.count;
}
