






































#ifndef __NS_SSLCERTERRORDIALOG_H__
#define __NS_SSLCERTERRORDIALOG_H__

#include "nsISSLCertErrorDialog.h"

class nsSSLCertErrorDialog : public nsISSLCertErrorDialog
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSISSLCERTERRORDIALOG

  nsSSLCertErrorDialog() {};
  ~nsSSLCertErrorDialog() {};

};

#define nsSSLCertErrorDialog_CID                    \
{ 0xb13f1121, 0xfa10, 0x4a7d,                       \
{0x82, 0xe5, 0x31, 0x59, 0x9b, 0x89, 0x60, 0xb8} }

#define nsSSLCertErrorDialog_ContractID "@mozilla.org/nsSSLCertErrorDialog;1"

#endif
