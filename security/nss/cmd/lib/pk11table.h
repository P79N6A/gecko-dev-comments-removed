


































#ifndef _PK11_TABLE_H_
#define _PK11_TABLE_H_




#include <pkcs11.h>
#include "nspr.h"
#include "prtypes.h"

typedef enum {
    F_No_Function,
#undef CK_NEED_ARG_LIST
#define CK_PKCS11_FUNCTION_INFO(func) F_##func,
#include "pkcs11f.h"
#undef CK_NEED_ARG_LISt
#undef CK_PKCS11_FUNCTION_INFO
    F_SetVar,
    F_SetStringVar,
    F_NewArray,
    F_NewInitializeArgs,
    F_NewTemplate,
    F_NewMechanism,
    F_BuildTemplate,
    F_SetTemplate,
    F_Print,
    F_SaveVar,
    F_RestoreVar,
    F_Increment,
    F_Decrement,
    F_Delete,
    F_List,
    F_Run,
    F_Load,
    F_Unload,
    F_System,
    F_Loop,
    F_Time,
    F_Help,
    F_Quit,
    F_QuitIf,
    F_QuitIfString
} FunctionType;




typedef enum {
    ArgNone,
    ArgVar,
    ArgULong,
    ArgChar,
    ArgUTF8,
    ArgInfo,
    ArgSlotInfo,
    ArgTokenInfo,
    ArgSessionInfo,
    ArgAttribute,
    ArgMechanism,
    ArgMechanismInfo,
    ArgInitializeArgs,
    ArgFunctionList,

    ArgMask = 0xff,
    ArgOut = 0x100,
    ArgArray = 0x200,
    ArgNew = 0x400,
    ArgFile = 0x800,
    ArgStatic = 0x1000,
    ArgOpt = 0x2000,
    ArgFull = 0x4000
} ArgType;

typedef enum _constType
{
    ConstNone,
    ConstBool,
    ConstInfoFlags,
    ConstSlotFlags,
    ConstTokenFlags,
    ConstSessionFlags,
    ConstMechanismFlags,
    ConstInitializeFlags,
    ConstUsers,
    ConstSessionState,
    ConstObject,
    ConstHardware,
    ConstKeyType,
    ConstCertType,
    ConstAttribute,
    ConstMechanism,
    ConstResult,
    ConstTrust,
    ConstAvailableSizes,
    ConstCurrentSize
} ConstType;

typedef struct _constant {
    const char *name;
    CK_ULONG value;
    ConstType type;
    ConstType attrType;
} Constant ;




typedef struct _values {
    ArgType	type;
    ConstType	constType;
    int		size;
    char	*filename;
    void	*data;
    int 	reference;
    int		arraySize;
} Value;




typedef struct _variable Variable;
struct _variable {
    Variable *next;
    char *vname;
    Value *value;
};





#define MAX_ARGS 10



typedef struct _commands {
    char	*fname;
    FunctionType	fType;
    char	*helpString;
    ArgType	args[MAX_ARGS];
} Commands;

typedef struct _module {
    PRLibrary *library;
    CK_FUNCTION_LIST *functionList;
} Module;

typedef struct _topics {
    char	*name;
    char	*helpString;
} Topics;





extern const char **valueString;
extern const int valueCount;
extern const char **constTypeString;
extern const int constTypeCount;
extern const Constant *consts;
extern const int constCount;
extern const Commands *commands;
extern const int commandCount;
extern const Topics *topics;
extern const int topicCount;

extern const char *
getName(CK_ULONG value, ConstType type);

extern const char *
getNameFromAttribute(CK_ATTRIBUTE_TYPE type);

extern int totalKnownType(ConstType type);

#endif 

