




#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsTArray.h"
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
  nsGroupsEnumerator(nsControllerCommandGroup::GroupsHashtable &inHashTable);
  virtual ~nsGroupsEnumerator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

protected:
  static PLDHashOperator HashEnum(const nsACString &aKey, nsTArray<char*> *aData, void *aClosure);
  nsresult Initialize();

protected:

  nsControllerCommandGroup::GroupsHashtable &mHashTable;
  int32_t mIndex;
  char **mGroupNames;        
  bool mInitted;
  
};


NS_IMPL_ISUPPORTS(nsGroupsEnumerator, nsISimpleEnumerator)

nsGroupsEnumerator::nsGroupsEnumerator(nsControllerCommandGroup::GroupsHashtable &inHashTable)
: mHashTable(inHashTable)
, mIndex(-1)
, mGroupNames(nullptr)
, mInitted(false)
{
  
}

nsGroupsEnumerator::~nsGroupsEnumerator()
{
  delete [] mGroupNames;    
}


NS_IMETHODIMP
nsGroupsEnumerator::HasMoreElements(bool *_retval)
{
  nsresult rv = NS_OK;
  
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mInitted) {
    rv = Initialize();
    if (NS_FAILED(rv)) return rv;
  }
  
  *_retval = (mIndex < static_cast<int32_t>(mHashTable.Count()) - 1);
  return NS_OK;
}


NS_IMETHODIMP
nsGroupsEnumerator::GetNext(nsISupports **_retval)
{
  nsresult rv = NS_OK;
  
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mInitted) {
    rv = Initialize();
    if (NS_FAILED(rv)) return rv;
  }
  
  mIndex ++;
  if (mIndex >= static_cast<int32_t>(mHashTable.Count()))
    return NS_ERROR_FAILURE;

  char *thisGroupName = mGroupNames[mIndex];
  
  nsCOMPtr<nsISupportsCString> supportsString = do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  supportsString->SetData(nsDependentCString(thisGroupName));
  return CallQueryInterface(supportsString, _retval);
}



PLDHashOperator
nsGroupsEnumerator::HashEnum(const nsACString &aKey, nsTArray<char*> *aData, void *aClosure)
{
  nsGroupsEnumerator *groupsEnum = static_cast<nsGroupsEnumerator*>(aClosure);
  groupsEnum->mGroupNames[groupsEnum->mIndex] = (char*)aKey.Data();
  groupsEnum->mIndex++;
  return PL_DHASH_NEXT;
}

nsresult
nsGroupsEnumerator::Initialize()
{
  if (mInitted) return NS_OK;
  
  mGroupNames = new char*[mHashTable.Count()];
  if (!mGroupNames) return NS_ERROR_OUT_OF_MEMORY;
  
  mIndex = 0; 
  mHashTable.EnumerateRead(HashEnum, this);

  mIndex = -1;
  mInitted = true;
  return NS_OK;
}

#if 0
#pragma mark -
#endif

class nsNamedGroupEnumerator : public nsISimpleEnumerator
{
public:
  nsNamedGroupEnumerator(nsTArray<char*> *inArray);
  virtual ~nsNamedGroupEnumerator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

protected:
  nsTArray<char*> *mGroupArray;
  int32_t mIndex;
};

nsNamedGroupEnumerator::nsNamedGroupEnumerator(nsTArray<char*> *inArray)
: mGroupArray(inArray)
, mIndex(-1)
{
}

nsNamedGroupEnumerator::~nsNamedGroupEnumerator()
{
}

NS_IMPL_ISUPPORTS(nsNamedGroupEnumerator, nsISimpleEnumerator)


NS_IMETHODIMP
nsNamedGroupEnumerator::HasMoreElements(bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  int32_t arrayLen = mGroupArray ? mGroupArray->Length() : 0;
  *_retval = (mIndex < arrayLen - 1); 
  return NS_OK;
}


NS_IMETHODIMP
nsNamedGroupEnumerator::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (!mGroupArray)
    return NS_ERROR_FAILURE;

  mIndex++;
  if (mIndex >= int32_t(mGroupArray->Length()))
    return NS_ERROR_FAILURE;
    
  char16_t *thisGroupName = (char16_t*)mGroupArray->ElementAt(mIndex);
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



NS_IMPL_ISUPPORTS(nsControllerCommandGroup, nsIControllerCommandGroup)

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
  mGroupsHash.EnumerateRead(ClearEnumerator, nullptr);
  mGroupsHash.Clear();
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP
nsControllerCommandGroup::AddCommandToGroup(const char *aCommand, const char *aGroup)
{
  nsDependentCString groupKey(aGroup);
  nsTArray<char*> *commandList;
  if ((commandList = mGroupsHash.Get(groupKey)) == nullptr)
  {
    
    commandList = new nsAutoTArray<char*, 8>;
    mGroupsHash.Put(groupKey, commandList);
  }
  
  char *commandString = NS_strdup(aCommand); 
  if (!commandString) return NS_ERROR_OUT_OF_MEMORY;
  
#ifdef DEBUG
  char **appended =
#endif
  commandList->AppendElement(commandString);
  NS_ASSERTION(appended, "Append failed");

  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandGroup::RemoveCommandFromGroup(const char *aCommand, const char *aGroup)
{
  nsDependentCString groupKey(aGroup);
  nsTArray<char*> *commandList = mGroupsHash.Get(groupKey);
  if (!commandList) return NS_OK; 

  uint32_t numEntries = commandList->Length();
  for (uint32_t i = 0; i < numEntries; i++)
  {
    char *commandString = commandList->ElementAt(i);
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
nsControllerCommandGroup::IsCommandInGroup(const char *aCommand, const char *aGroup, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = false;
  
  nsDependentCString groupKey(aGroup);
  nsTArray<char*> *commandList = mGroupsHash.Get(groupKey);
  if (!commandList) return NS_OK; 
  
  uint32_t numEntries = commandList->Length();
  for (uint32_t i = 0; i < numEntries; i++)
  {
    char *commandString = commandList->ElementAt(i);
    if (!nsCRT::strcmp(aCommand,commandString))
    {
      *_retval = true;
      break;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandGroup::GetGroupsEnumerator(nsISimpleEnumerator **_retval)
{
  nsGroupsEnumerator *groupsEnum = new nsGroupsEnumerator(mGroupsHash);
  if (!groupsEnum) return NS_ERROR_OUT_OF_MEMORY;

  return groupsEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void **)_retval);
}


NS_IMETHODIMP
nsControllerCommandGroup::GetEnumeratorForGroup(const char *aGroup, nsISimpleEnumerator **_retval)
{
  nsDependentCString groupKey(aGroup);
  nsTArray<char*> *commandList = mGroupsHash.Get(groupKey); 

  nsNamedGroupEnumerator*   theGroupEnum = new nsNamedGroupEnumerator(commandList);
  if (!theGroupEnum) return NS_ERROR_OUT_OF_MEMORY;

  return theGroupEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void **)_retval);
}

#if 0
#pragma mark -
#endif

PLDHashOperator
nsControllerCommandGroup::ClearEnumerator(const nsACString &aKey, nsTArray<char*> *aData, void *closure)
{
  nsTArray<char*> *commandList = aData;
  if (commandList)
  {  
    uint32_t numEntries = commandList->Length();
    for (uint32_t i = 0; i < numEntries; i++)
    {
      char *commandString = commandList->ElementAt(i);
      nsMemory::Free(commandString);
    }
  }
  return PL_DHASH_NEXT;
}
