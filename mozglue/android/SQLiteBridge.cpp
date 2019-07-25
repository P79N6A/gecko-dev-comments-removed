



































#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <android/log.h>
#include "dlfcn.h"
#include "APKOpen.h"
#ifndef MOZ_OLD_LINKER
#include "ElfLoader.h"
#endif
#include "SQLiteBridge.h"

#ifdef DEBUG
#define LOG(x...) __android_log_print(ANDROID_LOG_INFO, "GeckoJNI", x)
#else
#define LOG(x...)
#endif

#define SQLITE_WRAPPER_INT(name) name ## _t f_ ## name;

SQLITE_WRAPPER_INT(sqlite3_open)
SQLITE_WRAPPER_INT(sqlite3_errmsg)
SQLITE_WRAPPER_INT(sqlite3_prepare_v2)
SQLITE_WRAPPER_INT(sqlite3_bind_parameter_count)
SQLITE_WRAPPER_INT(sqlite3_bind_text)
SQLITE_WRAPPER_INT(sqlite3_step)
SQLITE_WRAPPER_INT(sqlite3_column_count)
SQLITE_WRAPPER_INT(sqlite3_finalize)
SQLITE_WRAPPER_INT(sqlite3_close)
SQLITE_WRAPPER_INT(sqlite3_column_name)
SQLITE_WRAPPER_INT(sqlite3_column_type)
SQLITE_WRAPPER_INT(sqlite3_column_blob)
SQLITE_WRAPPER_INT(sqlite3_column_bytes)
SQLITE_WRAPPER_INT(sqlite3_column_text)
SQLITE_WRAPPER_INT(sqlite3_changes)
SQLITE_WRAPPER_INT(sqlite3_last_insert_rowid)

void setup_sqlite_functions(void *sqlite_handle)
{
#define GETFUNC(name) f_ ## name = (name ## _t) __wrap_dlsym(sqlite_handle, #name)
  GETFUNC(sqlite3_open);
  GETFUNC(sqlite3_errmsg);
  GETFUNC(sqlite3_prepare_v2);
  GETFUNC(sqlite3_bind_parameter_count);
  GETFUNC(sqlite3_bind_text);
  GETFUNC(sqlite3_step);
  GETFUNC(sqlite3_column_count);
  GETFUNC(sqlite3_finalize);
  GETFUNC(sqlite3_close);
  GETFUNC(sqlite3_column_name);
  GETFUNC(sqlite3_column_type);
  GETFUNC(sqlite3_column_blob);
  GETFUNC(sqlite3_column_bytes);
  GETFUNC(sqlite3_column_text);
  GETFUNC(sqlite3_changes);
  GETFUNC(sqlite3_last_insert_rowid);
#undef GETFUNC
}

static bool initialized = false;
static jclass stringClass;
static jclass objectClass;
static jclass byteBufferClass;
static jclass arrayListClass;
static jmethodID jByteBufferAllocateDirect;
static jmethodID jArrayListAdd;
static jobject jNull;

static void
JNI_Throw(JNIEnv* jenv, const char* name, const char* msg)
{
    jclass cls = jenv->FindClass(name);
    if (cls == NULL) {
        LOG("Couldn't find exception class (or exception pending)\n");
        return;
    }
    int rc = jenv->ThrowNew(cls, msg);
    if (rc < 0) {
        LOG("Error throwing exception\n");
    }
    jenv->DeleteLocalRef(cls);
}

static void
JNI_Setup(JNIEnv* jenv)
{
    if (initialized) return;

    objectClass     = jenv->FindClass("java/lang/Object");
    stringClass     = jenv->FindClass("java/lang/String");
    byteBufferClass = jenv->FindClass("java/nio/ByteBuffer");
    arrayListClass  = jenv->FindClass("java/util/ArrayList");
    jNull           = jenv->NewGlobalRef(NULL);

    if (stringClass == NULL || objectClass == NULL
        || byteBufferClass == NULL || arrayListClass == NULL) {
        LOG("Error finding classes");
        JNI_Throw(jenv, "org/mozilla/gecko/sqlite/SQLiteBridgeException",
                  "FindClass error");
        return;
    }

    
    jByteBufferAllocateDirect =
        jenv->GetStaticMethodID(byteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;");
    
    jArrayListAdd =
        jenv->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

    if (jByteBufferAllocateDirect == NULL || jArrayListAdd == NULL) {
        LOG("Error finding methods");
        JNI_Throw(jenv, "org/mozilla/gecko/sqlite/SQLiteBridgeException",
                  "GetMethodId error");
        return;
    }

    initialized = true;
}

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_sqlite_SQLiteBridge_sqliteCall(JNIEnv* jenv, jclass,
                                                      jstring jDb,
                                                      jstring jQuery,
                                                      jobjectArray jParams,
                                                      jobject jColumns,
                                                      jobject jArrayList)
{
    JNI_Setup(jenv);

    char* errorMsg;
    jsize numPars = 0;

    const char* queryStr;
    queryStr = jenv->GetStringUTFChars(jQuery, NULL);

    const char* dbPath;
    dbPath = jenv->GetStringUTFChars(jDb, NULL);

    const char *pzTail;
    sqlite3_stmt *ppStmt;
    sqlite3 *db;
    int rc;
    rc = f_sqlite3_open(dbPath, &db);
    jenv->ReleaseStringUTFChars(jDb, dbPath);

    if (rc != SQLITE_OK) {
        asprintf(&errorMsg, "Can't open database: %s\n", f_sqlite3_errmsg(db));
        goto error_close;
    }

    rc = f_sqlite3_prepare_v2(db, queryStr, -1, &ppStmt, &pzTail);
    if (rc != SQLITE_OK || ppStmt == NULL) {
        asprintf(&errorMsg, "Can't prepare statement: %s\n", f_sqlite3_errmsg(db));
        goto error_close;
    }
    jenv->ReleaseStringUTFChars(jQuery, queryStr);

    
    if (jParams != NULL) {
        numPars = jenv->GetArrayLength(jParams);
    }
    int sqlNumPars;
    sqlNumPars = f_sqlite3_bind_parameter_count(ppStmt);
    if (numPars != sqlNumPars) {
        asprintf(&errorMsg, "Passed parameter count (%d) doesn't match SQL parameter count (%d)\n",
            numPars, sqlNumPars);
        goto error_close;
    }

    if (jParams != NULL) {
        
        if (numPars > 0) {
            for (int i = 0; i < numPars; i++) {
                jobject jObjectParam = jenv->GetObjectArrayElement(jParams, i);
                
                
                jboolean isString = jenv->IsInstanceOf(jObjectParam, stringClass);
                if (isString != JNI_TRUE) {
                    asprintf(&errorMsg, "Parameter is not of String type");
                    goto error_close;
                }
                jstring jStringParam = (jstring)jObjectParam;
                const char* paramStr = jenv->GetStringUTFChars(jStringParam, NULL);
                
                rc = f_sqlite3_bind_text(ppStmt, i + 1, paramStr, -1, SQLITE_TRANSIENT);
                jenv->ReleaseStringUTFChars(jStringParam, paramStr);
                if (rc != SQLITE_OK) {
                    asprintf(&errorMsg, "Error binding query parameter");
                    goto error_close;
                }
            }
        }
    }

    
    rc = f_sqlite3_step(ppStmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        asprintf(&errorMsg, "Can't step statement: (%d) %s\n", rc, f_sqlite3_errmsg(db));
        goto error_close;
    }

    
    int cols;
    cols = f_sqlite3_column_count(ppStmt);
    for (int i = 0; i < cols; i++) {
        const char* colName = f_sqlite3_column_name(ppStmt, i);
        jstring jStr = jenv->NewStringUTF(colName);
        jenv->CallBooleanMethod(jColumns, jArrayListAdd, jStr);
        jenv->DeleteLocalRef(jStr);
    }

    
    if (rc == SQLITE_DONE) {
        jclass integerClass = jenv->FindClass("java/lang/Integer");
        jmethodID intConstructor = jenv->GetMethodID(integerClass, "<init>", "(I)V");
        
        jobjectArray jRow = jenv->NewObjectArray(2, objectClass, NULL);
        if (jRow == NULL) {
            asprintf(&errorMsg, "Can't allocate jRow Object[]\n");
            goto error_close;
        }

        int id = f_sqlite3_last_insert_rowid(db);
        jobject jId = jenv->NewObject(integerClass, intConstructor, id);
        jenv->SetObjectArrayElement(jRow, 0, jId);
        jenv->DeleteLocalRef(jId);

        int changed = f_sqlite3_changes(db);
        jobject jChanged = jenv->NewObject(integerClass, intConstructor, changed);
        jenv->SetObjectArrayElement(jRow, 1, jChanged);
        jenv->DeleteLocalRef(jChanged);

        jenv->CallBooleanMethod(jArrayList, jArrayListAdd, jRow);
        jenv->DeleteLocalRef(jRow);
    }

    
    
    
    while (rc != SQLITE_DONE) {
        
        
        jobjectArray jRow = jenv->NewObjectArray(cols,
                                                 objectClass,
                                                 NULL);
        if (jRow == NULL) {
            asprintf(&errorMsg, "Can't allocate jRow Object[]\n");
            goto error_close;
        }

        for (int i = 0; i < cols; i++) {
            int colType = f_sqlite3_column_type(ppStmt, i);
            if (colType == SQLITE_BLOB) {
                
                const void* blob = f_sqlite3_column_blob(ppStmt, i);
                int colLen = f_sqlite3_column_bytes(ppStmt, i);

                
                jobject jByteBuffer =
                    jenv->CallStaticObjectMethod(byteBufferClass,
                                                 jByteBufferAllocateDirect,
                                                 colLen);
                if (jByteBuffer == NULL) {
                    goto error_close;
                }

                
                void* bufferArray = jenv->GetDirectBufferAddress(jByteBuffer);
                if (bufferArray == NULL) {
                    asprintf(&errorMsg, "Failure calling GetDirectBufferAddress\n");
                    goto error_close;
                }
                memcpy(bufferArray, blob, colLen);

                jenv->SetObjectArrayElement(jRow, i, jByteBuffer);
                jenv->DeleteLocalRef(jByteBuffer);
            } else if (colType == SQLITE_NULL) {
                jenv->SetObjectArrayElement(jRow, i, jNull);
            } else {
                
                const char* txt = (const char*)f_sqlite3_column_text(ppStmt, i);
                jstring jStr = jenv->NewStringUTF(txt);
                jenv->SetObjectArrayElement(jRow, i, jStr);
                jenv->DeleteLocalRef(jStr);
            }
        }

        
        
        jenv->CallBooleanMethod(jArrayList, jArrayListAdd, jRow);

        
        jenv->DeleteLocalRef(jRow);

        
        rc = f_sqlite3_step(ppStmt);
        
        if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
            asprintf(&errorMsg, "Can't re-step statement:(%d) %s\n", rc, f_sqlite3_errmsg(db));
            goto error_close;
        }
    }

    rc = f_sqlite3_finalize(ppStmt);
    if (rc != SQLITE_OK) {
        asprintf(&errorMsg, "Can't finalize statement: %s\n", f_sqlite3_errmsg(db));
        goto error_close;
    }

    f_sqlite3_close(db);
    return;

error_close:
    f_sqlite3_close(db);
    LOG("Error in SQLiteBridge: %s\n", errorMsg);
    JNI_Throw(jenv, "org/mozilla/gecko/sqlite/SQLiteBridgeException", errorMsg);
    free(errorMsg);
    return;
}
