




































#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsVoidArray.h"
#include "nsISimpleEnumerator.h"
#include "nsXPCOM.h"
#include "nsSupportsPrimitives.h"
#include "nsIComponentManager.h"

#include "nsCommandGroup.h"
#include "nsIControllerCommand.h"
#include "nsCRT.h"


class nsGroupsEnumerator : public nsISimpleEnumerator
{
public:
              nsGroupsEnumerator(nsHashtable& inHashTable);
  virtual     ~nsGroupsEnumerator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

protected:

  static PRBool PR_CALLBACK HashEnum(nsHashKey *aKey, void *aData, void* aClosure);

  nsresult      Initialize();

protected:

  nsHashtable&  mHashTable;
  PRInt32       mIndex;
  char **       mGroupNames;        
  PRBool        mInitted;
  
};


NS_IMPL_ISUPPORTS1(nsGroupsEnumerator, nsISimpleEnumerator)

nsGroupsEnumerator::nsGroupsEnumerator(nsHashtable& inHashTable)
: mHashTable(inHashTable)
, mIndex(-1)
, mGroupNames(nsnull)
, mInitted(PR_FALSE)
{
  
}

nsGroupsEnumerator::~nsGroupsEnumerator()
{
  delete [] mGroupNames;    
}


NS_IMETHODIMP
nsGroupsEnumerator::HasMoreElements(PRBool *_retval)
{
  nsresult  rv = NS_OK;
  
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mInitted) {
    rv = Initialize();
    if (NS_FAILED(rv)) return rv;
  }
  
  *_retval = (mIndex < mHashTable.Count() - 1); 
  return NS_OK;
}


NS_IMETHODIMP
nsGroupsEnumerator::GetNext(nsISupports **_retval)
{
  nsresult  rv = NS_OK;
  
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mInitted) {
    rv = Initialize();
    if (NS_FAILED(rv)) return rv;
  }
  
  mIndex ++;
  if (mIndex >= mHashTable.Count())
    return NS_ERROR_FAILURE;

  char *thisGroupName = mGroupNames[mIndex];
  
  nsCOMPtr<nsISupportsCString> supportsString = do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  supportsString->SetData(nsDependentCString(thisGroupName));
  return CallQueryInterface(supportsString, _retval);
}



PRBool
nsGroupsEnumerator::HashEnum(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsGroupsEnumerator*   groupsEnum = NS_REINTERPRET_CAST(nsGroupsEnumerator *, aClosure);
  nsCStringKey*         stringKey = NS_STATIC_CAST(nsCStringKey*, aKey);
  
  groupsEnum->mGroupNames[groupsEnum->mIndex] = (char*)stringKey->GetString();
  groupsEnum->mIndex ++;
  return PR_TRUE;
}

nsresult
nsGroupsEnumerator::Initialize()
{
  if (mInitted) return NS_OK;
  
  mGroupNames = new char*[mHashTable.Count()];
  if (!mGroupNames) return NS_ERROR_OUT_OF_MEMORY;
  
  mIndex = 0; 
  mHashTable.Enumerate(HashEnum, (void*)this);

  mIndex = -1;
  mInitted = PR_TRUE;
  return NS_OK;
}

#if 0
#pragma mark -
#endif

class nsNamedGroupEnumerator : public nsISimpleEnumerator
{
public:
              nsNamedGroupEnumerator(nsVoidArray* inArray);
  virtual     ~nsNamedGroupEnumerator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

protected:

  nsVoidArray*  mGroupArray;
  PRInt32       mIndex;
  
};

nsNamedGroupEnumerator::nsNamedGroupEnumerator(nsVoidArray* inArray)
: mGroupArray(inArray)
, mIndex(-1)
{
}

nsNamedGroupEnumerator::~nsNamedGroupEnumerator()
{
}

NS_IMPL_ISUPPORTS1(nsNamedGroupEnumerator, nsISimpleEnumerator)


NS_IMETHODIMP
nsNamedGroupEnumerator::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  PRInt32   arrayLen = mGroupArray ? mGroupArray->Count() : 0;
  *_retval = (mIndex < arrayLen - 1); 
  return NS_OK;
}


NS_IMETHODIMP
nsNamedGroupEnumerator::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mGroupArray)
    return NS_ERROR_FAILURE;

  mIndex ++;
  if (mIndex >= mGroupArray->Count())
    return NS_ERROR_FAILURE;
    
  PRUnichar   *thisGroupName = (PRUnichar *)mGroupArray->ElementAt(mIndex);
  NS_ASSERTION(thisGroupName, "Bad Element in mGroupArray");
  
  nsresult rv;
  nsCOMPtr<nsISupportsString> supportsString = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  supportsString->SetData(nsDependentString(thisGroupName));
  return CallQueryInterface(supportsString, _retval);
}

#if 0
#pragma mark -
#endif



NS_IMPL_ISUPPORTS1(nsControllerCommandGroup, nsIControllerCommandGroup)

nsControllerCommandGroup::nsControllerCommandGroup()
{
}

nsControllerCommandGroup::~nsControllerCommandGroup()
{
  ClearGroupsHash();
}

void
nsControllerCommandGroup::ClearGroupsHash()
{
    mGroupsHash.Reset(ClearEnumerator, (void *)this);
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP
nsControllerCommandGroup::AddCommandToGroup(const char * aCommand, const char *aGroup)
{
  nsCStringKey   groupKey(aGroup);  
  nsVoidArray*  commandList;  
  if ((commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey)) == nsnull)
  {
    
    commandList = new nsAutoVoidArray;    
    mGroupsHash.Put(&groupKey, (void *)commandList);
  }
  
  char*  commandString = nsCRT::strdup(aCommand);     
  if (!commandString) return NS_ERROR_OUT_OF_MEMORY;
  
  PRBool      appended = commandList->AppendElement((void *)commandString);
  NS_ASSERTION(appended, "Append failed");

  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandGroup::RemoveCommandFromGroup(const char * aCommand, const char * aGroup)
{
  nsCStringKey   groupKey(aGroup);
  nsVoidArray*  commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey);
  if (!commandList) return NS_OK;     

  PRInt32   numEntries = commandList->Count();
  for (PRInt32 i = 0; i < numEntries; i ++)
  {
    char*  commandString = (char*)commandList->ElementAt(i);
    if (!nsCRT::strcmp(aCommand,commandString))
    {
      commandList->RemoveElementAt(i);
      nsMemory::Free(commandString);
      break;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandGroup::IsCommandInGroup(const char * aCommand, const char * aGroup, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  nsCStringKey   groupKey(aGroup);
  nsVoidArray*  commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey);
  if (!commandList) return NS_OK;     
  
  PRInt32   numEntries = commandList->Count();
  for (PRInt32 i = 0; i < numEntries; i ++)
  {
    char*  commandString = (char*)commandList->ElementAt(i);
    if (!nsCRT::strcmp(aCommand,commandString))
    {
      *_retval = PR_TRUE;
      break;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandGroup::GetGroupsEnumerator(nsISimpleEnumerator **_retval)
{
  nsGroupsEnumerator*   groupsEnum = new nsGroupsEnumerator(mGroupsHash);
  if (!groupsEnum) return NS_ERROR_OUT_OF_MEMORY;

  return groupsEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void **)_retval);
}


NS_IMETHODIMP
nsControllerCommandGroup::GetEnumeratorForGroup(const char * aGroup, nsISimpleEnumerator **_retval)
{
  nsCStringKey   groupKey(aGroup);  
  nsVoidArray*  commandList = (nsVoidArray *)mGroupsHash.Get(&groupKey);    

  nsNamedGroupEnumerator*   theGroupEnum = new nsNamedGroupEnumerator(commandList);
  if (!theGroupEnum) return NS_ERROR_OUT_OF_MEMORY;

  return theGroupEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void **)_retval);
}

#if 0
#pragma mark -
#endif
 
PRBool nsControllerCommandGroup::ClearEnumerator(nsHashKey *aKey, void *aData, void* closure)
{
  nsVoidArray*    commandList = (nsVoidArray *)aData;
  if (commandList)
  {  
    PRInt32   numEntries = commandList->Count();
    for (PRInt32 i = 0; i < numEntries; i ++)
    {
      char*  commandString = (char*)commandList->ElementAt(i);
      nsMemory::Free(commandString);
    }
    
    delete commandList;
  }

  return PR_TRUE;
}
