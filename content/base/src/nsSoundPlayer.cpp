






































#include "nsSoundPlayer.h"
#include "nsString.h"
#include "nsIURL.h"
#include "nsServiceManagerUtils.h"
#include "nsGkAtoms.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsCOMPtr.h"

#ifdef DEBUG
#include "nsPrintfCString.h"
#endif

nsSoundPlayer* nsSoundPlayer::sInstance = nsnull;

NS_IMPL_ISUPPORTS2(nsSoundPlayer, nsISoundPlayer, nsIDOMEventListener)


nsSoundPlayer::nsSoundPlayer()
{
}

nsSoundPlayer::~nsSoundPlayer()
{
  if (this == sInstance) {
    sInstance = nsnull;
  }
#ifdef MOZ_MEDIA
  for (PRInt32 i = mAudioElements.Count() - 1; i >= 0; i--) {
    RemoveEventListeners(mAudioElements[i]);
  }
#endif 
}

 nsSoundPlayer*
nsSoundPlayer::GetInstance()
{
  if (!sInstance) {
    sInstance = new nsSoundPlayer();
  }
  NS_IF_ADDREF(sInstance);
  return sInstance;
}

#ifdef MOZ_MEDIA

void
nsSoundPlayer::RemoveEventListeners(nsIDOMHTMLMediaElement *aElement)
{
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(aElement));
  target->RemoveEventListener(NS_LITERAL_STRING("ended"), this, PR_TRUE);
  target->RemoveEventListener(NS_LITERAL_STRING("error"), this, PR_TRUE);
}

nsresult
nsSoundPlayer::CreateAudioElement(nsIDOMHTMLMediaElement **aElement)
{
  nsresult rv;
  nsCOMPtr<nsIDOMHTMLMediaElement> audioElement =
    do_CreateInstance("@mozilla.org/content/element/html;1?name=audio", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(audioElement,
               "do_CreateInterface succeeded, but the result is null");
  audioElement->SetAutoplay(PR_TRUE);
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(audioElement));
  target->AddEventListener(NS_LITERAL_STRING("ended"), this, PR_TRUE);
  target->AddEventListener(NS_LITERAL_STRING("error"), this, PR_TRUE);
  NS_ADDREF(*aElement = audioElement);
  return NS_OK;
}

#endif 

NS_IMETHODIMP
nsSoundPlayer::Play(nsIURL* aURL)
{
#ifdef MOZ_MEDIA
  nsCAutoString spec;
  nsresult rv = aURL->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  if (spec.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLMediaElement> audioElement;
  rv = CreateAudioElement(getter_AddRefs(audioElement));
  NS_ENSURE_SUCCESS(rv, rv);

  mAudioElements.AppendObject(audioElement);

  rv = audioElement->SetAttribute(NS_LITERAL_STRING("src"),
                                  NS_ConvertUTF8toUTF16(spec));
  NS_ENSURE_SUCCESS(rv, rv);
  audioElement->Load();
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}

NS_IMETHODIMP
nsSoundPlayer::Stop()
{
#ifdef MOZ_MEDIA
  for (PRInt32 i = mAudioElements.Count() - 1; i >= 0; i--) {
    nsCOMPtr<nsIDOMHTMLMediaElement> audioElement = mAudioElements[i];
    if (!audioElement) {
      NS_WARNING("mAudioElements has null item");
      continue;
    }
    RemoveEventListeners(audioElement);
    audioElement->Pause();
  }
  mAudioElements.Clear();
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}

NS_IMETHODIMP
nsSoundPlayer::HandleEvent(nsIDOMEvent *aEvent)
{
#ifdef MOZ_MEDIA
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv;
  nsCOMPtr<nsIDOMEventTarget> target;
  rv = aEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(target, NS_OK);

  nsCOMPtr<nsIDOMHTMLMediaElement> audioElement = do_QueryInterface(target);
  NS_ENSURE_TRUE(audioElement, NS_OK);

  RemoveEventListeners(audioElement);

  mAudioElements.RemoveObject(audioElement);

  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}
