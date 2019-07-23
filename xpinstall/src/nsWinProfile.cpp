




































#include "nsWinReg.h"
#include "nsWinProfile.h"
#include "nsWinProfileItem.h"
#include "nspr.h"
#include <windows.h>
#include "nsNativeCharsetUtils.h"



nsWinProfile::nsWinProfile( nsInstall* suObj, const nsString& folder, const nsString& file )
  : mFilename(folder)
{
  MOZ_COUNT_CTOR(nsWinProfile);

  if(mFilename.Last() != '\\')
  {
      mFilename.AppendLiteral("\\");
  }
  mFilename.Append(file);

	mInstallObject = suObj;
}

nsWinProfile::~nsWinProfile()
{
  MOZ_COUNT_DTOR(nsWinProfile);
}

PRInt32
nsWinProfile::GetString(nsString section, nsString key, nsString* aReturn)
{
  return NativeGetString(section, key, aReturn);
}

PRInt32
nsWinProfile::WriteString(nsString section, nsString key, nsString value, PRInt32* aReturn)
{
  *aReturn = NS_OK;
  
  nsWinProfileItem* wi = new nsWinProfileItem(this, section, key, value, aReturn);

  if(wi == nsnull)
  {
    *aReturn = nsInstall::OUT_OF_MEMORY;
    return NS_OK;
  }

  if(*aReturn != nsInstall::SUCCESS)
  {
    if(wi)
    {
      delete wi;
      return NS_OK;
    }
  }

  if (mInstallObject)
    mInstallObject->ScheduleForInstall(wi);
  
  return NS_OK;
}

nsString& nsWinProfile::GetFilename()
{
	return mFilename;
}

nsInstall* nsWinProfile::InstallObject()
{
	return mInstallObject;
}

PRInt32
nsWinProfile::FinalWriteString( nsString section, nsString key, nsString value )
{
	
	return NativeWriteString(section, key, value);
}



#define STRBUFLEN 255
  
PRInt32
nsWinProfile::NativeGetString(nsString aSection, nsString aKey, nsString* aReturn )
{
  int       numChars = 0;
  char      valbuf[STRBUFLEN];

  
  if(aSection.First() != '\0' && aKey.First() != '\0' && mFilename.First() != '\0')
  {
    nsCAutoString section;
    nsCAutoString key;
    nsCAutoString filename;

    if(NS_FAILED(NS_CopyUnicodeToNative(aSection, section)) ||
       NS_FAILED(NS_CopyUnicodeToNative(aKey, key)) ||
       NS_FAILED(NS_CopyUnicodeToNative(mFilename, filename)))
      return 0;

    valbuf[0] = 0;
    numChars = GetPrivateProfileString( section.get(), key.get(), "", valbuf,
                                        STRBUFLEN, filename.get());

    nsCAutoString cStrValue(valbuf);
    nsAutoString value;
    if(NS_SUCCEEDED(NS_CopyNativeToUnicode(cStrValue, value)))
      aReturn->Assign(value);
  }

  return numChars;
}

PRInt32
nsWinProfile::NativeWriteString( nsString aSection, nsString aKey, nsString aValue )
{
  int   success = 0;

	
  if(aSection.First() != '\0' && aKey.First() != '\0' && mFilename.First() != '\0')
  {
    nsCAutoString section;
    nsCAutoString key;
    nsCAutoString value;
    nsCAutoString filename;

    if(NS_FAILED(NS_CopyUnicodeToNative(aSection, section)) ||
       NS_FAILED(NS_CopyUnicodeToNative(aKey, key)) ||
       NS_FAILED(NS_CopyUnicodeToNative(aValue, value)) ||
       NS_FAILED(NS_CopyUnicodeToNative(mFilename, filename)))
      return 0;

    success = WritePrivateProfileString( section.get(), key.get(), value.get(), filename.get() );
  }

  return success;
}

