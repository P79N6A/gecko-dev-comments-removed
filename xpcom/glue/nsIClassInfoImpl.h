





#ifndef nsIClassInfoImpl_h__
#define nsIClassInfoImpl_h__

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/MacroArgs.h"
#include "mozilla/MacroForEach.h"
#include "nsIClassInfo.h"
#include "nsISupportsImpl.h"

#include <new>





































































class GenericClassInfo : public nsIClassInfo
{
public:
  struct ClassInfoData
  {
    typedef NS_CALLBACK(GetInterfacesProc)(uint32_t* aCountP,
                                           nsIID*** aArray);
    typedef NS_CALLBACK(GetLanguageHelperProc)(uint32_t aLanguage,
                                               nsISupports** aHelper);

    GetInterfacesProc getinterfaces;
    GetLanguageHelperProc getlanguagehelper;
    uint32_t flags;
    nsCID cid;
  };

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICLASSINFO

  explicit GenericClassInfo(const ClassInfoData* aData) : mData(aData) {}

private:
  const ClassInfoData* mData;
};

#define NS_CLASSINFO_NAME(_class) g##_class##_classInfoGlobal
#define NS_CI_INTERFACE_GETTER_NAME(_class) _class##_GetInterfacesHelper
#define NS_DECL_CI_INTERFACE_GETTER(_class)                                   \
  extern NS_IMETHODIMP NS_CI_INTERFACE_GETTER_NAME(_class)                    \
     (uint32_t *, nsIID ***);

#define NS_IMPL_CLASSINFO(_class, _getlanguagehelper, _flags, _cid)     \
  NS_DECL_CI_INTERFACE_GETTER(_class)                                   \
  static const GenericClassInfo::ClassInfoData k##_class##ClassInfoData = { \
    NS_CI_INTERFACE_GETTER_NAME(_class),                                \
    _getlanguagehelper,                                                 \
    _flags | nsIClassInfo::SINGLETON_CLASSINFO,                         \
    _cid,                                                               \
  };                                                                    \
  mozilla::AlignedStorage2<GenericClassInfo> k##_class##ClassInfoDataPlace;   \
  nsIClassInfo* NS_CLASSINFO_NAME(_class) = nullptr;

#define NS_IMPL_QUERY_CLASSINFO(_class)                                       \
  if ( aIID.Equals(NS_GET_IID(nsIClassInfo)) ) {                              \
    if (!NS_CLASSINFO_NAME(_class))                                           \
      NS_CLASSINFO_NAME(_class) = new (k##_class##ClassInfoDataPlace.addr())  \
        GenericClassInfo(&k##_class##ClassInfoData);                          \
    foundInterface = NS_CLASSINFO_NAME(_class);                               \
  } else

#define NS_CLASSINFO_HELPER_BEGIN(_class, _c)                                 \
NS_IMETHODIMP                                                                 \
NS_CI_INTERFACE_GETTER_NAME(_class)(uint32_t *count, nsIID ***array)          \
{                                                                             \
    *count = _c;                                                              \
    *array = (nsIID **)nsMemory::Alloc(sizeof (nsIID *) * _c);                \
    uint32_t i = 0;

#define NS_CLASSINFO_HELPER_ENTRY(_interface)                                 \
    (*array)[i++] = (nsIID*)nsMemory::Clone(&NS_GET_IID(_interface),          \
                                            sizeof(nsIID));

#define NS_CLASSINFO_HELPER_END                                               \
    MOZ_ASSERT(i == *count, "Incorrent number of entries");                   \
    return NS_OK;                                                             \
}

#define NS_IMPL_CI_INTERFACE_GETTER(aClass, ...)                              \
  MOZ_STATIC_ASSERT_VALID_ARG_COUNT(__VA_ARGS__);                             \
  NS_CLASSINFO_HELPER_BEGIN(aClass,                                           \
                            MOZ_PASTE_PREFIX_AND_ARG_COUNT(/* No prefix */,   \
                                                           __VA_ARGS__))      \
    MOZ_FOR_EACH(NS_CLASSINFO_HELPER_ENTRY, (), (__VA_ARGS__))                \
  NS_CLASSINFO_HELPER_END

#define NS_IMPL_QUERY_INTERFACE_CI(aClass, ...)                               \
  MOZ_STATIC_ASSERT_VALID_ARG_COUNT(__VA_ARGS__);                             \
  NS_INTERFACE_MAP_BEGIN(aClass)                                              \
    MOZ_FOR_EACH(NS_INTERFACE_MAP_ENTRY, (), (__VA_ARGS__))                   \
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, MOZ_ARG_1(__VA_ARGS__))     \
    NS_IMPL_QUERY_CLASSINFO(aClass)                                           \
  NS_INTERFACE_MAP_END

#define NS_IMPL_ISUPPORTS_CI(aClass, ...)                                     \
  NS_IMPL_ADDREF(aClass)                                                      \
  NS_IMPL_RELEASE(aClass)                                                     \
  NS_IMPL_QUERY_INTERFACE_CI(aClass, __VA_ARGS__)                             \
  NS_IMPL_CI_INTERFACE_GETTER(aClass, __VA_ARGS__)

#endif 
