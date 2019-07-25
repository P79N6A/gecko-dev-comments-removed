



































#ifndef SQLiteBridge_h
#define SQLiteBridge_h

#include "sqlite3.h"

void setup_sqlite_functions(void *sqlite_handle);

#define SQLITE_WRAPPER(name, return_type, args...) \
typedef return_type (*name ## _t)(args);  \
extern name ## _t f_ ## name;

SQLITE_WRAPPER(sqlite3_open, int, const char*, sqlite3**)
SQLITE_WRAPPER(sqlite3_errmsg, const char*, sqlite3*)
SQLITE_WRAPPER(sqlite3_prepare_v2, int, sqlite3*, const char*, int, sqlite3_stmt**, const char**)
SQLITE_WRAPPER(sqlite3_bind_parameter_count, int, sqlite3_stmt*)
SQLITE_WRAPPER(sqlite3_bind_text, int, sqlite3_stmt*, int, const char*, int, void(*)(void*))
SQLITE_WRAPPER(sqlite3_step, int, sqlite3_stmt*)
SQLITE_WRAPPER(sqlite3_column_count, int, sqlite3_stmt*)
SQLITE_WRAPPER(sqlite3_finalize, int, sqlite3_stmt*)
SQLITE_WRAPPER(sqlite3_close, int, sqlite3*)
SQLITE_WRAPPER(sqlite3_column_name, const char*, sqlite3_stmt*, int)
SQLITE_WRAPPER(sqlite3_column_type, int, sqlite3_stmt*, int)
SQLITE_WRAPPER(sqlite3_column_blob, const void*, sqlite3_stmt*, int)
SQLITE_WRAPPER(sqlite3_column_bytes, int, sqlite3_stmt*, int)
SQLITE_WRAPPER(sqlite3_column_text, const unsigned char*, sqlite3_stmt*, int)

#endif 
