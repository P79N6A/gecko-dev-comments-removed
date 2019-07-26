





#include "nsNSSComponent.h"
#include "secerr.h"
#include "sslerr.h"

const char *
nsNSSErrors::getDefaultErrorStringName(PRErrorCode err)
{
  return PR_ErrorToName(err);
}

const char *
nsNSSErrors::getOverrideErrorStringName(PRErrorCode aErrorCode)
{
  const char *id_str = nullptr;

  switch (aErrorCode) {
    case SSL_ERROR_SSL_DISABLED:
      id_str = "PSMERR_SSL_Disabled";
      break;
  
    case SSL_ERROR_SSL2_DISABLED:
      id_str = "PSMERR_SSL2_Disabled";
      break;
  
    case SEC_ERROR_REUSED_ISSUER_AND_SERIAL:
      id_str = "PSMERR_HostReusedIssuerSerial";
      break;
  }

  return id_str;
}

nsresult
nsNSSErrors::getErrorMessageFromCode(PRErrorCode err,
                                     nsINSSComponent *component,
                                     nsString &returnedMessage)
{
  NS_ENSURE_ARG_POINTER(component);
  returnedMessage.Truncate();

  const char *nss_error_id_str = getDefaultErrorStringName(err);
  const char *id_str = getOverrideErrorStringName(err);

  if (id_str || nss_error_id_str)
  {
    nsString defMsg;
    nsresult rv;
    if (id_str)
    {
      rv = component->GetPIPNSSBundleString(id_str, defMsg);
    }
    else
    {
      rv = component->GetNSSBundleString(nss_error_id_str, defMsg);
    }

    if (NS_SUCCEEDED(rv))
    {
      returnedMessage.Append(defMsg);
      returnedMessage.Append('\n');
    }
  }
  
  if (returnedMessage.IsEmpty())
  {
    
    returnedMessage.AppendASCII(PR_ErrorToString(err, PR_LANGUAGE_EN));
    returnedMessage.Append('\n');
  }
  
  if (nss_error_id_str)
  {
    nsresult rv;
    nsCString error_id(nss_error_id_str);
    ToLowerCase(error_id);
    NS_ConvertASCIItoUTF16 idU(error_id);

    const char16_t *params[1];
    params[0] = idU.get();

    nsString formattedString;
    rv = component->PIPBundleFormatStringFromName("certErrorCodePrefix", 
                                                  params, 1, 
                                                  formattedString);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append('\n');
      returnedMessage.Append(formattedString);
      returnedMessage.Append('\n');
    }
    else {
      returnedMessage.Append('(');
      returnedMessage.Append(idU);
      returnedMessage.Append(')');
    }
  }

  return NS_OK;
}
