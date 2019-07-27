





#ifndef mozilla_storage_StorageBaseStatementInternal_h_
#define mozilla_storage_StorageBaseStatementInternal_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

struct sqlite3;
struct sqlite3_stmt;
class mozIStorageBindingParamsArray;
class mozIStorageBindingParams;
class mozIStorageStatementCallback;
class mozIStoragePendingStatement;

namespace mozilla {
namespace storage {

#define STORAGEBASESTATEMENTINTERNAL_IID \
  {0xd18856c9, 0xbf07, 0x4ae2, {0x94, 0x5b, 0x1a, 0xdd, 0x49, 0x19, 0x55, 0x2a}}

class Connection;
class StatementData;

class AsyncStatementFinalizer;











class StorageBaseStatementInternal : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(STORAGEBASESTATEMENTINTERNAL_IID)

  


  Connection *getOwner()
  {
    return mDBConnection;
  }

  











  virtual int getAsyncStatement(sqlite3_stmt **_stmt) = 0;

  










  virtual nsresult getAsynchronousStatementData(StatementData &_data) = 0;

  









  virtual already_AddRefed<mozIStorageBindingParams> newBindingParams(
    mozIStorageBindingParamsArray *aOwner
  ) = 0;

protected: 
  StorageBaseStatementInternal();

  nsRefPtr<Connection> mDBConnection;
  sqlite3 *mNativeConnection;

  









  sqlite3_stmt *mAsyncStatement;

  













  void asyncFinalize();

  






  void destructorAsyncFinalize();

  NS_IMETHOD NewBindingParamsArray(mozIStorageBindingParamsArray **_array);
  NS_IMETHOD ExecuteAsync(mozIStorageStatementCallback *aCallback,
                          mozIStoragePendingStatement **_stmt);
  NS_IMETHOD EscapeStringForLIKE(const nsAString &aValue,
                                 char16_t aEscapeChar,
                                 nsAString &_escapedString);

  
  friend class AsyncStatementFinalizer;
};

NS_DEFINE_STATIC_IID_ACCESSOR(StorageBaseStatementInternal,
                              STORAGEBASESTATEMENTINTERNAL_IID)

#define NS_DECL_STORAGEBASESTATEMENTINTERNAL \
  virtual Connection *getOwner(); \
  virtual int getAsyncStatement(sqlite3_stmt **_stmt) override; \
  virtual nsresult getAsynchronousStatementData(StatementData &_data) override; \
  virtual already_AddRefed<mozIStorageBindingParams> newBindingParams( \
    mozIStorageBindingParamsArray *aOwner) override;







#define MIX_IMPL(_class, _optionalGuard, _method, _declArgs, _invokeArgs) \
  NS_IMETHODIMP _class::_method _declArgs                                 \
  {                                                                       \
    _optionalGuard                                                        \
    return StorageBaseStatementInternal::_method _invokeArgs;             \
  }







#define MIXIN_IMPL_STORAGEBASESTATEMENTINTERNAL(_class, _optionalGuard) \
  MIX_IMPL(_class, _optionalGuard,                                      \
           NewBindingParamsArray,                                       \
           (mozIStorageBindingParamsArray **_array),                    \
           (_array))                                                    \
  MIX_IMPL(_class, _optionalGuard,                                      \
           ExecuteAsync,                                                \
           (mozIStorageStatementCallback *aCallback,                    \
            mozIStoragePendingStatement **_stmt),                       \
           (aCallback, _stmt))                                          \
  MIX_IMPL(_class, _optionalGuard,                                      \
           EscapeStringForLIKE,                                         \
           (const nsAString &aValue, char16_t aEscapeChar,              \
            nsAString &_escapedString),                                 \
           (aValue, aEscapeChar, _escapedString))




#define BIND_NAME_CONCAT(_nameBit, _concatBit) \
  Bind##_nameBit##_concatBit






















#define BIND_GEN_IMPL(_class, _guard, _name, _declName, _declIndex, _invArgs) \
  NS_IMETHODIMP _class::BIND_NAME_CONCAT(_name, ByName) _declName             \
  {                                                                           \
    _guard                                                                    \
    mozIStorageBindingParams *params = getParams();                           \
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);                           \
    return params->BIND_NAME_CONCAT(_name, ByName) _invArgs;                  \
  }                                                                           \
  NS_IMETHODIMP _class::BIND_NAME_CONCAT(_name, ByIndex) _declIndex           \
  {                                                                           \
    _guard                                                                    \
    mozIStorageBindingParams *params = getParams();                           \
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);                           \
    return params->BIND_NAME_CONCAT(_name, ByIndex) _invArgs;                 \
  }                                                                           \
  NS_IMETHODIMP _class::BIND_NAME_CONCAT(_name, Parameter) _declIndex         \
  {                                                                           \
    _guard                                                                    \
    mozIStorageBindingParams *params = getParams();                           \
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);                           \
    return params->BIND_NAME_CONCAT(_name, ByIndex) _invArgs;                 \
  }







#define BIND_BASE_IMPLS(_class, _optionalGuard)             \
  NS_IMETHODIMP _class::BindByName(const nsACString &aName, \
                                   nsIVariant *aValue)      \
  {                                                         \
    _optionalGuard                                          \
    mozIStorageBindingParams *params = getParams();         \
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);         \
    return params->BindByName(aName, aValue);               \
  }                                                         \
  NS_IMETHODIMP _class::BindByIndex(uint32_t aIndex,        \
                                    nsIVariant *aValue)     \
  {                                                         \
    _optionalGuard                                          \
    mozIStorageBindingParams *params = getParams();         \
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);         \
    return params->BindByIndex(aIndex, aValue);             \
  }





#define BOILERPLATE_BIND_PROXIES(_class, _optionalGuard) \
  BIND_BASE_IMPLS(_class, _optionalGuard)                \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                UTF8String,                              \
                (const nsACString &aWhere,               \
                 const nsACString &aValue),              \
                (uint32_t aWhere,                        \
                 const nsACString &aValue),              \
                (aWhere, aValue))                        \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                String,                                  \
                (const nsACString &aWhere,               \
                 const nsAString  &aValue),              \
                (uint32_t aWhere,                        \
                 const nsAString  &aValue),              \
                (aWhere, aValue))                        \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                Double,                                  \
                (const nsACString &aWhere,               \
                 double aValue),                         \
                (uint32_t aWhere,                        \
                 double aValue),                         \
                (aWhere, aValue))                        \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                Int32,                                   \
                (const nsACString &aWhere,               \
                 int32_t aValue),                        \
                (uint32_t aWhere,                        \
                 int32_t aValue),                        \
                (aWhere, aValue))                        \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                Int64,                                   \
                (const nsACString &aWhere,               \
                 int64_t aValue),                        \
                (uint32_t aWhere,                        \
                 int64_t aValue),                        \
                (aWhere, aValue))                        \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                Null,                                    \
                (const nsACString &aWhere),              \
                (uint32_t aWhere),                       \
                (aWhere))                                \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                Blob,                                    \
                (const nsACString &aWhere,               \
                 const uint8_t *aValue,                  \
                 uint32_t aValueSize),                   \
                (uint32_t aWhere,                        \
                 const uint8_t *aValue,                  \
                 uint32_t aValueSize),                   \
                (aWhere, aValue, aValueSize))            \
  BIND_GEN_IMPL(_class, _optionalGuard,                  \
                AdoptedBlob,                             \
                (const nsACString &aWhere,               \
                 uint8_t *aValue,                        \
                 uint32_t aValueSize),                   \
                (uint32_t aWhere,                        \
                 uint8_t *aValue,                        \
                 uint32_t aValueSize),                   \
                (aWhere, aValue, aValueSize))

  

} 
} 

#endif 
