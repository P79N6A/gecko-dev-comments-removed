







































#include "nsDeleteDir.h"
#include "nsIFile.h"
#include "nsString.h"
#include "mozilla/Telemetry.h"
#include "nsITimer.h"
#include "nsISimpleEnumerator.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsISupportsPriority.h"

using namespace mozilla;

class nsBlockOnBackgroundThreadEvent : public nsRunnable {
public:
  nsBlockOnBackgroundThreadEvent() {}
  NS_IMETHOD Run()
  {
    MutexAutoLock lock(nsDeleteDir::gInstance->mLock);
    nsDeleteDir::gInstance->mCondVar.Notify();
    return NS_OK;
  }
};

class nsDestroyThreadEvent : public nsRunnable {
public:
  nsDestroyThreadEvent(nsIThread *thread)
    : mThread(thread)
  {}
  NS_IMETHOD Run()
  {
    mThread->Shutdown();
    return NS_OK;
  }
private:
  nsCOMPtr<nsIThread> mThread;
};


nsDeleteDir * nsDeleteDir::gInstance = nsnull;

nsDeleteDir::nsDeleteDir()
  : mLock("nsDeleteDir.mLock"),
    mCondVar(mLock, "nsDeleteDir.mCondVar"),
    mShutdownPending(false),
    mStopDeleting(false)
{
  NS_ASSERTION(gInstance==nsnull, "multiple nsCacheService instances!");
}

nsDeleteDir::~nsDeleteDir()
{
  gInstance = nsnull;
}

nsresult
nsDeleteDir::Init()
{
  if (gInstance)
    return NS_ERROR_ALREADY_INITIALIZED;

  gInstance = new nsDeleteDir();
  return NS_OK;
}

nsresult
nsDeleteDir::Shutdown(bool finishDeleting)
{
  if (!gInstance)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMArray<nsIFile> dirsToRemove;
  nsCOMPtr<nsIThread> thread;
  {
    MutexAutoLock lock(gInstance->mLock);
    NS_ASSERTION(!gInstance->mShutdownPending,
                 "Unexpected state in nsDeleteDir::Shutdown()");
    gInstance->mShutdownPending = true;

    if (!finishDeleting)
      gInstance->mStopDeleting = true;

    
    for (PRInt32 i = gInstance->mTimers.Count(); i > 0; i--) {
      nsCOMPtr<nsITimer> timer = gInstance->mTimers[i-1];
      gInstance->mTimers.RemoveObjectAt(i-1);
      timer->Cancel();

      nsCOMArray<nsIFile> *arg;
      timer->GetClosure((reinterpret_cast<void**>(&arg)));

      if (finishDeleting)
        dirsToRemove.AppendObjects(*arg);

      
      delete arg;
    }

    thread.swap(gInstance->mThread);
    if (thread) {
      
      
      nsCOMPtr<nsIRunnable> event = new nsBlockOnBackgroundThreadEvent();
      nsresult rv = thread->Dispatch(event, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed dispatching block-event");
        return NS_ERROR_UNEXPECTED;
      }

      rv = gInstance->mCondVar.Wait();
      thread->Shutdown();
    }
  }

  delete gInstance;

  for (PRInt32 i = 0; i < dirsToRemove.Count(); i++)
    dirsToRemove[i]->Remove(true);

  return NS_OK;
}

nsresult
nsDeleteDir::InitThread()
{
  if (mThread)
    return NS_OK;

  nsresult rv = NS_NewThread(getter_AddRefs(mThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create background thread");
    return rv;
  }

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mThread);
  if (p) {
    p->SetPriority(nsISupportsPriority::PRIORITY_LOWEST);
  }
  return NS_OK;
}

void
nsDeleteDir::DestroyThread()
{
  if (!mThread)
    return;

  if (mTimers.Count())
    
    return;

  NS_DispatchToMainThread(new nsDestroyThreadEvent(mThread));
  mThread = nsnull;
}

void
nsDeleteDir::TimerCallback(nsITimer *aTimer, void *arg)
{
  Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_DELETEDIR> timer;
  {
    MutexAutoLock lock(gInstance->mLock);

    PRInt32 idx = gInstance->mTimers.IndexOf(aTimer);
    if (idx == -1) {
      
      return;
    }

    gInstance->mTimers.RemoveObjectAt(idx);
  }

  nsAutoPtr<nsCOMArray<nsIFile> > dirList;
  dirList = static_cast<nsCOMArray<nsIFile> *>(arg);

  bool shuttingDown = false;
  for (PRInt32 i = 0; i < dirList->Count() && !shuttingDown; i++) {
    gInstance->RemoveDir((*dirList)[i], &shuttingDown);
  }

  {
    MutexAutoLock lock(gInstance->mLock);
    gInstance->DestroyThread();
  }
}

nsresult
nsDeleteDir::DeleteDir(nsIFile *dirIn, bool moveToTrash, PRUint32 delay)
{
  Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_TRASHRENAME> timer;

  if (!gInstance)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv;
  nsCOMPtr<nsIFile> trash, dir;

  
  
  rv = dirIn->Clone(getter_AddRefs(dir));
  if (NS_FAILED(rv))
    return rv;

  if (moveToTrash) {
    rv = GetTrashDir(dir, &trash);
    if (NS_FAILED(rv))
      return rv;
    nsCAutoString origLeaf;
    rv = trash->GetNativeLeafName(origLeaf);
    if (NS_FAILED(rv))
      return rv;

    
    
    
    
    nsCAutoString leaf;
    for (PRInt32 i = 0; i < 10; i++) {
      leaf = origLeaf;
      leaf.AppendInt(rand());
      rv = trash->SetNativeLeafName(leaf);
      if (NS_FAILED(rv))
        return rv;

      bool exists;
      if (NS_SUCCEEDED(trash->Exists(&exists)) && !exists) {
        break;
      }

      leaf.Truncate();
    }

    
    if (!leaf.Length())
      return NS_ERROR_FAILURE;

    rv = dir->MoveToNative(nsnull, leaf);
    if (NS_FAILED(rv))
      return rv;
  } else {
    
    trash.swap(dir);
  }

  nsAutoPtr<nsCOMArray<nsIFile> > arg(new nsCOMArray<nsIFile>);
  arg->AppendObject(trash);

  rv = gInstance->PostTimer(arg, delay);
  if (NS_FAILED(rv))
    return rv;

  arg.forget();
  return NS_OK;
}

nsresult
nsDeleteDir::GetTrashDir(nsIFile *target, nsCOMPtr<nsIFile> *result)
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

nsresult
nsDeleteDir::RemoveOldTrashes(nsIFile *cacheDir)
{
  if (!gInstance)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv;

  static bool firstRun = true;

  if (!firstRun)
    return NS_OK;

  firstRun = false;

  nsCOMPtr<nsIFile> trash;
  rv = GetTrashDir(cacheDir, &trash);
  if (NS_FAILED(rv))
    return rv;

  nsAutoString trashName;
  rv = trash->GetLeafName(trashName);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> parent;
  rv = cacheDir->GetParent(getter_AddRefs(parent));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = parent->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv))
    return rv;

  bool more;
  nsCOMPtr<nsISupports> elem;
  nsAutoPtr<nsCOMArray<nsIFile> > dirList;

  while (NS_SUCCEEDED(iter->HasMoreElements(&more)) && more) {
    rv = iter->GetNext(getter_AddRefs(elem));
    if (NS_FAILED(rv))
      continue;

    nsCOMPtr<nsIFile> file = do_QueryInterface(elem);
    if (!file)
      continue;

    nsAutoString leafName;
    rv = file->GetLeafName(leafName);
    if (NS_FAILED(rv))
      continue;

    
    if (Substring(leafName, 0, trashName.Length()).Equals(trashName)) {
      if (!dirList)
        dirList = new nsCOMArray<nsIFile>;
      dirList->AppendObject(file);
    }
  }

  if (dirList) {
    rv = gInstance->PostTimer(dirList, 90000);
    if (NS_FAILED(rv))
      return rv;

    dirList.forget();
  }

  return NS_OK;
}

nsresult
nsDeleteDir::PostTimer(void *arg, PRUint32 delay)
{
  nsresult rv;

  nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_FAILED(rv))
    return NS_ERROR_UNEXPECTED;

  MutexAutoLock lock(mLock);

  rv = InitThread();
  if (NS_FAILED(rv))
    return rv;

  rv = timer->SetTarget(mThread);
  if (NS_FAILED(rv))
    return rv;

  rv = timer->InitWithFuncCallback(TimerCallback, arg, delay,
                                   nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv))
    return rv;

  mTimers.AppendObject(timer);
  return NS_OK;
}

nsresult
nsDeleteDir::RemoveDir(nsIFile *file, bool *stopDeleting)
{
  nsresult rv;
  bool isLink;

  rv = file->IsSymlink(&isLink);
  if (NS_FAILED(rv) || isLink)
    return NS_ERROR_UNEXPECTED;

  bool isDir;
  rv = file->IsDirectory(&isDir);
  if (NS_FAILED(rv))
    return rv;

  if (isDir) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    rv = file->GetDirectoryEntries(getter_AddRefs(iter));
    if (NS_FAILED(rv))
      return rv;

    bool more;
    nsCOMPtr<nsISupports> elem;
    while (NS_SUCCEEDED(iter->HasMoreElements(&more)) && more) {
      rv = iter->GetNext(getter_AddRefs(elem));
      if (NS_FAILED(rv)) {
        NS_WARNING("Unexpected failure in nsDeleteDir::RemoveDir");
        continue;
      }

      nsCOMPtr<nsIFile> file2 = do_QueryInterface(elem);
      if (!file2) {
        NS_WARNING("Unexpected failure in nsDeleteDir::RemoveDir");
        continue;
      }

      RemoveDir(file2, stopDeleting);
      

      if (*stopDeleting)
        return NS_OK;
    }
  }

  file->Remove(false);
  

  MutexAutoLock lock(mLock);
  if (mStopDeleting)
    *stopDeleting = true;

  return NS_OK;
}
