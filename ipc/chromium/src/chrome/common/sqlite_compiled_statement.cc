



#include "chrome/common/sqlite_compiled_statement.h"

#include "base/logging.h"
#include "base/stl_util-inl.h"



SqliteStatementCache::~SqliteStatementCache() {
  STLDeleteContainerPairSecondPointers(statements_.begin(), statements_.end());
  statements_.clear();
  db_ = NULL;
}

void SqliteStatementCache::set_db(sqlite3* db) {
  DCHECK(!db_) << "Setting the database twice";
  db_ = db;
}

SQLStatement* SqliteStatementCache::InternalGetStatement(const char* func_name,
                                                         int func_number,
                                                         const char* sql) {
  FuncID id;
  id.name = func_name;
  id.number = func_number;

  StatementMap::const_iterator found = statements_.find(id);
  if (found != statements_.end())
    return found->second;

  if (!sql)
    return NULL;  

  
  SQLStatement* statement = new SQLStatement();
  if (statement->prepare(db_, sql) != SQLITE_OK) {
    const char* err_msg = sqlite3_errmsg(db_);
    NOTREACHED() << "SQL preparation error for: " << err_msg;
    return NULL;
  }

  statements_[id] = statement;
  return statement;
}

bool SqliteStatementCache::FuncID::operator<(const FuncID& other) const {
  
  
  if (number != other.number)
    return number < other.number;
  return name < other.name;
}



SqliteCompiledStatement::SqliteCompiledStatement(const char* func_name,
                                                 int func_number,
                                                 SqliteStatementCache& cache,
                                                 const char* sql) {
  statement_ = cache.GetStatement(func_name, func_number, sql);
}

SqliteCompiledStatement::~SqliteCompiledStatement() {
  
  if (statement_)
    statement_->reset();
}

SQLStatement& SqliteCompiledStatement::operator*() {
  DCHECK(statement_) << "Should check is_valid() before using the statement.";
  return *statement_;
}
SQLStatement* SqliteCompiledStatement::operator->() {
  DCHECK(statement_) << "Should check is_valid() before using the statement.";
  return statement_;
}
SQLStatement* SqliteCompiledStatement::statement() {
  DCHECK(statement_) << "Should check is_valid() before using the statement.";
  return statement_;
}
