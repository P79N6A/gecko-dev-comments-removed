




































#include "nsWinRegItem.h"
#include "nspr.h"
#include "nsWinReg.h"

#ifdef WIN32
#include <windows.h> 
#endif



nsWinRegItem::nsWinRegItem(nsWinReg* regObj, PRInt32 root, PRInt32 action, const nsAString& sub, const nsAString& valname, const nsAString& val, PRInt32 *aReturn)
: nsInstallObject(regObj->InstallObject())
{
    MOZ_COUNT_CTOR(nsWinRegItem);

    mReg     = regObj;
	mCommand = action;
	mRootkey = root;

  *aReturn = nsInstall::SUCCESS;

  
	mSubkey  = new nsString(sub);
	mName    = new nsString(valname);
	mValue   = new nsString(val);

  if((mSubkey == nsnull) ||
     (mName   == nsnull) ||
     (mValue  == nsnull))
  {
    *aReturn = nsInstall::OUT_OF_MEMORY;
  }
}

nsWinRegItem::nsWinRegItem(nsWinReg* regObj, PRInt32 root, PRInt32 action, const nsAString& sub, const nsAString& valname, PRInt32 val, PRInt32 *aReturn)
: nsInstallObject(regObj->InstallObject())
{
    MOZ_COUNT_CTOR(nsWinRegItem);

	mReg     = regObj;
	mCommand = action;
	mRootkey = root;

  *aReturn = nsInstall::SUCCESS;

  
	mSubkey  = new nsString(sub);
	mName    = new nsString(valname);
	mValue   = new PRInt32(val);

  if((mSubkey == nsnull) ||
     (mName   == nsnull) ||
     (mValue  == nsnull))
  {
    *aReturn = nsInstall::OUT_OF_MEMORY;
  }
}

nsWinRegItem::~nsWinRegItem()
{
  if (mSubkey)  delete mSubkey;
  if (mName)    delete mName;
  if (mValue)   delete mValue;
  MOZ_COUNT_DTOR(nsWinRegItem);
}

PRInt32 nsWinRegItem::Complete()
{
  PRInt32 aReturn = NS_OK;
  
  if (mReg == nsnull)
      return nsInstall::OUT_OF_MEMORY;

  switch (mCommand)
  {
    case NS_WIN_REG_CREATE:
        mReg->FinalCreateKey(mRootkey, *mSubkey, *mName, &aReturn);
        break;
    
    case NS_WIN_REG_DELETE:
        mReg->FinalDeleteKey(mRootkey, *mSubkey, &aReturn);
        break;
    
    case NS_WIN_REG_DELETE_VAL:
        mReg->FinalDeleteValue(mRootkey, *mSubkey, *mName, &aReturn);
        break;
    
    case NS_WIN_REG_SET_VAL_STRING:
        mReg->FinalSetValueString(mRootkey, *mSubkey, *mName, *(nsString*)mValue, &aReturn);
        break;

    case NS_WIN_REG_SET_VAL_NUMBER:
        mReg->FinalSetValueNumber(mRootkey, *mSubkey, *mName, *(PRInt32*)mValue, &aReturn);
        break;
    
    case NS_WIN_REG_SET_VAL:
        mReg->FinalSetValue(mRootkey, *mSubkey, *mName, (nsWinRegValue*)mValue, &aReturn);
        break;
  }
	return aReturn;
}
  
#define kCRK  "Create Registry Key: "
#define kDRK  "Delete Registry Key: "
#define kDRV  "Delete Registry Value: "
#define kSRVS "Store Registry Value String: "
#define kSRVN "Store Registry Value Number: "
#define kSRV  "Store Registry Value: "
#define kUNK  "Unknown "

char* nsWinRegItem::toString()
{
	nsString*  keyString     = nsnull;
	nsString*  result        = nsnull;
  char*      resultCString = nsnull;

	switch(mCommand)
	{
    case NS_WIN_REG_CREATE:
      keyString = keystr(mRootkey, mSubkey, nsnull);
      result    = new nsString;
      result->AssignWithConversion(kCRK);
      break;

    case NS_WIN_REG_DELETE:
      keyString = keystr(mRootkey, mSubkey, nsnull);
      result    = new nsString;
      result->AssignWithConversion(kDRK);
      break;

    case NS_WIN_REG_DELETE_VAL:
      keyString = keystr(mRootkey, mSubkey, mName);
      result    = new nsString;
      result->AssignWithConversion(kDRV);
     break;

    case NS_WIN_REG_SET_VAL_STRING:
      keyString = keystr(mRootkey, mSubkey, mName);
      result    = new nsString;
      result->AssignWithConversion(kSRVS);
      break;

    case NS_WIN_REG_SET_VAL_NUMBER:
      keyString = keystr(mRootkey, mSubkey, mName);
      result    = new nsString;
      result->AssignWithConversion(kSRVN);
      break;

    case NS_WIN_REG_SET_VAL:
      keyString = keystr(mRootkey, mSubkey, mName);
      result    = new nsString;
      result->AssignWithConversion(kSRV);
      break;

    default:
      keyString = keystr(mRootkey, mSubkey, mName);
      result    = new nsString;
      result->AssignWithConversion(kUNK);
      break;
	}
    
  if (result)
  {
      result->Append(*keyString);
      resultCString = ToNewCString(*result);
  }
  
  if (keyString) delete keyString;
  if (result)    delete result;
  
  return resultCString;
}

PRInt32 nsWinRegItem::Prepare()
{
  PRInt32 aReturn = NS_OK;
  
  if (mReg == nsnull)
      return nsInstall::OUT_OF_MEMORY;

  switch (mCommand)
  {
    case NS_WIN_REG_CREATE:
        mReg->PrepareCreateKey(mRootkey, *mSubkey, &aReturn);
        break;
    
    case NS_WIN_REG_DELETE:
        mReg->PrepareDeleteKey(mRootkey, *mSubkey, &aReturn);
        break;
    
    case NS_WIN_REG_DELETE_VAL:
        mReg->PrepareDeleteValue(mRootkey, *mSubkey, *mName, &aReturn);
        break;
    
    case NS_WIN_REG_SET_VAL_STRING:
        mReg->PrepareSetValueString(mRootkey, *mSubkey, &aReturn);
        break;

    case NS_WIN_REG_SET_VAL_NUMBER:
        mReg->PrepareSetValueNumber(mRootkey, *mSubkey, &aReturn);
        break;
    
    case NS_WIN_REG_SET_VAL:
        mReg->PrepareSetValue(mRootkey, *mSubkey, &aReturn);
        break;

    default:
        break;
  }
	return aReturn;
}

void nsWinRegItem::Abort()
{
}



nsString* nsWinRegItem::keystr(PRInt32 root, nsString* mSubkey, nsString* mName)
{
	nsString  rootstr;
	nsString* finalstr = nsnull;
  char*     istr     = nsnull;

	switch(root)
	{
	  case nsWinReg::NS_HKEY_CLASSES_ROOT:
		  rootstr.AssignLiteral("HKEY_CLASSES_ROOT\\");
		  break;

	  case nsWinReg::NS_HKEY_CURRENT_USER:
		  rootstr.AssignLiteral("HKEY_CURRENT_USER\\");
		  break;

	  case nsWinReg::NS_HKEY_LOCAL_MACHINE:
		  rootstr.AssignLiteral("HKEY_LOCAL_MACHINE\\");
		  break;

	  case nsWinReg::NS_HKEY_USERS:
		  rootstr.AssignLiteral("HKEY_USERS\\");
		  break;

    default:
      istr = itoa(root);
      if (istr)
      {
        rootstr.AssignLiteral("#");
        rootstr.AppendWithConversion(istr);
        rootstr.AppendLiteral("\\");
        
        PR_DELETE(istr);
      }
      break;
	}

  finalstr = new nsString(rootstr);
	if(finalstr != nsnull)
	{
    finalstr->Append(*mSubkey);
    finalstr->AppendLiteral(" [");

    if(mName != nsnull)
      finalstr->Append(*mName);

    finalstr->AppendLiteral("]");
	}

  return finalstr;
}


char* nsWinRegItem::itoa(PRInt32 n)
{
	char* s;
	int i, sign;
	if((sign = n) < 0)
		n = -n;
	i = 0;
	
	s = (char*)PR_CALLOC(sizeof(char));

	do
		{
		s = (char*)PR_REALLOC(s, (i+1)*sizeof(char));
		s[i++] = n%10 + '0';
		s[i] = '\0';
		} while ((n/=10) > 0);
		
	if(sign < 0)
	{
		s = (char*)PR_REALLOC(s, (i+1)*sizeof(char));
		s[i++] = '-';
	}
	s[i]  = '\0';
	reverseString(s);
	return s;
}

void nsWinRegItem::reverseString(char* s)
{
	int c, i, j;
	
	for(i=0, j=strlen(s)-1; i<j; i++, j--)
		{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
		}
}





PRBool
nsWinRegItem:: CanUninstall()
{
    return PR_FALSE;
}





PRBool
nsWinRegItem:: RegisterPackageNode()
{
    return PR_TRUE;
}

