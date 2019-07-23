





































#include "nsWinProfileItem.h"
#include "nspr.h"
#include <windows.h>



nsWinProfileItem::nsWinProfileItem(nsWinProfile* profileObj, 
                                   nsString sectionName,
                                   nsString keyName,
                                   nsString val,
                                   PRInt32 *aReturn) : nsInstallObject(profileObj->InstallObject())
{
  MOZ_COUNT_CTOR(nsWinProfileItem);

  mProfile = profileObj;
  mSection = new nsString(sectionName);
  mKey     = new nsString(keyName);
  mValue   = new nsString(val);

  *aReturn = nsInstall::SUCCESS;

  if((mSection == nsnull) ||
     (mKey     == nsnull) ||
     (mValue   == nsnull))
  {
    *aReturn = nsInstall::OUT_OF_MEMORY;
  }
}

nsWinProfileItem::~nsWinProfileItem()
{
  if (mSection) delete mSection;
  if (mKey)     delete mKey;
  if (mValue)   delete mValue;

  MOZ_COUNT_DTOR(nsWinProfileItem);
}

PRInt32 nsWinProfileItem::Complete()
{
	if (mProfile) 
        mProfile->FinalWriteString(*mSection, *mKey, *mValue);
	
    return NS_OK;
}
  
char* nsWinProfileItem::toString()
{
  char*     resultCString;
  
  nsString* filename = new nsString(mProfile->GetFilename());
  nsString* result = new nsString;
  result->AssignLiteral("Write ");

  if (filename == nsnull || result == nsnull)
      return nsnull;

  result->Append(*filename);
  result->AppendLiteral(": [");
  result->Append(*mSection);
  result->AppendLiteral("] ");
  result->Append(*mKey);
  result->AppendLiteral("=");
  result->Append(*mValue);

  resultCString = ToNewCString(*result);
  
  if (result)   delete result;
  if (filename) delete filename;

  return resultCString;
}

void nsWinProfileItem::Abort()
{
}

PRInt32 nsWinProfileItem::Prepare()
{
	return nsnull;
}






PRBool 
nsWinProfileItem::CanUninstall()
{
    return PR_FALSE;
}





PRBool
nsWinProfileItem::RegisterPackageNode()
{
    return PR_TRUE;
}

