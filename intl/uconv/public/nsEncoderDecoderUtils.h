




#ifndef nsEncoderDecoderUtils_h__
#define nsEncoderDecoderUtils_h__

#define NS_UNICODEDECODER_NAME "Charset Decoders"
#define NS_UNICODEENCODER_NAME "Charset Encoders"

#define NS_DATA_BUNDLE_CATEGORY     "uconv-charset-data"
#define NS_TITLE_BUNDLE_CATEGORY    "uconv-charset-titles"

#define NS_CONVERTER_REGISTRY_START \
  static const mozilla::Module::CategoryEntry kUConvCategories[] = {

#define NS_CONVERTER_REGISTRY_END \
  { NULL } \
  };

#define NS_UCONV_REG_UNREG_DECODER(_Charset, _CID)          \
  { NS_UNICODEDECODER_NAME, _Charset, "" },
  
#define NS_UCONV_REG_UNREG_ENCODER(_Charset, _CID)          \
  { NS_UNICODEENCODER_NAME, _Charset, "" },

#define NS_UCONV_REG_UNREG(_Charset, _DecoderCID, _EncoderCID) \
  NS_UCONV_REG_UNREG_DECODER(_Charset, *) \
  NS_UCONV_REG_UNREG_ENCODER(_Charset, *)

#endif
