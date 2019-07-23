



#ifndef Expat_INCLUDED
#define Expat_INCLUDED 1

#ifdef __VMS


#define XML_SetProcessingInstructionHandler XML_SetProcessingInstrHandler
#define XML_SetUnparsedEntityDeclHandler    XML_SetUnparsedEntDeclHandler
#define XML_SetStartNamespaceDeclHandler    XML_SetStartNamespcDeclHandler
#define XML_SetExternalEntityRefHandlerArg  XML_SetExternalEntRefHandlerArg
#endif

#include <stdlib.h>
#include "expat_external.h"

#ifdef __cplusplus
extern "C" {
#endif

struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;


typedef unsigned char XML_Bool;
#define XML_TRUE   ((XML_Bool) 1)
#define XML_FALSE  ((XML_Bool) 0)














enum XML_Status {
  XML_STATUS_ERROR = 0,
#define XML_STATUS_ERROR XML_STATUS_ERROR
  XML_STATUS_OK = 1,
#define XML_STATUS_OK XML_STATUS_OK
  XML_STATUS_SUSPENDED = 2
#define XML_STATUS_SUSPENDED XML_STATUS_SUSPENDED
};

enum XML_Error {
  XML_ERROR_NONE,
  XML_ERROR_NO_MEMORY,
  XML_ERROR_SYNTAX,
  XML_ERROR_NO_ELEMENTS,
  XML_ERROR_INVALID_TOKEN,
  XML_ERROR_UNCLOSED_TOKEN,
  XML_ERROR_PARTIAL_CHAR,
  XML_ERROR_TAG_MISMATCH,
  XML_ERROR_DUPLICATE_ATTRIBUTE,
  XML_ERROR_JUNK_AFTER_DOC_ELEMENT,
  XML_ERROR_PARAM_ENTITY_REF,
  XML_ERROR_UNDEFINED_ENTITY,
  XML_ERROR_RECURSIVE_ENTITY_REF,
  XML_ERROR_ASYNC_ENTITY,
  XML_ERROR_BAD_CHAR_REF,
  XML_ERROR_BINARY_ENTITY_REF,
  XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF,
  XML_ERROR_MISPLACED_XML_PI,
  XML_ERROR_UNKNOWN_ENCODING,
  XML_ERROR_INCORRECT_ENCODING,
  XML_ERROR_UNCLOSED_CDATA_SECTION,
  XML_ERROR_EXTERNAL_ENTITY_HANDLING,
  XML_ERROR_NOT_STANDALONE,
  XML_ERROR_UNEXPECTED_STATE,
  XML_ERROR_ENTITY_DECLARED_IN_PE,
  XML_ERROR_FEATURE_REQUIRES_XML_DTD,
  XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING,
  
  XML_ERROR_UNBOUND_PREFIX,
  
  XML_ERROR_UNDECLARING_PREFIX,
  XML_ERROR_INCOMPLETE_PE,
  XML_ERROR_XML_DECL,
  XML_ERROR_TEXT_DECL,
  XML_ERROR_PUBLICID,
  XML_ERROR_SUSPENDED,
  XML_ERROR_NOT_SUSPENDED,
  XML_ERROR_ABORTED,
  XML_ERROR_FINISHED,
  XML_ERROR_SUSPEND_PE,
  
  XML_ERROR_RESERVED_PREFIX_XML,
  XML_ERROR_RESERVED_PREFIX_XMLNS,
  XML_ERROR_RESERVED_NAMESPACE_URI
};

enum XML_Content_Type {
  XML_CTYPE_EMPTY = 1,
  XML_CTYPE_ANY,
  XML_CTYPE_MIXED,
  XML_CTYPE_NAME,
  XML_CTYPE_CHOICE,
  XML_CTYPE_SEQ
};

enum XML_Content_Quant {
  XML_CQUANT_NONE,
  XML_CQUANT_OPT,
  XML_CQUANT_REP,
  XML_CQUANT_PLUS
};



















typedef struct XML_cp XML_Content;

struct XML_cp {
  enum XML_Content_Type         type;
  enum XML_Content_Quant        quant;
  XML_Char *                    name;
  unsigned int                  numchildren;
  XML_Content *                 children;
};






typedef void (XMLCALL *XML_ElementDeclHandler) (void *userData,
                                                const XML_Char *name,
                                                XML_Content *model);

XMLPARSEAPI(void)
XML_SetElementDeclHandler(XML_Parser parser,
                          XML_ElementDeclHandler eldecl);









typedef void (XMLCALL *XML_AttlistDeclHandler) (
                                    void            *userData,
                                    const XML_Char  *elname,
                                    const XML_Char  *attname,
                                    const XML_Char  *att_type,
                                    const XML_Char  *dflt,
                                    int              isrequired);

XMLPARSEAPI(void)
XML_SetAttlistDeclHandler(XML_Parser parser,
                          XML_AttlistDeclHandler attdecl);









typedef void (XMLCALL *XML_XmlDeclHandler) (void           *userData,
                                            const XML_Char *version,
                                            const XML_Char *encoding,
                                            int             standalone);

XMLPARSEAPI(void)
XML_SetXmlDeclHandler(XML_Parser parser,
                      XML_XmlDeclHandler xmldecl);


typedef struct {
  void *(*malloc_fcn)(size_t size);
  void *(*realloc_fcn)(void *ptr, size_t size);
  void (*free_fcn)(void *ptr);
} XML_Memory_Handling_Suite;




XMLPARSEAPI(XML_Parser)
XML_ParserCreate(const XML_Char *encoding);












XMLPARSEAPI(XML_Parser)
XML_ParserCreateNS(const XML_Char *encoding, XML_Char namespaceSeparator);











XMLPARSEAPI(XML_Parser)
XML_ParserCreate_MM(const XML_Char *encoding,
                    const XML_Memory_Handling_Suite *memsuite,
                    const XML_Char *namespaceSeparator);










XMLPARSEAPI(XML_Bool)
XML_ParserReset(XML_Parser parser, const XML_Char *encoding);




typedef void (XMLCALL *XML_StartElementHandler) (void *userData,
                                                 const XML_Char *name,
                                                 const XML_Char **atts);

typedef void (XMLCALL *XML_EndElementHandler) (void *userData,
                                               const XML_Char *name);



typedef void (XMLCALL *XML_CharacterDataHandler) (void *userData,
                                                  const XML_Char *s,
                                                  int len);


typedef void (XMLCALL *XML_ProcessingInstructionHandler) (
                                                void *userData,
                                                const XML_Char *target,
                                                const XML_Char *data);


typedef void (XMLCALL *XML_CommentHandler) (void *userData,
                                            const XML_Char *data);

typedef void (XMLCALL *XML_StartCdataSectionHandler) (void *userData);
typedef void (XMLCALL *XML_EndCdataSectionHandler) (void *userData);














typedef void (XMLCALL *XML_DefaultHandler) (void *userData,
                                            const XML_Char *s,
                                            int len);




typedef void (XMLCALL *XML_StartDoctypeDeclHandler) (
                                            void *userData,
                                            const XML_Char *doctypeName,
                                            const XML_Char *sysid,
                                            const XML_Char *pubid,
                                            int has_internal_subset);





typedef void (XMLCALL *XML_EndDoctypeDeclHandler)(void *userData);



















typedef void (XMLCALL *XML_EntityDeclHandler) (
                              void *userData,
                              const XML_Char *entityName,
                              int is_parameter_entity,
                              const XML_Char *value,
                              int value_length,
                              const XML_Char *base,
                              const XML_Char *systemId,
                              const XML_Char *publicId,
                              const XML_Char *notationName);

XMLPARSEAPI(void)
XML_SetEntityDeclHandler(XML_Parser parser,
                         XML_EntityDeclHandler handler);










typedef void (XMLCALL *XML_UnparsedEntityDeclHandler) (
                                    void *userData,
                                    const XML_Char *entityName,
                                    const XML_Char *base,
                                    const XML_Char *systemId,
                                    const XML_Char *publicId,
                                    const XML_Char *notationName);





typedef void (XMLCALL *XML_NotationDeclHandler) (
                                    void *userData,
                                    const XML_Char *notationName,
                                    const XML_Char *base,
                                    const XML_Char *systemId,
                                    const XML_Char *publicId);







typedef void (XMLCALL *XML_StartNamespaceDeclHandler) (
                                    void *userData,
                                    const XML_Char *prefix,
                                    const XML_Char *uri);

typedef void (XMLCALL *XML_EndNamespaceDeclHandler) (
                                    void *userData,
                                    const XML_Char *prefix);










typedef int (XMLCALL *XML_NotStandaloneHandler) (void *userData);



































typedef int (XMLCALL *XML_ExternalEntityRefHandler) (
                                    XML_Parser parser,
                                    const XML_Char *context,
                                    const XML_Char *base,
                                    const XML_Char *systemId,
                                    const XML_Char *publicId);











typedef void (XMLCALL *XML_SkippedEntityHandler) (
                                    void *userData,
                                    const XML_Char *entityName,
                                    int is_parameter_entity);





















































typedef struct {
  int map[256];
  void *data;
  int (XMLCALL *convert)(void *data, const char *s);
  void (XMLCALL *release)(void *data);
} XML_Encoding;
















typedef int (XMLCALL *XML_UnknownEncodingHandler) (
                                    void *encodingHandlerData,
                                    const XML_Char *name,
                                    XML_Encoding *info);

XMLPARSEAPI(void)
XML_SetElementHandler(XML_Parser parser,
                      XML_StartElementHandler start,
                      XML_EndElementHandler end);

XMLPARSEAPI(void)
XML_SetStartElementHandler(XML_Parser parser,
                           XML_StartElementHandler handler);

XMLPARSEAPI(void)
XML_SetEndElementHandler(XML_Parser parser,
                         XML_EndElementHandler handler);

XMLPARSEAPI(void)
XML_SetCharacterDataHandler(XML_Parser parser,
                            XML_CharacterDataHandler handler);

XMLPARSEAPI(void)
XML_SetProcessingInstructionHandler(XML_Parser parser,
                                    XML_ProcessingInstructionHandler handler);
XMLPARSEAPI(void)
XML_SetCommentHandler(XML_Parser parser,
                      XML_CommentHandler handler);

XMLPARSEAPI(void)
XML_SetCdataSectionHandler(XML_Parser parser,
                           XML_StartCdataSectionHandler start,
                           XML_EndCdataSectionHandler end);

XMLPARSEAPI(void)
XML_SetStartCdataSectionHandler(XML_Parser parser,
                                XML_StartCdataSectionHandler start);

XMLPARSEAPI(void)
XML_SetEndCdataSectionHandler(XML_Parser parser,
                              XML_EndCdataSectionHandler end);





XMLPARSEAPI(void)
XML_SetDefaultHandler(XML_Parser parser,
                      XML_DefaultHandler handler);





XMLPARSEAPI(void)
XML_SetDefaultHandlerExpand(XML_Parser parser,
                            XML_DefaultHandler handler);

XMLPARSEAPI(void)
XML_SetDoctypeDeclHandler(XML_Parser parser,
                          XML_StartDoctypeDeclHandler start,
                          XML_EndDoctypeDeclHandler end);

XMLPARSEAPI(void)
XML_SetStartDoctypeDeclHandler(XML_Parser parser,
                               XML_StartDoctypeDeclHandler start);

XMLPARSEAPI(void)
XML_SetEndDoctypeDeclHandler(XML_Parser parser,
                             XML_EndDoctypeDeclHandler end);

XMLPARSEAPI(void)
XML_SetUnparsedEntityDeclHandler(XML_Parser parser,
                                 XML_UnparsedEntityDeclHandler handler);

XMLPARSEAPI(void)
XML_SetNotationDeclHandler(XML_Parser parser,
                           XML_NotationDeclHandler handler);

XMLPARSEAPI(void)
XML_SetNamespaceDeclHandler(XML_Parser parser,
                            XML_StartNamespaceDeclHandler start,
                            XML_EndNamespaceDeclHandler end);

XMLPARSEAPI(void)
XML_SetStartNamespaceDeclHandler(XML_Parser parser,
                                 XML_StartNamespaceDeclHandler start);

XMLPARSEAPI(void)
XML_SetEndNamespaceDeclHandler(XML_Parser parser,
                               XML_EndNamespaceDeclHandler end);

XMLPARSEAPI(void)
XML_SetNotStandaloneHandler(XML_Parser parser,
                            XML_NotStandaloneHandler handler);

XMLPARSEAPI(void)
XML_SetExternalEntityRefHandler(XML_Parser parser,
                                XML_ExternalEntityRefHandler handler);





XMLPARSEAPI(void)
XML_SetExternalEntityRefHandlerArg(XML_Parser parser,
                                   void *arg);

XMLPARSEAPI(void)
XML_SetSkippedEntityHandler(XML_Parser parser,
                            XML_SkippedEntityHandler handler);

XMLPARSEAPI(void)
XML_SetUnknownEncodingHandler(XML_Parser parser,
                              XML_UnknownEncodingHandler handler,
                              void *encodingHandlerData);





XMLPARSEAPI(void)
XML_DefaultCurrent(XML_Parser parser);















XMLPARSEAPI(void)
XML_SetReturnNSTriplet(XML_Parser parser, int do_nst);


XMLPARSEAPI(void)
XML_SetUserData(XML_Parser parser, void *userData);


#define XML_GetUserData(parser) (*(void **)(parser))







XMLPARSEAPI(enum XML_Status)
XML_SetEncoding(XML_Parser parser, const XML_Char *encoding);





XMLPARSEAPI(void)
XML_UseParserAsHandlerArg(XML_Parser parser);



















XMLPARSEAPI(enum XML_Error)
XML_UseForeignDTD(XML_Parser parser, XML_Bool useDTD);










XMLPARSEAPI(enum XML_Status)
XML_SetBase(XML_Parser parser, const XML_Char *base);

XMLPARSEAPI(const XML_Char *)
XML_GetBase(XML_Parser parser);







XMLPARSEAPI(int)
XML_GetSpecifiedAttributeCount(XML_Parser parser);






XMLPARSEAPI(int)
XML_GetIdAttributeIndex(XML_Parser parser);










XMLPARSEAPI(enum XML_Status)
XML_Parse(XML_Parser parser, const char *s, int len, int isFinal);

XMLPARSEAPI(void *)
XML_GetBuffer(XML_Parser parser, int len);

XMLPARSEAPI(enum XML_Status)
XML_ParseBuffer(XML_Parser parser, int len, int isFinal);
































XMLPARSEAPI(enum XML_Status)
XML_StopParser(XML_Parser parser, XML_Bool resumable);













XMLPARSEAPI(enum XML_Status)
XML_ResumeParser(XML_Parser parser);

enum XML_Parsing {
  XML_INITIALIZED,
  XML_PARSING,
  XML_FINISHED,
  XML_SUSPENDED
};

typedef struct {
  enum XML_Parsing parsing;
  XML_Bool finalBuffer;
} XML_ParsingStatus;






XMLPARSEAPI(void)
XML_GetParsingStatus(XML_Parser parser, XML_ParsingStatus *status);

















XMLPARSEAPI(XML_Parser)
XML_ExternalEntityParserCreate(XML_Parser parser,
                               const XML_Char *context,
                               const XML_Char *encoding);

enum XML_ParamEntityParsing {
  XML_PARAM_ENTITY_PARSING_NEVER,
  XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE,
  XML_PARAM_ENTITY_PARSING_ALWAYS
};
























XMLPARSEAPI(int)
XML_SetParamEntityParsing(XML_Parser parser,
                          enum XML_ParamEntityParsing parsing);




XMLPARSEAPI(enum XML_Error)
XML_GetErrorCode(XML_Parser parser);

















XMLPARSEAPI(XML_Size) XML_GetCurrentLineNumber(XML_Parser parser);
XMLPARSEAPI(XML_Size) XML_GetCurrentColumnNumber(XML_Parser parser);
XMLPARSEAPI(XML_Index) XML_GetCurrentByteIndex(XML_Parser parser);




XMLPARSEAPI(int)
XML_GetCurrentByteCount(XML_Parser parser);











XMLPARSEAPI(const char *)
XML_GetInputContext(XML_Parser parser,
                    int *offset,
                    int *size);


#define XML_GetErrorLineNumber   XML_GetCurrentLineNumber
#define XML_GetErrorColumnNumber XML_GetCurrentColumnNumber
#define XML_GetErrorByteIndex    XML_GetCurrentByteIndex


XMLPARSEAPI(void)
XML_FreeContentModel(XML_Parser parser, XML_Content *model);


XMLPARSEAPI(void *)
XML_MemMalloc(XML_Parser parser, size_t size);

XMLPARSEAPI(void *)
XML_MemRealloc(XML_Parser parser, void *ptr, size_t size);

XMLPARSEAPI(void)
XML_MemFree(XML_Parser parser, void *ptr);


XMLPARSEAPI(void)
XML_ParserFree(XML_Parser parser);


XMLPARSEAPI(const XML_LChar *)
XML_ErrorString(enum XML_Error code);


XMLPARSEAPI(const XML_LChar *)
XML_ExpatVersion(void);

typedef struct {
  int major;
  int minor;
  int micro;
} XML_Expat_Version;




XMLPARSEAPI(XML_Expat_Version)
XML_ExpatVersionInfo(void);


enum XML_FeatureEnum {
  XML_FEATURE_END = 0,
  XML_FEATURE_UNICODE,
  XML_FEATURE_UNICODE_WCHAR_T,
  XML_FEATURE_DTD,
  XML_FEATURE_CONTEXT_BYTES,
  XML_FEATURE_MIN_SIZE,
  XML_FEATURE_SIZEOF_XML_CHAR,
  XML_FEATURE_SIZEOF_XML_LCHAR,
  XML_FEATURE_NS
  
};

typedef struct {
  enum XML_FeatureEnum  feature;
  const XML_LChar       *name;
  long int              value;
} XML_Feature;

XMLPARSEAPI(const XML_Feature *)
XML_GetFeatureList(void);







#define XML_MAJOR_VERSION 2
#define XML_MINOR_VERSION 0
#define XML_MICRO_VERSION 0


XMLPARSEAPI(const XML_Char*)
MOZ_XML_GetMismatchedTag(XML_Parser parser);


#ifdef __cplusplus
}
#endif

#endif
