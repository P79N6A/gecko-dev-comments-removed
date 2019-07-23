



































#ifndef __NS_NSFORMSIGNINGDIALOG_H__
#define __NS_NSFORMSIGNINGDIALOG_H__

#include "nsIFormSigningDialog.h"

#define NS_FORMSIGNINGDIALOG_CID \
  { 0xa4bd2161, 0x7892, 0x4389, \
    { 0x8d, 0x5a, 0x31, 0x11, 0xa6, 0xd1, 0x7e, 0xc7 }}

class nsFormSigningDialog : public nsIFormSigningDialog
{
public:
  nsFormSigningDialog();
  ~nsFormSigningDialog();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMSIGNINGDIALOG
};

#endif
