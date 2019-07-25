





































#include "nsGConfService.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsIMutableArray.h"

#include <gconf/gconf-client.h>

nsGConfService::~nsGConfService()
{
  if (mClient)
    g_object_unref(mClient);
}

nsresult
nsGConfService::Init()
{
  mClient = gconf_client_get_default();
  return mClient ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMPL_ISUPPORTS1(nsGConfService, nsIGConfService)

NS_IMETHODIMP
nsGConfService::GetBool(const nsACString &aKey, PRBool *aResult)
{
  GError* error = nsnull;
  *aResult = gconf_client_get_bool(mClient, PromiseFlatCString(aKey).get(),
                                   &error);

  if (error) {
    g_error_free(error);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::GetString(const nsACString &aKey, nsACString &aResult)
{
  GError* error = nsnull;
  gchar *result = gconf_client_get_string(mClient,
                                          PromiseFlatCString(aKey).get(),
                                          &error);

  if (error) {
    g_error_free(error);
    return NS_ERROR_FAILURE;
  }

  
  

  aResult.Assign(result);
  g_free(result);

  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::GetInt(const nsACString &aKey, PRInt32* aResult)
{
  GError* error = nsnull;
  *aResult = gconf_client_get_int(mClient, PromiseFlatCString(aKey).get(),
                                  &error);

  if (error) {
    g_error_free(error);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::GetFloat(const nsACString &aKey, float* aResult)
{
  GError* error = nsnull;
  *aResult = gconf_client_get_float(mClient, PromiseFlatCString(aKey).get(),
                                    &error);

  if (error) {
    g_error_free(error);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::GetStringList(const nsACString &aKey, nsIArray** aResult)
{
  nsCOMPtr<nsIMutableArray> items(do_CreateInstance(NS_ARRAY_CONTRACTID));
  if (!items)
    return NS_ERROR_OUT_OF_MEMORY;
    
  GError* error = nsnull;
  GSList* list = gconf_client_get_list(mClient, PromiseFlatCString(aKey).get(),
                                       GCONF_VALUE_STRING, &error);
  if (error) {
    g_error_free(error);
    return NS_ERROR_FAILURE;
  }

  for (GSList* l = list; l; l = l->next) {
    nsCOMPtr<nsISupportsString> obj(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
    if (!obj) {
      g_slist_free(list);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    obj->SetData(NS_ConvertUTF8toUTF16((const char*)l->data));
    items->AppendElement(obj, PR_FALSE);
    g_free(l->data);
  }
  
  g_slist_free(list);
  NS_ADDREF(*aResult = items);
  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::SetBool(const nsACString &aKey, PRBool aValue)
{
  PRBool res = gconf_client_set_bool(mClient, PromiseFlatCString(aKey).get(),
                                     aValue, nsnull);

  return res ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGConfService::SetString(const nsACString &aKey, const nsACString &aValue)
{
  PRBool res = gconf_client_set_string(mClient, PromiseFlatCString(aKey).get(),
                                       PromiseFlatCString(aValue).get(),
                                       nsnull);

  return res ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGConfService::SetInt(const nsACString &aKey, PRInt32 aValue)
{
  PRBool res = gconf_client_set_int(mClient, PromiseFlatCString(aKey).get(),
                                    aValue, nsnull);

  return res ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGConfService::SetFloat(const nsACString &aKey, float aValue)
{
  PRBool res = gconf_client_set_float(mClient, PromiseFlatCString(aKey).get(),
                                      aValue, nsnull);

  return res ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGConfService::GetAppForProtocol(const nsACString &aScheme, PRBool *aEnabled,
                                  nsACString &aHandler)
{
  nsCAutoString key("/desktop/gnome/url-handlers/");
  key.Append(aScheme);
  key.Append("/command");

  GError *err = nsnull;
  gchar *command = gconf_client_get_string(mClient, key.get(), &err);
  if (!err && command) {
    key.Replace(key.Length() - 7, 7, NS_LITERAL_CSTRING("enabled"));
    *aEnabled = gconf_client_get_bool(mClient, key.get(), &err);
  } else {
    *aEnabled = PR_FALSE;
  }

  aHandler.Assign(command);
  g_free(command);

  if (err) {
    g_error_free(err);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::HandlerRequiresTerminal(const nsACString &aScheme,
                                        PRBool *aResult)
{
  nsCAutoString key("/desktop/gnome/url-handlers/");
  key.Append(aScheme);
  key.Append("/requires_terminal");

  GError *err = nsnull;
  *aResult = gconf_client_get_bool(mClient, key.get(), &err);
  if (err) {
    g_error_free(err);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGConfService::SetAppForProtocol(const nsACString &aScheme,
                                  const nsACString &aCommand)
{
  nsCAutoString key("/desktop/gnome/url-handlers/");
  key.Append(aScheme);
  key.Append("/command");

  PRBool res = gconf_client_set_string(mClient, key.get(),
                                       PromiseFlatCString(aCommand).get(),
                                       nsnull);
  if (res) {
    key.Replace(key.Length() - 7, 7, NS_LITERAL_CSTRING("enabled"));
    res = gconf_client_set_bool(mClient, key.get(), PR_TRUE, nsnull);
    if (res) {
      key.Replace(key.Length() - 7, 7, NS_LITERAL_CSTRING("needs_terminal"));
      res = gconf_client_set_bool(mClient, key.get(), PR_FALSE, nsnull);
      if (res) {
        key.Replace(key.Length() - 14, 14, NS_LITERAL_CSTRING("command-id"));
        res = gconf_client_unset(mClient, key.get(), nsnull);
      }
    }
  }

  return res ? NS_OK : NS_ERROR_FAILURE;
}
