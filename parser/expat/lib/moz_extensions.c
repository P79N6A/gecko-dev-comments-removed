






































#ifdef IS_LITTLE_ENDIAN

#define PREFIX(ident) little2_ ## ident
#define BYTE_TYPE(p) LITTLE2_BYTE_TYPE(XmlGetUtf16InternalEncodingNS(), p)
#define IS_NAME_CHAR_MINBPC(p) LITTLE2_IS_NAME_CHAR_MINBPC(0, p)
#define IS_NMSTRT_CHAR_MINBPC(p) LITTLE2_IS_NMSTRT_CHAR_MINBPC(0, p)

#else

#define PREFIX(ident) big2_ ## ident
#define BYTE_TYPE(p) BIG2_BYTE_TYPE(XmlGetUtf16InternalEncodingNS(), p)
#define IS_NAME_CHAR_MINBPC(p) BIG2_IS_NAME_CHAR_MINBPC(0, p)
#define IS_NMSTRT_CHAR_MINBPC(p) BIG2_IS_NMSTRT_CHAR_MINBPC(0, p)

#endif

#define MOZ_EXPAT_VALID_QNAME       (0)
#define MOZ_EXPAT_EMPTY_QNAME       (1 << 0)
#define MOZ_EXPAT_INVALID_CHARACTER (1 << 1)
#define MOZ_EXPAT_MALFORMED         (1 << 2)

int MOZ_XMLCheckQName(const char* ptr, const char* end, int ns_aware,
                      const char** colon)
{
  int result = MOZ_EXPAT_VALID_QNAME;
  int nmstrt = 1;
  *colon = 0;
  if (ptr == end) {
    return MOZ_EXPAT_EMPTY_QNAME;
  }
  do {
    switch (BYTE_TYPE(ptr)) {
    case BT_COLON:
      if (ns_aware) {
        if (*colon != 0 || nmstrt || ptr + 2 == end) {
          

          result |= MOZ_EXPAT_MALFORMED;
        }
        *colon = ptr;
        nmstrt = 1;
      }
      else if (nmstrt) {
        
        result |= MOZ_EXPAT_MALFORMED;
        nmstrt = 0;
      }
      break;
    case BT_NONASCII:
      if (nmstrt) {
        if (!IS_NMSTRT_CHAR_MINBPC(ptr)) {
          

          result |= IS_NAME_CHAR_MINBPC(ptr) ? MOZ_EXPAT_MALFORMED :
                                               MOZ_EXPAT_INVALID_CHARACTER;
        }
      }
      else if (!IS_NAME_CHAR_MINBPC(ptr)) {
        result |= MOZ_EXPAT_INVALID_CHARACTER;
      }
      nmstrt = 0;
      break;
    case BT_NMSTRT:
    case BT_HEX:
      nmstrt = 0;
      break;
    case BT_DIGIT:
    case BT_NAME:
    case BT_MINUS:
      if (nmstrt) {
        result |= MOZ_EXPAT_MALFORMED;
        nmstrt = 0;
      }
      break;
    default:
      result |= MOZ_EXPAT_INVALID_CHARACTER;
      nmstrt = 0;
    }
    ptr += 2;
  } while (ptr != end);
  return result;
}

int MOZ_XMLIsLetter(const char* ptr)
{
  switch (BYTE_TYPE(ptr)) {
  case BT_NONASCII:
    if (!IS_NMSTRT_CHAR_MINBPC(ptr)) {
      return 0;
    }
  case BT_NMSTRT:
  case BT_HEX:
    return 1;
  default:
    return 0;
  }
}

int MOZ_XMLIsNCNameChar(const char* ptr)
{
  switch (BYTE_TYPE(ptr)) {
  case BT_NONASCII:
    if (!IS_NAME_CHAR_MINBPC(ptr)) {
      return 0;
    }
  case BT_NMSTRT:
  case BT_HEX:
  case BT_DIGIT:
  case BT_NAME:
  case BT_MINUS:
    return 1;
  default:
    return 0;
  }
}

int MOZ_XMLTranslateEntity(const char* ptr, const char* end, const char** next,
                           XML_Char* result)
{
  const ENCODING* enc = XmlGetUtf16InternalEncodingNS();
  int tok = PREFIX(scanRef)(enc, ptr, end, next);
  if (tok <= XML_TOK_INVALID) {
    return 0;
  }

  if (tok == XML_TOK_CHAR_REF) {
    int n = XmlCharRefNumber(enc, ptr);

    
    if (n <= 0) {
      return 0;
    }

    return XmlUtf16Encode(n, (unsigned short*)result);
  }

  if (tok == XML_TOK_ENTITY_REF) {
    

    XML_Char ch =
      (XML_Char)XmlPredefinedEntityName(enc, ptr, *next - enc->minBytesPerChar);
    if (!ch) {
      return 0;
    }

    *result = ch;
    return 1;
  }

  return 0;
}

#undef PREFIX
#undef BYTE_TYPE
#undef IS_NAME_CHAR_MINBPC
#undef IS_NMSTRT_CHAR_MINBPC
