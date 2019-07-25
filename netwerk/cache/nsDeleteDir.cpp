






































#include "nsDeleteDir.h"
#include "nsIFile.h"
#include "nsString.h"
#include "prthread.h"
#include "mozilla/Telemetry.h"
#include "nsITimer.h"

using namespace mozilla;

static void DeleteDirThreadFunc(void *arg)
{
  Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_DELETEDIR> timer;
  nsIFile *dir = static_cast<nsIFile *>(arg);
  dir->Remove(true);
  NS_RELEASE(dir);
}

static void CreateDeleterThread(nsITimer *aTimer, void *arg)
{
  nsIFile *dir = static_cast<nsIFile *>(arg);

  
  PR_CreateThread(PR_USER_THREAD, DeleteDirThreadFunc, dir, PR_PRIORITY_LOW,
                  PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD, 0);
}

nsresult DeleteDir(nsIFile *dirIn, bool moveToTrash, bool sync,
                   PRUint32 delay)
{
  Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_TRASHRENAME> timer;
  nsresult rv;
  nsCOMPtr<nsIFile> trash, dir;

  
  
  rv = dirIn->Clone(getter_AddRefs(dir));
  if (NS_FAILED(rv))
    return rv;

  if (moveToTrash) {
    rv = GetTrashDir(dir, &trash);
    if (NS_FAILED(rv))
      return rv;
    nsCAutoString leaf;
    rv = trash->GetNativeLeafName(leaf);
    if (NS_FAILED(rv))
      return rv;

    
    
    
    rv = dir->MoveToNative(nsnull, leaf);
    if (NS_FAILED(rv)) {
      nsresult rvMove = rv;
      
      
      
      leaf.AppendInt(rand()); 
      rv = dir->MoveToNative(trash, leaf);
      if (NS_FAILED(rv))
        return rvMove;
      
      
      delay = 0;
    }
  } else {
    
    trash.swap(dir);
  }

  
  nsIFile *trashRef = nsnull;
  trash.swap(trashRef);

  if (sync) {
    DeleteDirThreadFunc(trashRef);
  } else {
    if (delay) {
      nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1", &rv);
      if (NS_FAILED(rv))
        return NS_ERROR_UNEXPECTED;
      timer->InitWithFuncCallback(CreateDeleterThread, trashRef, delay,
                                  nsITimer::TYPE_ONE_SHOT);
    } else {
      CreateDeleterThread(nsnull, trashRef);
    }
  }

  return NS_OK;
}

nsresult GetTrashDir(nsIFile *target, nsCOMPtr<nsIFile> *result)
{
  nsresult rv = target->Clone(getter_AddRefs(*result));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString leaf;
  rv = (*result)->GetNativeLeafName(leaf);
  if (NS_FAILED(rv))
    return rv;
  leaf.AppendLiteral(".Trash");

  return (*result)->SetNativeLeafName(leaf);
}
