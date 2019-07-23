





































#ifndef _NSPKIPARAMBLOCK_
#define _NSPKIPARAMBLOCK_
#include "nsCOMPtr.h"
#include "nsIPKIParamBlock.h"
#include "nsIDialogParamBlock.h"
#include "nsISupportsArray.h"

#define NS_PKIPARAMBLOCK_CID \
  { 0x0bec75a8, 0x1dd2, 0x11b2, \
    { 0x86, 0x3a, 0xf6, 0x9f, 0x77, 0xc3, 0x13, 0x71 }}

#define NS_PKIPARAMBLOCK_CONTRACTID "@mozilla.org/security/pkiparamblock;1"

class nsPKIParamBlock : public nsIPKIParamBlock,
                        public nsIDialogParamBlock
{
public:
 
  nsPKIParamBlock();
  virtual ~nsPKIParamBlock();
  nsresult Init();

  NS_DECL_NSIPKIPARAMBLOCK
  NS_DECL_NSIDIALOGPARAMBLOCK
  NS_DECL_ISUPPORTS
private:
  nsCOMPtr<nsIDialogParamBlock> mDialogParamBlock;
  nsCOMPtr<nsISupportsArray>    mSupports;
};

#endif 
