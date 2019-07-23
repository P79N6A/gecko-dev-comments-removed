






































#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsNSSDialogs.h"
#include "nsPKIParamBlock.h"
#include "nsASN1Tree.h"
#include "nsFormSigningDialog.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsNSSDialogs, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPKIParamBlock, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNSSASN1Tree)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFormSigningDialog)

#define NSS_DIALOGS_DESCRIPTION "PSM Dialog Impl"

static const nsModuleComponentInfo components[] =
{
  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_TOKENPASSWORDSDIALOG_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_BADCERTLISTENER_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_CERTIFICATEDIALOGS_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_CLIENTAUTHDIALOGS_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_CERTPICKDIALOGS_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_TOKENDIALOGS_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_DOMCRYPTODIALOGS_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    NSS_DIALOGS_DESCRIPTION,
    NS_NSSDIALOGS_CID,
    NS_GENERATINGKEYPAIRINFODIALOGS_CONTRACTID,
    nsNSSDialogsConstructor
  },

  {
    "ASN1 Tree",
    NS_NSSASN1OUTINER_CID,
    NS_ASN1TREE_CONTRACTID,
    nsNSSASN1TreeConstructor
  },

  { "PKI Parm Block", 
    NS_PKIPARAMBLOCK_CID,
    NS_PKIPARAMBLOCK_CONTRACTID,
    nsPKIParamBlockConstructor
  },

  { "Form Signing Dialog", 
    NS_FORMSIGNINGDIALOG_CID,
    NS_FORMSIGNINGDIALOG_CONTRACTID,
    nsFormSigningDialogConstructor
  }
};

NS_IMPL_NSGETMODULE(PKI, components)
