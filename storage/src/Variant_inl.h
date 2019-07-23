










































#ifndef mozilla_storage_Variant_h__
#error "Do not include this file directly!"
#endif

namespace mozilla {
namespace storage {




inline NS_IMPL_THREADSAFE_ADDREF(Variant_base)
inline NS_IMPL_THREADSAFE_RELEASE(Variant_base)
inline NS_IMPL_THREADSAFE_QUERY_INTERFACE1(
  Variant_base,
  nsIVariant
)




inline
NS_IMETHODIMP
Variant_base::GetDataType(PRUint16 *_type)
{
  *_type = nsIDataType::VTYPE_EMPTY;
  return NS_OK;
}

inline
NS_IMETHODIMP
Variant_base::GetAsInt32(PRInt32 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsInt64(PRInt64 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsDouble(double *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsAUTF8String(nsACString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsAString(nsAString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsArray(PRUint16 *,
                         nsIID *,
                         PRUint32 *,
                         void **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsInt8(PRUint8 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsInt16(PRInt16 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsUint8(PRUint8 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsUint16(PRUint16 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsUint32(PRUint32 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsUint64(PRUint64 *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsFloat(float *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsBool(PRBool *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsChar(char *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsWChar(PRUnichar *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsID(nsID *)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsDOMString(nsAString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsString(char **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsWString(PRUnichar **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsISupports(nsISupports **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsInterface(nsIID **,
                             void **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsACString(nsACString &)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsStringWithSize(PRUint32 *,
                                  char **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

inline
NS_IMETHODIMP
Variant_base::GetAsWStringWithSize(PRUint32 *,
                                   PRUnichar **)
{
  return NS_ERROR_CANNOT_CONVERT_DATA;
}

} 
} 
