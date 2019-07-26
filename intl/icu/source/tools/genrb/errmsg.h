
















#ifndef ERROR_H
#define ERROR_H 1

U_CDECL_BEGIN

extern const char *gCurrentFileName;

U_CFUNC void error(uint32_t linenumber, const char *msg, ...);
U_CFUNC void warning(uint32_t linenumber, const char *msg, ...);


U_CFUNC void setShowWarning(UBool val);
U_CFUNC UBool getShowWarning(void);


U_CFUNC void setStrict(UBool val);
U_CFUNC UBool isStrict(void);


U_CFUNC void setVerbose(UBool val);
U_CFUNC UBool isVerbose(void);

U_CDECL_END

#endif
