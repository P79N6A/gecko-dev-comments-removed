



#ifndef CHROME_COMMON_SQLITEUTILS_H_
#define CHROME_COMMON_SQLITEUTILS_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/string16.h"
#include "base/string_util.h"

#include "third_party/sqlite/preprocessed/sqlite3.h"


class FilePath;
class SQLTransaction;
class SQLNestedTransaction;
class SQLNestedTransactionSite;
class scoped_sqlite3_stmt_ptr;
class SQLStatement;






class SQLTransaction {
 public:
  explicit SQLTransaction(sqlite3* db);
  virtual ~SQLTransaction();

  int Begin() {
    
    
    
    
    return BeginImmediate();
  }

  int BeginExclusive() {
    return BeginCommand("BEGIN EXCLUSIVE");
  }

  int BeginImmediate() {
    return BeginCommand("BEGIN IMMEDIATE");
  }

  int BeginDeferred() {
    return BeginCommand("BEGIN DEFERRED");
  }

  int Commit() {
    return EndCommand("COMMIT");
  }

  int Rollback() {
    return EndCommand("ROLLBACK");
  }

  bool HasBegun() {
    return began_;
  }

 protected:
  virtual int BeginCommand(const char* command);
  virtual int EndCommand(const char* command);

  sqlite3* db_;
  bool began_;
  DISALLOW_COPY_AND_ASSIGN(SQLTransaction);
};





class SQLNestedTransactionSite {
 protected:
  SQLNestedTransactionSite() : db_(NULL), top_transaction_(NULL) {}
  virtual ~SQLNestedTransactionSite();

  
  
  
  
  

  virtual void OnBegin() {}
  virtual void OnCommit() {}
  virtual void OnRollback() {}

  
  
  sqlite3* GetSqlite3DB() { return db_; }

  
  
  SQLNestedTransaction* GetTopTransaction() {
    return top_transaction_;
  }

  
  
  void SetTopTransaction(SQLNestedTransaction* top);

  sqlite3* db_;
  SQLNestedTransaction* top_transaction_;
  friend class SQLNestedTransaction;
};





















class SQLNestedTransaction : public SQLTransaction {
 public:
  explicit SQLNestedTransaction(SQLNestedTransactionSite* site);
  virtual ~SQLNestedTransaction();

 protected:
  virtual int BeginCommand(const char* command);
  virtual int EndCommand(const char* command);

 private:
  bool needs_rollback_;
  SQLNestedTransactionSite* site_;
  DISALLOW_COPY_AND_ASSIGN(SQLNestedTransaction);
};




class scoped_sqlite3_stmt_ptr {
 public:
  ~scoped_sqlite3_stmt_ptr() {
    finalize();
  }

  scoped_sqlite3_stmt_ptr() : stmt_(NULL) {
  }

  explicit scoped_sqlite3_stmt_ptr(sqlite3_stmt* stmt)
    : stmt_(stmt) {
  }

  sqlite3_stmt* get() const {
    return stmt_;
  }

  void set(sqlite3_stmt* stmt) {
    finalize();
    stmt_ = stmt;
  }

  sqlite3_stmt* release() {
    sqlite3_stmt* tmp = stmt_;
    stmt_ = NULL;
    return tmp;
  }

  
  
  
  
  
  
  
  int finalize() {
    int err = sqlite3_finalize(stmt_);
    stmt_ = NULL;
    return err;
  }

 protected:
  sqlite3_stmt* stmt_;

 private:
  DISALLOW_COPY_AND_ASSIGN(scoped_sqlite3_stmt_ptr);
};





class SQLStatement : public scoped_sqlite3_stmt_ptr {
 public:
  SQLStatement() {
  }

  int prepare(sqlite3* db, const char* sql) {
    return prepare(db, sql, -1);
  }

  int prepare(sqlite3* db, const char* sql, int sql_len);

  int step();
  int reset();
  sqlite_int64 last_insert_rowid();
  sqlite3* db_handle();

  
  
  

  int bind_parameter_count();

  typedef void (*Function)(void*);

  int bind_blob(int index, std::vector<unsigned char>* blob);
  int bind_blob(int index, const void* value, int value_len);
  int bind_blob(int index, const void* value, int value_len, Function dtor);
  int bind_double(int index, double value);
  int bind_bool(int index, bool value);
  int bind_int(int index, int value);
  int bind_int64(int index, sqlite_int64 value);
  int bind_null(int index);

  int bind_string(int index, const std::string& value) {
    
    
    return bind_text(index, value.data(),
                     static_cast<int>(value.length()), SQLITE_TRANSIENT);
  }

  int bind_wstring(int index, const std::wstring& value) {
    
    
    std::string value_utf8(WideToUTF8(value));
    return bind_text(index, value_utf8.data(),
                     static_cast<int>(value_utf8.length()), SQLITE_TRANSIENT);
  }

  int bind_text(int index, const char* value) {
    return bind_text(index, value, -1, SQLITE_TRANSIENT);
  }

  
  
  int bind_text(int index, const char* value, int value_len) {
    return bind_text(index, value, value_len, SQLITE_TRANSIENT);
  }

  
  
  int bind_text(int index, const char* value, int value_len,
                Function dtor);

  int bind_text16(int index, const char16* value) {
    return bind_text16(index, value, -1, SQLITE_TRANSIENT);
  }

  
  
  int bind_text16(int index, const char16* value, int value_len) {
    return bind_text16(index, value, value_len, SQLITE_TRANSIENT);
  }

  
  
  int bind_text16(int index, const char16* value, int value_len,
                  Function dtor);

  int bind_value(int index, const sqlite3_value* value);

  
  
  

  int column_count();
  int column_type(int index);
  const void* column_blob(int index);
  bool column_blob_as_vector(int index, std::vector<unsigned char>* blob);
  bool column_blob_as_string(int index, std::string* blob);
  int column_bytes(int index);
  int column_bytes16(int index);
  double column_double(int index);
  bool column_bool(int index);
  int column_int(int index);
  sqlite_int64 column_int64(int index);
  const char* column_text(int index);
  bool column_string(int index, std::string* str);
  std::string column_string(int index);
  const char16* column_text16(int index);
  bool column_wstring(int index, std::wstring* str);
  std::wstring column_wstring(int index);

 private:
  DISALLOW_COPY_AND_ASSIGN(SQLStatement);
};







int OpenSqliteDb(const FilePath& filepath, sqlite3** database);




bool DoesSqliteTableExist(sqlite3* db,
                          const char* db_name,
                          const char* table_name);
inline bool DoesSqliteTableExist(sqlite3* db, const char* table_name) {
  return DoesSqliteTableExist(db, NULL, table_name);
}








bool DoesSqliteColumnExist(sqlite3* db,
                           const char* datbase_name,
                           const char* table_name,
                           const char* column_name,
                           const char* column_type);
inline bool DoesSqliteColumnExist(sqlite3* db,
                           const char* table_name,
                           const char* column_name,
                           const char* column_type) {
  return DoesSqliteColumnExist(db, NULL, table_name, column_name, column_type);
}



bool DoesSqliteTableHaveRow(sqlite3* db, const char* table_name);

#endif  
