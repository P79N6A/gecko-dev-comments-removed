















#ifndef _SQLITE3_H_
#define _SQLITE3_H_
#include <stdarg.h>     




#ifdef __cplusplus
extern "C" {
#endif




#ifdef SQLITE_VERSION
# undef SQLITE_VERSION
#endif
#define SQLITE_VERSION         "3.3.17"













#ifdef SQLITE_VERSION_NUMBER
# undef SQLITE_VERSION_NUMBER
#endif
#define SQLITE_VERSION_NUMBER 3003017








extern const char sqlite3_version[];
const char *sqlite3_libversion(void);





int sqlite3_libversion_number(void);





typedef struct sqlite3 sqlite3;







#ifdef SQLITE_INT64_TYPE
  typedef SQLITE_INT64_TYPE sqlite_int64;
  typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 sqlite_int64;
  typedef unsigned __int64 sqlite_uint64;
#else
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;
#endif





#ifdef SQLITE_OMIT_FLOATING_POINT
# define double sqlite_int64
#endif












int sqlite3_close(sqlite3 *);




typedef int (*sqlite3_callback)(void*,int,char**, char**);









































int sqlite3_exec(
  sqlite3*,                     
  const char *sql,              
  sqlite3_callback,             
  void *,                       
  char **errmsg                 
);




#define SQLITE_OK           0   /* Successful result */

#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* NOT USED. Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* NOT USED. Table or record not found */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* NOT USED. Database lock protocol error */
#define SQLITE_EMPTY       16   /* Database is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* NOT USED. Too much data for one row */
#define SQLITE_CONSTRAINT  19   /* Abort due to contraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Auxiliary database format error */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */

























#define SQLITE_IOERR_READ          (SQLITE_IOERR | (1<<8))
#define SQLITE_IOERR_SHORT_READ    (SQLITE_IOERR | (2<<8))
#define SQLITE_IOERR_WRITE         (SQLITE_IOERR | (3<<8))
#define SQLITE_IOERR_FSYNC         (SQLITE_IOERR | (4<<8))
#define SQLITE_IOERR_DIR_FSYNC     (SQLITE_IOERR | (5<<8))
#define SQLITE_IOERR_TRUNCATE      (SQLITE_IOERR | (6<<8))
#define SQLITE_IOERR_FSTAT         (SQLITE_IOERR | (7<<8))
#define SQLITE_IOERR_UNLOCK        (SQLITE_IOERR | (8<<8))
#define SQLITE_IOERR_RDLOCK        (SQLITE_IOERR | (9<<8))
#define SQLITE_IOERR_DELETE        (SQLITE_IOERR | (10<<8))




int sqlite3_extended_result_codes(sqlite3*, int onoff);








sqlite_int64 sqlite3_last_insert_rowid(sqlite3*);



























int sqlite3_changes(sqlite3*);

















int sqlite3_total_changes(sqlite3*);










void sqlite3_interrupt(sqlite3*);
















int sqlite3_complete(const char *sql);
int sqlite3_complete16(const void *sql);








































int sqlite3_busy_handler(sqlite3*, int(*)(void*,int), void*);











int sqlite3_busy_timeout(sqlite3*, int ms);










































int sqlite3_get_table(
  sqlite3*,               
  const char *sql,       
  char ***resultp,       
  int *nrow,             
  int *ncolumn,          
  char **errmsg          
);




void sqlite3_free_table(char **result);










































char *sqlite3_mprintf(const char*,...);
char *sqlite3_vmprintf(const char*, va_list);
char *sqlite3_snprintf(int,char*,const char*, ...);








void *sqlite3_malloc(int);
void *sqlite3_realloc(void*, int);
void sqlite3_free(void*);

#ifndef SQLITE_OMIT_AUTHORIZATION








int sqlite3_set_authorizer(
  sqlite3*,
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*),
  void *pUserData
);
#endif














#define SQLITE_COPY                  0   /* Table Name      File Name       */
#define SQLITE_CREATE_INDEX          1   /* Index Name      Table Name      */
#define SQLITE_CREATE_TABLE          2   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_INDEX     3   /* Index Name      Table Name      */
#define SQLITE_CREATE_TEMP_TABLE     4   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_TRIGGER   5   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_TEMP_VIEW      6   /* View Name       NULL            */
#define SQLITE_CREATE_TRIGGER        7   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_VIEW           8   /* View Name       NULL            */
#define SQLITE_DELETE                9   /* Table Name      NULL            */
#define SQLITE_DROP_INDEX           10   /* Index Name      Table Name      */
#define SQLITE_DROP_TABLE           11   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_INDEX      12   /* Index Name      Table Name      */
#define SQLITE_DROP_TEMP_TABLE      13   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_TRIGGER    14   /* Trigger Name    Table Name      */
#define SQLITE_DROP_TEMP_VIEW       15   /* View Name       NULL            */
#define SQLITE_DROP_TRIGGER         16   /* Trigger Name    Table Name      */
#define SQLITE_DROP_VIEW            17   /* View Name       NULL            */
#define SQLITE_INSERT               18   /* Table Name      NULL            */
#define SQLITE_PRAGMA               19   /* Pragma Name     1st arg or NULL */
#define SQLITE_READ                 20   /* Table Name      Column Name     */
#define SQLITE_SELECT               21   /* NULL            NULL            */
#define SQLITE_TRANSACTION          22   /* NULL            NULL            */
#define SQLITE_UPDATE               23   /* Table Name      Column Name     */
#define SQLITE_ATTACH               24   /* Filename        NULL            */
#define SQLITE_DETACH               25   /* Database Name   NULL            */
#define SQLITE_ALTER_TABLE          26   /* Database Name   Table Name      */
#define SQLITE_REINDEX              27   /* Index Name      NULL            */
#define SQLITE_ANALYZE              28   /* Table Name      NULL            */
#define SQLITE_CREATE_VTABLE        29   /* Table Name      Module Name     */
#define SQLITE_DROP_VTABLE          30   /* Table Name      Module Name     */
#define SQLITE_FUNCTION             31   /* Function Name   NULL            */






#define SQLITE_DENY   1   /* Abort the SQL statement with an error */
#define SQLITE_IGNORE 2   /* Don't allow access, but don't generate an error */











void *sqlite3_trace(sqlite3*, void(*xTrace)(void*,const char*), void*);
void *sqlite3_profile(sqlite3*,
   void(*xProfile)(void*,const char*,sqlite_uint64), void*);



























void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*);














void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);


















int sqlite3_open(
  const char *filename,   
  sqlite3 **ppDb          
);
int sqlite3_open16(
  const void *filename,   
  sqlite3 **ppDb          
);
















int sqlite3_errcode(sqlite3 *db);









const char *sqlite3_errmsg(sqlite3*);









const void *sqlite3_errmsg16(sqlite3*);





typedef struct sqlite3_stmt sqlite3_stmt;


























int sqlite3_prepare(
  sqlite3 *db,            
  const char *zSql,       
  int nBytes,             
  sqlite3_stmt **ppStmt,  
  const char **pzTail     
);
int sqlite3_prepare16(
  sqlite3 *db,            
  const void *zSql,       
  int nBytes,             
  sqlite3_stmt **ppStmt,  
  const void **pzTail     
);











int sqlite3_prepare_v2(
  sqlite3 *db,            
  const char *zSql,       
  int nBytes,             
  sqlite3_stmt **ppStmt,  
  const char **pzTail     
);
int sqlite3_prepare16_v2(
  sqlite3 *db,            
  const void *zSql,       
  int nBytes,             
  sqlite3_stmt **ppStmt,  
  const void **pzTail     
);





typedef struct sqlite3_context sqlite3_context;
typedef struct Mem sqlite3_value;






























int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
int sqlite3_bind_double(sqlite3_stmt*, int, double);
int sqlite3_bind_int(sqlite3_stmt*, int, int);
int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite_int64);
int sqlite3_bind_null(sqlite3_stmt*, int);
int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);





int sqlite3_bind_parameter_count(sqlite3_stmt*);







const char *sqlite3_bind_parameter_name(sqlite3_stmt*, int);






int sqlite3_bind_parameter_index(sqlite3_stmt*, const char *zName);




int sqlite3_clear_bindings(sqlite3_stmt*);






int sqlite3_column_count(sqlite3_stmt *pStmt);







const char *sqlite3_column_name(sqlite3_stmt*,int);
const void *sqlite3_column_name16(sqlite3_stmt*,int);


















const char *sqlite3_column_database_name(sqlite3_stmt*,int);
const void *sqlite3_column_database_name16(sqlite3_stmt*,int);
const char *sqlite3_column_table_name(sqlite3_stmt*,int);
const void *sqlite3_column_table_name16(sqlite3_stmt*,int);
const char *sqlite3_column_origin_name(sqlite3_stmt*,int);
const void *sqlite3_column_origin_name16(sqlite3_stmt*,int);



















const char *sqlite3_column_decltype(sqlite3_stmt *, int i);



















const void *sqlite3_column_decltype16(sqlite3_stmt*,int);

































int sqlite3_step(sqlite3_stmt*);










int sqlite3_data_count(sqlite3_stmt *pStmt);





#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2

#define SQLITE_BLOB     4
#define SQLITE_NULL     5






#ifdef SQLITE_TEXT
# undef SQLITE_TEXT
#else
# define SQLITE_TEXT     3
#endif
#define SQLITE3_TEXT     3


























































const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes16(sqlite3_stmt*, int iCol);
double sqlite3_column_double(sqlite3_stmt*, int iCol);
int sqlite3_column_int(sqlite3_stmt*, int iCol);
sqlite_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
const void *sqlite3_column_text16(sqlite3_stmt*, int iCol);
int sqlite3_column_type(sqlite3_stmt*, int iCol);
int sqlite3_column_numeric_type(sqlite3_stmt*, int iCol);
sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol);















int sqlite3_finalize(sqlite3_stmt *pStmt);








int sqlite3_reset(sqlite3_stmt *pStmt);




































int sqlite3_create_function(
  sqlite3 *,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void*,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
int sqlite3_create_function16(
  sqlite3*,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void*,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);





int sqlite3_aggregate_count(sqlite3_context*);









const void *sqlite3_value_blob(sqlite3_value*);
int sqlite3_value_bytes(sqlite3_value*);
int sqlite3_value_bytes16(sqlite3_value*);
double sqlite3_value_double(sqlite3_value*);
int sqlite3_value_int(sqlite3_value*);
sqlite_int64 sqlite3_value_int64(sqlite3_value*);
const unsigned char *sqlite3_value_text(sqlite3_value*);
const void *sqlite3_value_text16(sqlite3_value*);
const void *sqlite3_value_text16le(sqlite3_value*);
const void *sqlite3_value_text16be(sqlite3_value*);
int sqlite3_value_type(sqlite3_value*);
int sqlite3_value_numeric_type(sqlite3_value*);











void *sqlite3_aggregate_context(sqlite3_context*, int nBytes);






void *sqlite3_user_data(sqlite3_context*);



























void *sqlite3_get_auxdata(sqlite3_context*, int);
void sqlite3_set_auxdata(sqlite3_context*, int, void*, void (*)(void*));














typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)





void sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));
void sqlite3_result_double(sqlite3_context*, double);
void sqlite3_result_error(sqlite3_context*, const char*, int);
void sqlite3_result_error16(sqlite3_context*, const void*, int);
void sqlite3_result_int(sqlite3_context*, int);
void sqlite3_result_int64(sqlite3_context*, sqlite_int64);
void sqlite3_result_null(sqlite3_context*);
void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
void sqlite3_result_text16(sqlite3_context*, const void*, int, void(*)(void*));
void sqlite3_result_text16le(sqlite3_context*, const void*, int,void(*)(void*));
void sqlite3_result_text16be(sqlite3_context*, const void*, int,void(*)(void*));
void sqlite3_result_value(sqlite3_context*, sqlite3_value*);





#define SQLITE_UTF8           1
#define SQLITE_UTF16LE        2
#define SQLITE_UTF16BE        3
#define SQLITE_UTF16          4    /* Use native byte order */
#define SQLITE_ANY            5    /* sqlite3_create_function only */
#define SQLITE_UTF16_ALIGNED  8    /* sqlite3_create_collation only */





























int sqlite3_create_collation(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*)
);
int sqlite3_create_collation16(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*)
);

























int sqlite3_collation_needed(
  sqlite3*, 
  void*, 
  void(*)(void*,sqlite3*,int eTextRep,const char*)
);
int sqlite3_collation_needed16(
  sqlite3*, 
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const void*)
);








int sqlite3_key(
  sqlite3 *db,                   
  const void *pKey, int nKey     
);









int sqlite3_rekey(
  sqlite3 *db,                   
  const void *pKey, int nKey     
);










int sqlite3_sleep(int);










int sqlite3_expired(sqlite3_stmt*);








int sqlite3_transfer_bindings(sqlite3_stmt*, sqlite3_stmt*);











extern char *sqlite3_temp_directory;




















int sqlite3_global_recover(void);







int sqlite3_get_autocommit(sqlite3*);







sqlite3 *sqlite3_db_handle(sqlite3_stmt*);























void *sqlite3_update_hook(
  sqlite3*, 
  void(*)(void *,int ,char const *,char const *,sqlite_int64),
  void*
);
















void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);







int sqlite3_enable_shared_cache(int);









int sqlite3_release_memory(int);















void sqlite3_soft_heap_limit(int);












void sqlite3_thread_cleanup(void);


























































int sqlite3_table_column_metadata(
  sqlite3 *db,                
  const char *zDbName,        
  const char *zTableName,     
  const char *zColumnName,    
  char const **pzDataType,    
  char const **pzCollSeq,     
  int *pNotNull,              
  int *pPrimaryKey,           
  int *pAutoinc               
);



















int sqlite3_load_extension(
  sqlite3 *db,          
  const char *zFile,    
  const char *zProc,    
  char **pzErrMsg       
);











int sqlite3_enable_load_extension(sqlite3 *db, int onoff);






















int sqlite3_auto_extension(void *xEntryPoint);











void sqlite3_reset_auto_extension(void);
















typedef struct sqlite3_vtab sqlite3_vtab;
typedef struct sqlite3_index_info sqlite3_index_info;
typedef struct sqlite3_vtab_cursor sqlite3_vtab_cursor;
typedef struct sqlite3_module sqlite3_module;






struct sqlite3_module {
  int iVersion;
  int (*xCreate)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xConnect)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xBestIndex)(sqlite3_vtab *pVTab, sqlite3_index_info*);
  int (*xDisconnect)(sqlite3_vtab *pVTab);
  int (*xDestroy)(sqlite3_vtab *pVTab);
  int (*xOpen)(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor);
  int (*xClose)(sqlite3_vtab_cursor*);
  int (*xFilter)(sqlite3_vtab_cursor*, int idxNum, const char *idxStr,
                int argc, sqlite3_value **argv);
  int (*xNext)(sqlite3_vtab_cursor*);
  int (*xEof)(sqlite3_vtab_cursor*);
  int (*xColumn)(sqlite3_vtab_cursor*, sqlite3_context*, int);
  int (*xRowid)(sqlite3_vtab_cursor*, sqlite_int64 *pRowid);
  int (*xUpdate)(sqlite3_vtab *, int, sqlite3_value **, sqlite_int64 *);
  int (*xBegin)(sqlite3_vtab *pVTab);
  int (*xSync)(sqlite3_vtab *pVTab);
  int (*xCommit)(sqlite3_vtab *pVTab);
  int (*xRollback)(sqlite3_vtab *pVTab);
  int (*xFindFunction)(sqlite3_vtab *pVtab, int nArg, const char *zName,
                       void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                       void **ppArg);
};















































struct sqlite3_index_info {
  
  const int nConstraint;     
  const struct sqlite3_index_constraint {
     int iColumn;              
     unsigned char op;         
     unsigned char usable;     
     int iTermOffset;          
  } *const aConstraint;      
  const int nOrderBy;        
  const struct sqlite3_index_orderby {
     int iColumn;              
     unsigned char desc;       
  } *const aOrderBy;         

  
  struct sqlite3_index_constraint_usage {
    int argvIndex;           
    unsigned char omit;      
  } *const aConstraintUsage;
  int idxNum;                
  char *idxStr;              
  int needToFreeIdxStr;      
  int orderByConsumed;       
  double estimatedCost;      
};
#define SQLITE_INDEX_CONSTRAINT_EQ    2
#define SQLITE_INDEX_CONSTRAINT_GT    4
#define SQLITE_INDEX_CONSTRAINT_LE    8
#define SQLITE_INDEX_CONSTRAINT_LT    16
#define SQLITE_INDEX_CONSTRAINT_GE    32
#define SQLITE_INDEX_CONSTRAINT_MATCH 64







int sqlite3_create_module(
  sqlite3 *db,               
  const char *zName,         
  const sqlite3_module *,    
  void *                     
);


















struct sqlite3_vtab {
  const sqlite3_module *pModule;  
  int nRef;                       
  char *zErrMsg;                  
  
};










struct sqlite3_vtab_cursor {
  sqlite3_vtab *pVtab;      
  
};






int sqlite3_declare_vtab(sqlite3*, const char *zCreateTable);

















int sqlite3_overload_function(sqlite3*, const char *zFuncName, int nArg);

















#ifdef SQLITE_OMIT_FLOATING_POINT
# undef double
#endif

#ifdef __cplusplus
}  
#endif
#endif
