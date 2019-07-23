






































#ifndef __NS_NSSDIALOGS_H__
#define __NS_NSSDIALOGS_H__

#include "nsITokenPasswordDialogs.h"
#include "nsIBadCertListener.h"
#include "nsICertificateDialogs.h"
#include "nsIClientAuthDialogs.h"
#include "nsICertPickDialogs.h"
#include "nsITokenDialogs.h"
#include "nsIDOMCryptoDialogs.h"
#include "nsIGenKeypairInfoDlg.h"

#include "nsCOMPtr.h"
#include "nsIStringBundle.h"

#define NS_NSSDIALOGS_CID \
  { 0x518e071f, 0x1dd2, 0x11b2, \
    { 0x93, 0x7e, 0xc4, 0x5f, 0x14, 0xde, 0xf7, 0x78 }}

class nsNSSDialogs
: public nsITokenPasswordDialogs,
  public nsIBadCertListener,
  public nsICertificateDialogs,
  public nsIClientAuthDialogs,
  public nsICertPickDialogs,
  public nsITokenDialogs,
  public nsIDOMCryptoDialogs,
  public nsIGeneratingKeypairInfoDialogs
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITOKENPASSWORDDIALOGS
  NS_DECL_NSIBADCERTLISTENER
  NS_DECL_NSICERTIFICATEDIALOGS
  NS_DECL_NSICLIENTAUTHDIALOGS
  NS_DECL_NSICERTPICKDIALOGS
  NS_DECL_NSITOKENDIALOGS
  NS_DECL_NSIDOMCRYPTODIALOGS
  NS_DECL_NSIGENERATINGKEYPAIRINFODIALOGS
  nsNSSDialogs();
  virtual ~nsNSSDialogs();

  nsresult Init();

protected:
  nsCOMPtr<nsIStringBundle> mPIPStringBundle;
};

#endif
