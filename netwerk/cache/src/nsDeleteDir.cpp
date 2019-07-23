





































#include "nsDeleteDir.h"
#include "nsIFile.h"
#include "nsString.h"
#include "prthread.h"

PR_STATIC_CALLBACK(void) DeleteDirThreadFunc(void *arg)
{
  nsIFile *dir = NS_STATIC_CAST(nsIFile *, arg);
  dir->Remove(PR_TRUE);
  NS_RELEASE(dir);
}

nsresult DeleteDir(nsIFile *dirIn, PRBool moveToTrash, PRBool sync)
{
  nsresult rv;
  nsCOMPtr<nsIFile> trash, dir;

  
  
  rv = dirIn->Clone(getter_AddRefs(dir));
  if (NS_FAILED(rv))
    return rv;

  if (moveToTrash)
  {
    rv = GetTrashDir(dir, &trash);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIFile> subDir;
    rv = trash->Clone(getter_AddRefs(subDir));
    if (NS_FAILED(rv))
      return rv;

    rv = subDir->AppendNative(NS_LITERAL_CSTRING("Trash"));
    if (NS_FAILED(rv))
      return rv;

    rv = subDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
    if (NS_FAILED(rv))
      return rv;

    rv = dir->MoveToNative(subDir, EmptyCString());
    if (NS_FAILED(rv))
      return rv;
  }
  else
  {
    
    trash.swap(dir);
  }

  
  nsIFile *trashRef = nsnull;
  trash.swap(trashRef);

  if (sync)
  {
    DeleteDirThreadFunc(trashRef);
  }
  else
  {
    
    PRThread *thread = PR_CreateThread(PR_USER_THREAD,
                                       DeleteDirThreadFunc,
                                       trashRef,
                                       PR_PRIORITY_LOW,
                                       PR_GLOBAL_THREAD,
                                       PR_UNJOINABLE_THREAD,
                                       0);
    if (!thread)
      return NS_ERROR_UNEXPECTED;
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
