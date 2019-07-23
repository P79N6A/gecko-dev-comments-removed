






































#include "mozStorageVariant.h"




NS_IMPL_THREADSAFE_ISUPPORTS1(
  mozStorageVariant_base,
  nsIVariant
)




NS_IMETHODIMP
mozStorageVariant_base::GetDataType(PRUint16 *_type)
{
  *_type = nsIDataType::VTYPE_EMPTY;
  return NS_OK;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsInt32(PRInt32 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsInt64(PRInt64 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsDouble(double *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsAUTF8String(nsACString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsAString(nsAString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsArray(PRUint16 *, nsIID *, PRUint32 *, void **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsInt8(PRUint8 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsInt16(PRInt16 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsUint8(PRUint8 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsUint16(PRUint16 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsUint32(PRUint32 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsUint64(PRUint64 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsFloat(float *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsBool(PRBool *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsChar(char *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsWChar(PRUnichar *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsID(nsID *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsDOMString(nsAString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsString(char **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsWString(PRUnichar **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsISupports(nsISupports **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsInterface(nsIID **, void **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsACString(nsACString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsStringWithSize(PRUint32 *, char **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

NS_IMETHODIMP
mozStorageVariant_base::GetAsWStringWithSize(PRUint32 *, PRUnichar **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}
