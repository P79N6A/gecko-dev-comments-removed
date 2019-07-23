






































#include "nscore.h"
#include "plstr.h"
#include <stdio.h>
#include "nsIURL.h"
#include "nsString.h"
#include "nsIFileURL.h"
#include "nsSound.h"
#include "nsNetUtil.h"

#include "nsDirectoryServiceDefs.h"

#include "nsNativeCharsetUtils.h"

#include <OS.h>
#include <SimpleGameSound.h>
#include <Beep.h>
#include <unistd.h>



NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)


nsSound::nsSound()
 : mSound(0)
{
}

nsSound::~nsSound()
{
	Init();
}

NS_IMETHODIMP nsSound::OnStreamComplete(nsIStreamLoader *aLoader,
                                        nsISupports *context,
                                        nsresult aStatus,
                                        PRUint32 dataLen,
                                        const PRUint8 *data)
{
	
	if (NS_FAILED(aStatus))
	{
#ifdef DEBUG
		printf("Failed load sound file");
#endif
		return aStatus;
	}
	
	
	
	static const char kSoundTmpFileName[] = "mozsound";
	nsCOMPtr<nsIFile> soundTmp;
	nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(soundTmp));
	NS_ENSURE_SUCCESS(rv, rv);
	rv = soundTmp->AppendNative(nsDependentCString(kSoundTmpFileName));
	NS_ENSURE_SUCCESS(rv, rv);
	nsCAutoString soundFilename;
	(void) soundTmp->GetNativePath(soundFilename);
	FILE *fp = fopen(soundFilename.get(), "wb+");
#ifdef DEBUG
	printf("Playing sound file%s\n",soundFilename.get());
#endif
	if (fp) 
	{
		fwrite(data, 1, dataLen, fp);
		fflush(fp);
		fclose(fp);
		Init();
		mSound = new BSimpleGameSound(soundFilename.get());
		if (mSound != NULL && mSound->InitCheck() == B_OK)
		{
			mSound->SetIsLooping(false);
			mSound->StartPlaying();
			rv = NS_OK;
		}
		else
		{
			rv = NS_ERROR_FAILURE;
		}
		unlink(soundFilename.get());
	}
	else
	{
		return Beep();
	}
	return rv;
}

NS_IMETHODIMP nsSound::Init(void)
{
	if (mSound)
	{
		mSound->StopPlaying();
		delete mSound;
		mSound = NULL;
	}
	return NS_OK;
}

NS_METHOD nsSound::Beep()
{
	::beep();
	return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
{
	nsresult rv;
	nsCOMPtr<nsIStreamLoader> loader;
	rv = NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);
	return rv;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
	nsresult rv = NS_ERROR_FAILURE;
	if (NS_IsMozAliasSound(aSoundAlias)) {
		NS_WARNING("nsISound::playSystemSound is called with \"_moz_\" events, they are obsolete, use nsISound::playEventSound instead");
		if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP))
			return Beep();
		return NS_OK;
	}
	nsCOMPtr <nsIURI> fileURI;
	
	nsCOMPtr <nsILocalFile> soundFile;
	rv = NS_NewLocalFile(aSoundAlias, PR_TRUE, 
    					getter_AddRefs(soundFile));
	NS_ENSURE_SUCCESS(rv, rv);
	rv = NS_NewFileURI(getter_AddRefs(fileURI), soundFile);
	NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI, &rv);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = Play(fileURL);
	return rv;
}

NS_IMETHODIMP nsSound::PlayEventSound(PRUint32 aEventId)
{
  return aEventId == EVENT_NEW_MAIL_RECEIVED ? Beep() : NS_OK;
}
