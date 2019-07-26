








#ifndef _PKIX_UTIL_H
#define _PKIX_UTIL_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif
















































#define PKIX_LOGGER_LEVEL_TRACE                5
#define PKIX_LOGGER_LEVEL_DEBUG                4
#define PKIX_LOGGER_LEVEL_WARNING              3
#define PKIX_LOGGER_LEVEL_ERROR                2
#define PKIX_LOGGER_LEVEL_FATALERROR           1

#define PKIX_LOGGER_LEVEL_MAX                  5






































typedef PKIX_Error *
(*PKIX_Logger_LogCallback)(
        PKIX_Logger *logger,
        PKIX_PL_String *message,
        PKIX_UInt32 logLevel,
        PKIX_ERRORCLASS logComponent,
        void *plContext);



























PKIX_Error *
PKIX_Logger_Create(
        PKIX_Logger_LogCallback callback,
        PKIX_PL_Object *loggerContext,
        PKIX_Logger **pLogger,
        void *plContext);























PKIX_Error *
PKIX_Logger_GetLogCallback(
        PKIX_Logger *logger,
        PKIX_Logger_LogCallback *pCallback,
        void *plContext);






















PKIX_Error *
PKIX_Logger_GetLoggerContext(
        PKIX_Logger *logger,
        PKIX_PL_Object **pLoggerContext,
        void *plContext);


























PKIX_Error *
PKIX_Logger_GetMaxLoggingLevel(
        PKIX_Logger *logger,
        PKIX_UInt32 *pLevel,
        void *plContext);
























PKIX_Error *
PKIX_Logger_SetMaxLoggingLevel(
        PKIX_Logger *logger,
        PKIX_UInt32 level,
        void *plContext);


























PKIX_Error *
PKIX_Logger_GetLoggingComponent(
        PKIX_Logger *logger,
        PKIX_ERRORCLASS *pComponent,
        void *plContext);

























PKIX_Error *
PKIX_Logger_SetLoggingComponent(
        PKIX_Logger *logger,
        PKIX_ERRORCLASS component,
        void *plContext);
























PKIX_Error *
PKIX_GetLoggers(
        PKIX_List **pLoggers,  
        void *plContext);





















PKIX_Error *
PKIX_SetLoggers(
        PKIX_List *loggers,  
        void *plContext);





















PKIX_Error *
PKIX_AddLogger(
        PKIX_Logger *logger,
        void *plContext);




























































PKIX_Error *
PKIX_Error_Create(
        PKIX_ERRORCLASS errClass,
        PKIX_Error *cause,
        PKIX_PL_Object *info,
        PKIX_ERRORCODE errCode,
        PKIX_Error **pError,
        void *plContext);






















PKIX_Error *
PKIX_Error_GetErrorClass(
        PKIX_Error *error,
        PKIX_ERRORCLASS *pClass,
        void *plContext);






















PKIX_Error *
PKIX_Error_GetErrorCode(
        PKIX_Error *error,
        PKIX_ERRORCODE *pCode,
        void *plContext);






















PKIX_Error *
PKIX_Error_GetCause(
        PKIX_Error *error,
        PKIX_Error **pCause,
        void *plContext);






















PKIX_Error *
PKIX_Error_GetSupplementaryInfo(
        PKIX_Error *error,
        PKIX_PL_Object **pInfo,
        void *plContext);























PKIX_Error *
PKIX_Error_GetDescription(
        PKIX_Error *error,
        PKIX_PL_String **pDesc,
        void *plContext);

























PKIX_Error *
PKIX_List_Create(
        PKIX_List **pList,
        void *plContext);






















PKIX_Error *
PKIX_List_SetImmutable(
        PKIX_List *list,
        void *plContext);


























PKIX_Error *
PKIX_List_IsImmutable(
        PKIX_List *list,
        PKIX_Boolean *pImmutable,
        void *plContext);






















PKIX_Error *
PKIX_List_GetLength(
        PKIX_List *list,
        PKIX_UInt32 *pLength,
        void *plContext);






















PKIX_Error *
PKIX_List_IsEmpty(
        PKIX_List *list,
        PKIX_Boolean *pEmpty,
        void *plContext);
























PKIX_Error *
PKIX_List_AppendItem(
        PKIX_List *list,
        PKIX_PL_Object *item,
        void *plContext);




























PKIX_Error *
PKIX_List_InsertItem(
        PKIX_List *list,
        PKIX_UInt32 index,
        PKIX_PL_Object *item,
        void *plContext);

























PKIX_Error *
PKIX_List_GetItem(
        PKIX_List *list,
        PKIX_UInt32 index,
        PKIX_PL_Object **pItem,
        void *plContext);



























PKIX_Error *
PKIX_List_SetItem(
        PKIX_List *list,
        PKIX_UInt32 index,
        PKIX_PL_Object *item,
        void *plContext);


























PKIX_Error *
PKIX_List_DeleteItem(
        PKIX_List *list,
        PKIX_UInt32 index,
        void *plContext);
























PKIX_Error *
PKIX_List_ReverseList(
        PKIX_List *list,
        PKIX_List **pReversedList,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
