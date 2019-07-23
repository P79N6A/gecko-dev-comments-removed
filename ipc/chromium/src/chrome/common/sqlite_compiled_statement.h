



#ifndef CHROME_COMMON_SQLITE_COMPILED_STATEMENT_
#define CHROME_COMMON_SQLITE_COMPILED_STATEMENT_

#include <map>
#include <string>

#include "chrome/common/sqlite_utils.h"

#include "third_party/sqlite/preprocessed/sqlite3.h"





class SqliteStatementCache {
 public:
  
  SqliteStatementCache() : db_(NULL) {
  }

  explicit SqliteStatementCache(sqlite3* db) : db_(db) {
  }

  
  
  
  ~SqliteStatementCache();

  void set_db(sqlite3* db);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  SQLStatement* GetStatement(const char* func_name,
                             int func_number,
                             const char* sql) {
    return InternalGetStatement(func_name, func_number, sql);
  }

  
  
  SQLStatement* GetExistingStatement(const char* func_name,
                                     int func_number) {
    return InternalGetStatement(func_name, func_number, NULL);
  }

 private:
  
  struct FuncID {
    int number;
    std::string name;

    
    bool operator<(const FuncID& other) const;
  };

  
  
  
  SQLStatement* InternalGetStatement(const char* func_name,
                                     int func_number,
                                     const char* sql);

  sqlite3* db_;

  
  typedef std::map<FuncID, SQLStatement*> StatementMap;
  StatementMap statements_;

  DISALLOW_EVIL_CONSTRUCTORS(SqliteStatementCache);
};



class SqliteCompiledStatement {
 public:
  
  SqliteCompiledStatement(const char* func_name,
                          int func_number,
                          SqliteStatementCache& cache,
                          const char* sql);
  ~SqliteCompiledStatement();

  
  
  bool is_valid() const { return !!statement_; }

  
  
  
  SQLStatement& operator*();
  SQLStatement* operator->();
  SQLStatement* statement();

 private:
  
  
  SQLStatement* statement_;

  DISALLOW_EVIL_CONSTRUCTORS(SqliteCompiledStatement);
};










#define SQLITE_UNIQUE_STATEMENT(var_name, cache, sql) \
    SqliteCompiledStatement var_name(__FILE__, __LINE__, cache, sql)

#endif  
