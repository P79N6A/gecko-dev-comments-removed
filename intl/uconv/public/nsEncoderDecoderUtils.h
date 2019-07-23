





































#ifndef nsEncoderDecoderUtils_h__
#define nsEncoderDecoderUtils_h__

#define NS_ERROR_UCONV_NOCONV \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_UCONV, 0x01)

#define NS_SUCCESS_USING_FALLBACK_LOCALE \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 0x02)

#define NS_UNICODEDECODER_NAME "Charset Decoders"
#define NS_UNICODEENCODER_NAME "Charset Encoders"

#define NS_DATA_BUNDLE_CATEGORY     "uconv-charset-data"
#define NS_TITLE_BUNDLE_CATEGORY    "uconv-charset-titles"

struct nsConverterRegistryInfo {
  PRBool isEncoder;             
  const char *charset;
  nsCID cid;
};

#define NS_CONVERTER_REGISTRY_START \
  static const nsConverterRegistryInfo gConverterRegistryInfo[] = {

#define NS_CONVERTER_REGISTRY_END \
  };


#define NS_IMPL_NSUCONVERTERREGSELF                                     \
static NS_IMETHODIMP                                                    \
nsUConverterRegSelf(nsIComponentManager *aCompMgr,                      \
                    nsIFile *aPath,                                     \
                    const char* registryLocation,                       \
                    const char* componentType,                          \
                    const nsModuleComponentInfo *info)                  \
{                                                                       \
  nsresult rv;                                                          \
  nsCOMPtr<nsICategoryManager> catman =                                 \
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);                  \
  if (NS_FAILED(rv)) return rv;                                         \
                                                                        \
  nsXPIDLCString previous;                                              \
  PRUint32 i;                                                           \
  for (i=0; i<sizeof(gConverterRegistryInfo)/sizeof(gConverterRegistryInfo[0]); i++) { \
    const nsConverterRegistryInfo* entry = &gConverterRegistryInfo[i];         \
    const char *category;                                               \
    const char *key;                                                    \
                                                                        \
    if (entry->isEncoder) {                                             \
      category = NS_UNICODEENCODER_NAME;                                \
    } else {                                                            \
      category = NS_UNICODEDECODER_NAME;                                \
    }                                                                   \
    key = entry->charset;                                               \
                                                                        \
    rv = catman->AddCategoryEntry(category, key, "",                    \
                                  PR_TRUE,                              \
                                  PR_TRUE,                              \
                                  getter_Copies(previous));             \
  }                                                                     \
  return rv;                                                            \
} \
static NS_IMETHODIMP \
nsUConverterUnregSelf(nsIComponentManager *aCompMgr,                        \
                      nsIFile *aPath,                                       \
                      const char*,                                          \
                      const nsModuleComponentInfo *info)                    \
{ \
  nsresult rv;                                                          \
  nsCOMPtr<nsICategoryManager> catman =                                 \
  do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);                    \
  if (NS_FAILED(rv)) return rv;                                         \
                                                                        \
  nsXPIDLCString previous;                                              \
  PRUint32 i;                                                           \
  for (i=0; i<sizeof(gConverterRegistryInfo)/sizeof(gConverterRegistryInfo[0]); i++) { \
    const nsConverterRegistryInfo* entry = &gConverterRegistryInfo[i];         \
    const char *category;                                               \
    const char *key;                                                    \
                                                                        \
    if (entry->isEncoder) {                                             \
      category = NS_UNICODEDECODER_NAME;                                \
    } else {                                                            \
      category = NS_UNICODEENCODER_NAME;                                \
    }                                                                   \
    key = entry->charset;                                               \
                                                                        \
    char * value = entry->cid.ToString();                               \
                                                                        \
    rv = catman->DeleteCategoryEntry(category, key, PR_TRUE);           \
    CRTFREEIF(value);                                                   \
  }                                                                     \
  return rv;                                                            \
}


#define NS_UCONV_REG_UNREG_DECODER(_Charset, _CID)          \
  {                                                         \
    PR_FALSE,                                                \
    _Charset,                                               \
    _CID,                                                   \
  },
  
#define NS_UCONV_REG_UNREG_ENCODER(_Charset, _CID)          \
  {                                                         \
    PR_TRUE,                                               \
    _Charset,                                               \
    _CID,                                                   \
  }, 

  
  
  
#define NS_UCONV_REG_UNREG(_Charset, _DecoderCID, _EncoderCID) \
  {                                                         \
    PR_FALSE,                                               \
    _Charset,                                               \
    _DecoderCID,                                            \
  },                                                        \
  {                                                         \
    PR_TRUE,                                                \
    _Charset,                                               \
    _EncoderCID,                                            \
  },
  
#endif
