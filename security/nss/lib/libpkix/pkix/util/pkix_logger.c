










































#include "pkix_logger.h"


PKIX_List *pkixLoggers = NULL;




























PKIX_List *pkixLoggersErrors = NULL;
PKIX_List *pkixLoggersDebugTrace = NULL;


PKIX_PL_MonitorLock *pkixLoggerLock = NULL;




































PKIX_Error *
pkix_Logger_Check(
        PKIX_List *pkixLoggersList,
        const char *message,
        const char *message2,
        PKIX_ERRORCLASS logComponent,
        PKIX_UInt32 currentLevel,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *messageString = NULL;
        PKIX_PL_String *message2String = NULL;
        PKIX_PL_String *msgString = NULL;
        PKIX_Error *error = NULL;
        PKIX_Boolean needLogging = PKIX_FALSE;
        PKIX_UInt32 i, length;

        




        if ((pkixLoggersList == NULL) || (message == NULL)) {
                return(NULL);
        }

        













        error = PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext);
        if (error) { return(NULL); }

        savedPkixLoggersDebugTrace = pkixLoggersDebugTrace;
        pkixLoggersDebugTrace = NULL;
        savedPkixLoggersErrors = pkixLoggersErrors;
        pkixLoggersErrors = NULL;

        
        error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, message, 0, &messageString, plContext);
        if (error) { goto cleanup; }

        if (message2) {
                error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, message2, 0, &message2String, plContext);
                if (error) { goto cleanup; }
                error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, "%s %s", 0, &formatString, plContext);
                if (error) { goto cleanup; }

        } else {
                error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, "%s", 0, &formatString, plContext);
                if (error) { goto cleanup; }

        }

        error = PKIX_PL_Sprintf
                    (&msgString,
                    plContext,
                    formatString,
                    messageString,
                    message2String);
        if (error) { goto cleanup; }

        

        error = PKIX_List_GetLength(pkixLoggersList, &length, plContext);
        if (error) { goto cleanup; }

        for (i = 0; i < length; i++) {

                error = PKIX_List_GetItem
                    (pkixLoggersList,
                    i,
                    (PKIX_PL_Object **) &logger,
                    plContext);
                if (error) { goto cleanup; }

                
                needLogging = (currentLevel <= logger->maxLevel);

                if (needLogging && (logger->callback)) {

                    




                    if (pkixLoggersList == pkixLoggersErrors) {
                            needLogging = needLogging && 
                                (currentLevel <= PKIX_LOGGER_LEVEL_WARNING);
                    } else if (pkixLoggersList == pkixLoggersDebugTrace) {
                            needLogging = needLogging && 
                                (currentLevel > PKIX_LOGGER_LEVEL_WARNING);
                    }
                
                    if (needLogging) {
                        if (logComponent == logger->logComponent) {
                            needLogging = PKIX_TRUE;
                        } else {
                            needLogging = PKIX_FALSE;
                        }
                    }

                    if (needLogging) {
                        error = logger->callback
                                (logger,
                                msgString,
                                currentLevel,
                                logComponent,
                                plContext);
                        if (error) { goto cleanup; }
                    }
                }

                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)logger, plContext);
                logger = NULL;
                if (error) { goto cleanup; }

        }

cleanup:

        if (formatString) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)formatString, plContext);
        }

        if (messageString) {
                error = PKIX_PL_Object_DecRef
                         ((PKIX_PL_Object *)messageString, plContext);
        }

        if (message2String) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)message2String, plContext);
        }

        if (msgString) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)msgString, plContext);
        }

        if (logger) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)logger, plContext);
        }

        if (pkixLoggersErrors == NULL && savedPkixLoggersErrors != NULL) {
                pkixLoggersErrors = savedPkixLoggersErrors;
        } 

        if (pkixLoggersDebugTrace == NULL && 
           savedPkixLoggersDebugTrace != NULL) {
                pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;
        }

        error = PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext);
        if (error) { return(NULL); }

        return(NULL);
}





static PKIX_Error *
pkix_Logger_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_Logger *logger = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType(object, PKIX_LOGGER_TYPE, plContext),
                    PKIX_OBJECTNOTLOGGER);

        logger = (PKIX_Logger *)object;

        

        logger->callback = NULL;
        PKIX_DECREF(logger->context);
        logger->logComponent = (PKIX_ERRORCLASS)NULL;

cleanup:

        PKIX_RETURN(LOGGER);
}





static PKIX_Error *
pkix_Logger_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        char *asciiFormat = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *contextString = NULL;
        PKIX_PL_String *componentString = NULL;
        PKIX_PL_String *loggerString = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_ToString_Helper");
        PKIX_NULLCHECK_TWO(object, pString);

        
        PKIX_CHECK(pkix_CheckType(object, PKIX_LOGGER_TYPE, plContext),
                    PKIX_OBJECTNOTLOGGER);

        logger = (PKIX_Logger *)object;

        asciiFormat =
                "[\n"
                "\tLogger: \n"
                "\tContext:          %s\n"
                "\tMaximum Level:    %d\n"
                "\tComponent Name:   %s\n"
                "]\n";

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    asciiFormat,
                    0,
                    &formatString,
                    plContext),
                    PKIX_STRINGCREATEFAILED);

        PKIX_TOSTRING(logger->context, &contextString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII,
                (void *)PKIX_ERRORCLASSNAMES[logger->logComponent],
                0,
                &componentString,
                plContext),
                PKIX_STRINGCREATEFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&loggerString,
                plContext,
                formatString,
                contextString,
                logger->maxLevel,
                componentString),
                PKIX_SPRINTFFAILED);

        *pString = loggerString;

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(contextString);
        PKIX_RETURN(LOGGER);
}





static PKIX_Error *
pkix_Logger_Equals(
        PKIX_PL_Object *first,
        PKIX_PL_Object *second,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType;
        PKIX_Boolean cmpResult;
        PKIX_Logger *firstLogger = NULL;
        PKIX_Logger *secondLogger = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_Equals");
        PKIX_NULLCHECK_THREE(first, second, pResult);

        
        PKIX_CHECK(pkix_CheckType(first, PKIX_LOGGER_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTLOGGER);

        



        if (first == second){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType(second, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_LOGGER_TYPE) goto cleanup;

        firstLogger = (PKIX_Logger *)first;
        secondLogger = (PKIX_Logger *)second;

        cmpResult = PKIX_FALSE;

        if (firstLogger->callback != secondLogger->callback) {
                goto cleanup;
        }

        if (firstLogger->logComponent != secondLogger->logComponent) {
                goto cleanup;
        }

        PKIX_EQUALS  
                (firstLogger->context,
                secondLogger->context,
                &cmpResult,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (cmpResult == PKIX_FALSE) {
                goto cleanup;
        }

        if (firstLogger->maxLevel != secondLogger->maxLevel) {
                goto cleanup;
        }

        *pResult = cmpResult;

cleanup:

        PKIX_RETURN(LOGGER);
}





static PKIX_Error *
pkix_Logger_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        PKIX_UInt32 hash = 0;
        PKIX_UInt32 tempHash = 0;

        PKIX_ENTER(LOGGER, "pkix_Logger_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LOGGER_TYPE, plContext),
                    PKIX_OBJECTNOTLOGGER);

        logger = (PKIX_Logger *)object;

        PKIX_HASHCODE(logger->context, &tempHash, plContext,
                PKIX_OBJECTHASHCODEFAILED);

        hash = (((((PKIX_UInt32) logger->callback + tempHash) << 7) +
                logger->maxLevel) << 7) + (PKIX_UInt32)logger->logComponent;

        *pHashcode = hash;

cleanup:

        PKIX_RETURN(LOGGER);
}






static PKIX_Error *
pkix_Logger_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        PKIX_Logger *dupLogger = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_Duplicate");
        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                    ((PKIX_PL_Object *)object, PKIX_LOGGER_TYPE, plContext),
                    PKIX_OBJECTNOTLOGGER);

        logger = (PKIX_Logger *) object;

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LOGGER_TYPE,
                    sizeof (PKIX_Logger),
                    (PKIX_PL_Object **)&dupLogger,
                    plContext),
                    PKIX_COULDNOTCREATELOGGEROBJECT);

        dupLogger->callback = logger->callback;
        dupLogger->maxLevel = logger->maxLevel;
        
        PKIX_DUPLICATE
                    (logger->context,
                    &dupLogger->context,
                    plContext,
                    PKIX_OBJECTDUPLICATEFAILED);

        dupLogger->logComponent = logger->logComponent;

        *pNewObject = (PKIX_PL_Object *) dupLogger;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(dupLogger);
        }

        PKIX_RETURN(LOGGER);
}












PKIX_Error *
pkix_Logger_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(LOGGER, "pkix_Logger_RegisterSelf");

        entry.description = "Logger";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_Logger);
        entry.destructor = pkix_Logger_Destroy;
        entry.equalsFunction = pkix_Logger_Equals;
        entry.hashcodeFunction = pkix_Logger_Hashcode;
        entry.toStringFunction = pkix_Logger_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_Logger_Duplicate;

        systemClasses[PKIX_LOGGER_TYPE] = entry;

        PKIX_RETURN(LOGGER);
}






PKIX_Error *
PKIX_Logger_Create(
        PKIX_Logger_LogCallback callback,
        PKIX_PL_Object *loggerContext,
        PKIX_Logger **pLogger,
        void *plContext)
{
        PKIX_Logger *logger = NULL;

        PKIX_ENTER(LOGGER, "PKIX_Logger_Create");
        PKIX_NULLCHECK_ONE(pLogger);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LOGGER_TYPE,
                    sizeof (PKIX_Logger),
                    (PKIX_PL_Object **)&logger,
                    plContext),
                    PKIX_COULDNOTCREATELOGGEROBJECT);

        logger->callback = callback;
        logger->maxLevel = 0;
        logger->logComponent = (PKIX_ERRORCLASS)NULL;

        PKIX_INCREF(loggerContext);
        logger->context = loggerContext;

        *pLogger = logger;
        logger = NULL;

cleanup:

        PKIX_DECREF(logger);

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_Logger_GetLogCallback(
        PKIX_Logger *logger,
        PKIX_Logger_LogCallback *pCallback,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLogCallback");
        PKIX_NULLCHECK_TWO(logger, pCallback);

        *pCallback = logger->callback;

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_Logger_GetLoggerContext(
        PKIX_Logger *logger,
        PKIX_PL_Object **pLoggerContext,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLoggerContex");
        PKIX_NULLCHECK_TWO(logger, pLoggerContext);

        PKIX_INCREF(logger->context);
        *pLoggerContext = logger->context;

cleanup:
        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_Logger_GetMaxLoggingLevel(
        PKIX_Logger *logger,
        PKIX_UInt32 *pLevel,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetMaxLoggingLevel");
        PKIX_NULLCHECK_TWO(logger, pLevel);

        *pLevel = logger->maxLevel;

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_Logger_SetMaxLoggingLevel(
        PKIX_Logger *logger,
        PKIX_UInt32 level,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_SetMaxLoggingLevel");
        PKIX_NULLCHECK_ONE(logger);

        if (level > PKIX_LOGGER_LEVEL_MAX) {
                PKIX_ERROR(PKIX_LOGGINGLEVELEXCEEDSMAXIMUM);
        } else {
                logger->maxLevel = level;
        }

cleanup:

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_Logger_GetLoggingComponent(
        PKIX_Logger *logger,
        PKIX_ERRORCLASS *pComponent,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLoggingComponent");
        PKIX_NULLCHECK_TWO(logger, pComponent);

        *pComponent = logger->logComponent;

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_Logger_SetLoggingComponent(
        PKIX_Logger *logger,
        PKIX_ERRORCLASS component,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_SetLoggingComponent");
        PKIX_NULLCHECK_ONE(logger);

        logger->logComponent = component;

        PKIX_RETURN(LOGGER);
}











PKIX_Error *
PKIX_GetLoggers(
        PKIX_List **pLoggers,  
        void *plContext)
{
        PKIX_List *list = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_Logger *logger = NULL;
        PKIX_Logger *dupLogger = NULL;
        PKIX_UInt32 i, length;
        PKIX_Boolean locked = PKIX_FALSE;

        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLoggers");
        PKIX_NULLCHECK_ONE(pLoggers);

        PKIX_CHECK(PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext),
                PKIX_MONITORLOCKENTERFAILED);
        locked = PKIX_TRUE;

        






        savedPkixLoggersDebugTrace = pkixLoggersDebugTrace;
        pkixLoggersDebugTrace = NULL;
        savedPkixLoggersErrors = pkixLoggersErrors;
        pkixLoggersErrors = NULL;

        if (pkixLoggers == NULL) {
                length = 0;
        } else {
                PKIX_CHECK(PKIX_List_GetLength
                    (pkixLoggers, &length, plContext),
                    PKIX_LISTGETLENGTHFAILED);
        }

        
        PKIX_CHECK(PKIX_List_Create(&list, plContext),
                    PKIX_LISTCREATEFAILED);

        for (i = 0; i < length; i++) {

            PKIX_CHECK(PKIX_List_GetItem
                        (pkixLoggers,
                        i,
                        (PKIX_PL_Object **) &logger,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

            PKIX_CHECK(pkix_Logger_Duplicate
                        ((PKIX_PL_Object *)logger,
                        (PKIX_PL_Object **)&dupLogger,
                        plContext),
                        PKIX_LOGGERDUPLICATEFAILED);

            PKIX_CHECK(PKIX_List_AppendItem
                        (list,
                        (PKIX_PL_Object *) dupLogger,
                        plContext),
                        PKIX_LISTAPPENDITEMFAILED);

            PKIX_DECREF(logger);
            PKIX_DECREF(dupLogger);
        }

        
        PKIX_CHECK(PKIX_List_SetImmutable(list, plContext),
                        PKIX_LISTSETIMMUTABLEFAILED);

        *pLoggers = list;

cleanup:

        PKIX_DECREF(logger);

        
        pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;
        pkixLoggersErrors = savedPkixLoggersErrors;

        if (locked) {
                PKIX_CHECK(PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext),
                        PKIX_MONITORLOCKEXITFAILED);
        }

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_SetLoggers(
        PKIX_List *loggers,  
        void *plContext)
{
        PKIX_List *list = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_Logger *logger = NULL;
        PKIX_Logger *dupLogger = NULL;
        PKIX_Boolean locked = PKIX_FALSE;
        PKIX_UInt32 i, length;

        PKIX_ENTER(LOGGER, "PKIX_SetLoggers");

        PKIX_CHECK(PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext),
                PKIX_MONITORLOCKENTERFAILED);
        locked = PKIX_TRUE;

        
        savedPkixLoggersDebugTrace = pkixLoggersDebugTrace;
        pkixLoggersDebugTrace = NULL;
        savedPkixLoggersErrors = pkixLoggersErrors;
        pkixLoggersErrors = NULL;

        
        PKIX_DECREF(pkixLoggers);
        PKIX_DECREF(savedPkixLoggersErrors);
        PKIX_DECREF(savedPkixLoggersDebugTrace);

        if (loggers != NULL) {

                PKIX_CHECK(PKIX_List_Create(&list, plContext),
                    PKIX_LISTCREATEFAILED);

                PKIX_CHECK(PKIX_List_GetLength(loggers, &length, plContext),
                    PKIX_LISTGETLENGTHFAILED);

                for (i = 0; i < length; i++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (loggers,
                        i,
                        (PKIX_PL_Object **) &logger,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK(pkix_Logger_Duplicate
                        ((PKIX_PL_Object *)logger,
                        (PKIX_PL_Object **)&dupLogger,
                        plContext),
                        PKIX_LOGGERDUPLICATEFAILED);

                    PKIX_CHECK(PKIX_List_AppendItem
                        (list,
                        (PKIX_PL_Object *) dupLogger,
                        plContext),
                        PKIX_LISTAPPENDITEMFAILED);

                    

                    

                    if (savedPkixLoggersErrors == NULL) {

                        PKIX_CHECK(PKIX_List_Create
                                (&savedPkixLoggersErrors,
                                plContext),
                                PKIX_LISTCREATEFAILED);
                    }
        
                    PKIX_CHECK(PKIX_List_AppendItem
                            (savedPkixLoggersErrors,
                            (PKIX_PL_Object *) dupLogger,
                            plContext),
                            PKIX_LISTAPPENDITEMFAILED);

                    if (logger->maxLevel > PKIX_LOGGER_LEVEL_WARNING) {

                        

                        if (savedPkixLoggersDebugTrace == NULL) {

                            PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersDebugTrace,
                                    plContext),
                                    PKIX_LISTCREATEFAILED);
                        }
        
                        PKIX_CHECK(PKIX_List_AppendItem
                                (savedPkixLoggersDebugTrace,
                                (PKIX_PL_Object *) dupLogger,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);
                    }
                    PKIX_DECREF(logger);
                    PKIX_DECREF(dupLogger);

                }

                pkixLoggers = list;
        }

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(list);
                PKIX_DECREF(savedPkixLoggersErrors);
                PKIX_DECREF(savedPkixLoggersDebugTrace);
                pkixLoggers = NULL;
        }

        PKIX_DECREF(logger);

        
        pkixLoggersErrors = savedPkixLoggersErrors;
        pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;

        if (locked) {
                PKIX_CHECK(PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext),
                        PKIX_MONITORLOCKEXITFAILED);
        }

        PKIX_RETURN(LOGGER);
}




PKIX_Error *
PKIX_AddLogger(
        PKIX_Logger *logger,
        void *plContext)
{
        PKIX_Logger *dupLogger = NULL;
        PKIX_Logger *addLogger = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_Boolean locked = PKIX_FALSE;
        PKIX_UInt32 i, length;

        PKIX_ENTER(LOGGER, "PKIX_Logger_AddLogger");
        PKIX_NULLCHECK_ONE(logger);

        PKIX_CHECK(PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext),
                PKIX_MONITORLOCKENTERFAILED);
        locked = PKIX_TRUE;

        savedPkixLoggersDebugTrace = pkixLoggersDebugTrace;
        pkixLoggersDebugTrace = NULL;
        savedPkixLoggersErrors = pkixLoggersErrors;
        pkixLoggersErrors = NULL;

        PKIX_DECREF(savedPkixLoggersErrors);
        PKIX_DECREF(savedPkixLoggersDebugTrace);

        if (pkixLoggers == NULL) {

            PKIX_CHECK(PKIX_List_Create(&pkixLoggers, plContext),
                    PKIX_LISTCREATEFAILED);
        }

        PKIX_CHECK(pkix_Logger_Duplicate
                    ((PKIX_PL_Object *)logger,
                    (PKIX_PL_Object **)&dupLogger,
                    plContext),
                    PKIX_LOGGERDUPLICATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                    (pkixLoggers,
                    (PKIX_PL_Object *) dupLogger,
                    plContext),
                    PKIX_LISTAPPENDITEMFAILED);

        PKIX_CHECK(PKIX_List_GetLength(pkixLoggers, &length, plContext),
                    PKIX_LISTGETLENGTHFAILED);

        
        for (i = 0; i < length; i++) {

                PKIX_CHECK(PKIX_List_GetItem
                        (pkixLoggers,
                        i,
                        (PKIX_PL_Object **) &addLogger,
                        plContext),
                        PKIX_LISTGETITEMFAILED);


                

                if (savedPkixLoggersErrors == NULL) {

                        PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersErrors,
                                    plContext),
                                    PKIX_LISTCREATEFAILED);
                }
        
                PKIX_CHECK(PKIX_List_AppendItem
                        (savedPkixLoggersErrors,
                        (PKIX_PL_Object *) addLogger,
                        plContext),
                        PKIX_LISTAPPENDITEMFAILED);
                            
                if (addLogger->maxLevel > PKIX_LOGGER_LEVEL_WARNING) {

                        

                        if (savedPkixLoggersDebugTrace == NULL) {

                            PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersDebugTrace,
                                    plContext),
                                    PKIX_LISTCREATEFAILED);
                        }

                        PKIX_CHECK(PKIX_List_AppendItem
                                (savedPkixLoggersDebugTrace,
                                (PKIX_PL_Object *) addLogger,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);
                }

                PKIX_DECREF(addLogger);

        }

cleanup:

        PKIX_DECREF(dupLogger);
        PKIX_DECREF(addLogger);

        
        pkixLoggersErrors = savedPkixLoggersErrors;
        pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;

        if (locked) {
                PKIX_CHECK(PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext),
                        PKIX_MONITORLOCKEXITFAILED);
        }

       PKIX_RETURN(LOGGER);
}
