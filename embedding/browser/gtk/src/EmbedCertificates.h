










































#ifndef __EmbedCertificates_h
#define __EmbedCertificates_h
#include "nsITokenPasswordDialogs.h"
#include "nsIBadCertListener.h"
#ifdef BAD_CERT_LISTENER2
#include "nsIBadCertListener2.h"
#endif
#include "nsICertificateDialogs.h"
#include "nsIClientAuthDialogs.h"
#include "nsICertPickDialogs.h"
#include "nsITokenDialogs.h"
#include "nsIDOMCryptoDialogs.h"
#include "nsIGenKeypairInfoDlg.h"
#include "nsCOMPtr.h"
#include "nsIStringBundle.h"
#define EMBED_CERTIFICATES_CID \
  { 0x518e071f, 0x1dd2, 0x11b2, \
  { 0x93, 0x7e, 0xc4, 0x5f, 0x14, 0xde, 0xf7, 0x78 }}
#define EMBED_CERTIFICATES_DESCRIPTION "Certificates Listener Impl"
class EmbedPrivate;
class EmbedCertificates
: public nsITokenPasswordDialogs,
  public nsIBadCertListener,
#ifdef BAD_CERT_LISTENER2
  public nsIBadCertListener2,
#endif
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
#ifdef BAD_CERT_LISTENER2
    NS_DECL_NSIBADCERTLISTENER2
#endif
    NS_DECL_NSICERTIFICATEDIALOGS
    NS_DECL_NSICLIENTAUTHDIALOGS
    NS_DECL_NSICERTPICKDIALOGS
    NS_DECL_NSITOKENDIALOGS
    NS_DECL_NSIDOMCRYPTODIALOGS
    NS_DECL_NSIGENERATINGKEYPAIRINFODIALOGS
    EmbedCertificates();
    virtual ~EmbedCertificates();
    nsresult Init(void);
  protected:
  nsCOMPtr<nsIStringBundle> mPIPStringBundle;
};
#endif
