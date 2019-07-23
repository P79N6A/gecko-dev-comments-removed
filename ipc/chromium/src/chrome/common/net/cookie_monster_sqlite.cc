



#include "chrome/common/net/cookie_monster_sqlite.h"

#include <list>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/ref_counted.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "chrome/common/sqlite_compiled_statement.h"
#include "chrome/common/sqlite_utils.h"

using base::Time;



class SQLitePersistentCookieStore::Backend
    : public base::RefCountedThreadSafe<SQLitePersistentCookieStore::Backend> {
 public:
  
  
  explicit Backend(sqlite3* db, MessageLoop* loop)
      : db_(db),
        background_loop_(loop),
        cache_(new SqliteStatementCache(db)),
        num_pending_(0) {
    DCHECK(db_) << "Database must exist.";
  }

  
  ~Backend() {
    DCHECK(!db_) << "Close should have already been called.";
    DCHECK(num_pending_ == 0 && pending_.empty());
  }

  
  void AddCookie(const std::string& key,
                 const net::CookieMonster::CanonicalCookie& cc);

  
  void UpdateCookieAccessTime(const net::CookieMonster::CanonicalCookie& cc);

  
  void DeleteCookie(const net::CookieMonster::CanonicalCookie& cc);

  
  
  void Close();

 private:
  class PendingOperation {
   public:
    typedef enum {
      COOKIE_ADD,
      COOKIE_UPDATEACCESS,
      COOKIE_DELETE,
    } OperationType;

    PendingOperation(OperationType op,
                     const std::string& key,
                     const net::CookieMonster::CanonicalCookie& cc)
        : op_(op), key_(key), cc_(cc) { }

    OperationType op() const { return op_; }
    const std::string& key() const { return key_; }
    const net::CookieMonster::CanonicalCookie& cc() const { return cc_; }

   private:
    OperationType op_;
    std::string key_;  
    net::CookieMonster::CanonicalCookie cc_;
  };

 private:
  
  void BatchOperation(PendingOperation::OperationType op,
                      const std::string& key,
                      const net::CookieMonster::CanonicalCookie& cc);
  
  void Commit();
  
  void InternalBackgroundClose();

  sqlite3* db_;
  MessageLoop* background_loop_;
  SqliteStatementCache* cache_;

  typedef std::list<PendingOperation*> PendingOperationsList;
  PendingOperationsList pending_;
  PendingOperationsList::size_type num_pending_;
  Lock pending_lock_;  

  DISALLOW_EVIL_CONSTRUCTORS(Backend);
};

void SQLitePersistentCookieStore::Backend::AddCookie(
    const std::string& key,
    const net::CookieMonster::CanonicalCookie& cc) {
  BatchOperation(PendingOperation::COOKIE_ADD, key, cc);
}

void SQLitePersistentCookieStore::Backend::UpdateCookieAccessTime(
    const net::CookieMonster::CanonicalCookie& cc) {
  BatchOperation(PendingOperation::COOKIE_UPDATEACCESS, std::string(), cc);
}

void SQLitePersistentCookieStore::Backend::DeleteCookie(
    const net::CookieMonster::CanonicalCookie& cc) {
  BatchOperation(PendingOperation::COOKIE_DELETE, std::string(), cc);
}

void SQLitePersistentCookieStore::Backend::BatchOperation(
    PendingOperation::OperationType op,
    const std::string& key,
    const net::CookieMonster::CanonicalCookie& cc) {
  
  static const int kCommitIntervalMs = 30 * 1000;
  
  static const size_t kCommitAfterBatchSize = 512;
  DCHECK(MessageLoop::current() != background_loop_);

  
  scoped_ptr<PendingOperation> po(new PendingOperation(op, key, cc));
  CHECK(po.get());

  PendingOperationsList::size_type num_pending;
  {
    AutoLock locked(pending_lock_);
    pending_.push_back(po.release());
    num_pending = ++num_pending_;
  }

  
  if (num_pending == 1) {
    
    background_loop_->PostDelayedTask(FROM_HERE,
        NewRunnableMethod(this, &Backend::Commit), kCommitIntervalMs);
  } else if (num_pending == kCommitAfterBatchSize) {
    
    background_loop_->PostTask(FROM_HERE,
        NewRunnableMethod(this, &Backend::Commit));
  }
}

void SQLitePersistentCookieStore::Backend::Commit() {
  DCHECK(MessageLoop::current() == background_loop_);
  PendingOperationsList ops;
  {
    AutoLock locked(pending_lock_);
    pending_.swap(ops);
    num_pending_ = 0;
  }

  
  if (!db_ || ops.empty())
    return;

  SQLITE_UNIQUE_STATEMENT(add_smt, *cache_,
      "INSERT INTO cookies (creation_utc, host_key, name, value, path, "
      "expires_utc, secure, httponly, last_access_utc) "
      "VALUES (?,?,?,?,?,?,?,?,?)");
  if (!add_smt.is_valid()) {
    NOTREACHED();
    return;
  }

  SQLITE_UNIQUE_STATEMENT(update_access_smt, *cache_,
                          "UPDATE cookies SET last_access_utc=? "
                          "WHERE creation_utc=?");
  if (!update_access_smt.is_valid()) {
    NOTREACHED();
    return;
  }

  SQLITE_UNIQUE_STATEMENT(del_smt, *cache_,
                          "DELETE FROM cookies WHERE creation_utc=?");
  if (!del_smt.is_valid()) {
    NOTREACHED();
    return;
  }

  SQLTransaction transaction(db_);
  transaction.Begin();
  for (PendingOperationsList::iterator it = ops.begin();
       it != ops.end(); ++it) {
    
    scoped_ptr<PendingOperation> po(*it);
    switch (po->op()) {
      case PendingOperation::COOKIE_ADD:
        add_smt->reset();
        add_smt->bind_int64(0, po->cc().CreationDate().ToInternalValue());
        add_smt->bind_string(1, po->key());
        add_smt->bind_string(2, po->cc().Name());
        add_smt->bind_string(3, po->cc().Value());
        add_smt->bind_string(4, po->cc().Path());
        add_smt->bind_int64(5, po->cc().ExpiryDate().ToInternalValue());
        add_smt->bind_int(6, po->cc().IsSecure());
        add_smt->bind_int(7, po->cc().IsHttpOnly());
        add_smt->bind_int64(8, po->cc().LastAccessDate().ToInternalValue());
        if (add_smt->step() != SQLITE_DONE) {
          NOTREACHED() << "Could not add a cookie to the DB.";
        }
        break;

      case PendingOperation::COOKIE_UPDATEACCESS:
        update_access_smt->reset();
        update_access_smt->bind_int64(0,
            po->cc().LastAccessDate().ToInternalValue());
        update_access_smt->bind_int64(1,
            po->cc().CreationDate().ToInternalValue());
        if (update_access_smt->step() != SQLITE_DONE) {
          NOTREACHED() << "Could not update cookie last access time in the DB.";
        }
        break;

      case PendingOperation::COOKIE_DELETE:
        del_smt->reset();
        del_smt->bind_int64(0, po->cc().CreationDate().ToInternalValue());
        if (del_smt->step() != SQLITE_DONE) {
          NOTREACHED() << "Could not delete a cookie from the DB.";
        }
        break;

      default:
        NOTREACHED();
        break;
    }
  }
  transaction.Commit();
}




void SQLitePersistentCookieStore::Backend::Close() {
  DCHECK(MessageLoop::current() != background_loop_);
  
  
  background_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &Backend::InternalBackgroundClose));
}

void SQLitePersistentCookieStore::Backend::InternalBackgroundClose() {
  DCHECK(MessageLoop::current() == background_loop_);
  
  Commit();
  
  delete cache_;
  cache_ = NULL;
  sqlite3_close(db_);
  db_ = NULL;
}

SQLitePersistentCookieStore::SQLitePersistentCookieStore(
    const std::wstring& path, MessageLoop* background_loop)
    : path_(path),
      background_loop_(background_loop) {
  DCHECK(background_loop) << "SQLitePersistentCookieStore needs a MessageLoop";
}

SQLitePersistentCookieStore::~SQLitePersistentCookieStore() {
  if (backend_.get()) {
    backend_->Close();
    
    
    backend_ = NULL;
  }
}


static const int kCurrentVersionNumber = 3;
static const int kCompatibleVersionNumber = 3;

namespace {


bool InitTable(sqlite3* db) {
  if (!DoesSqliteTableExist(db, "cookies")) {
    if (sqlite3_exec(db, "CREATE TABLE cookies ("
                         "creation_utc INTEGER NOT NULL UNIQUE PRIMARY KEY,"
                         "host_key TEXT NOT NULL,"
                         "name TEXT NOT NULL,"
                         "value TEXT NOT NULL,"
                         "path TEXT NOT NULL,"
                         
                         "expires_utc INTEGER NOT NULL,"
                         "secure INTEGER NOT NULL,"
                         "httponly INTEGER NOT NULL,"
                         "last_access_utc INTEGER NOT NULL)",
                         NULL, NULL, NULL) != SQLITE_OK)
      return false;
  }

  
  
  sqlite3_exec(db, "CREATE INDEX cookie_times ON cookies (creation_utc)",
               NULL, NULL, NULL);

  return true;
}

void PrimeCache(sqlite3* db) {
  
  
  
  SQLStatement dummy;
  if (dummy.prepare(db, "SELECT * from meta") != SQLITE_OK)
    return;
  if (dummy.step() != SQLITE_ROW)
    return;

  sqlite3Preload(db);
}

}  

bool SQLitePersistentCookieStore::Load(
    std::vector<net::CookieMonster::KeyedCanonicalCookie>* cookies) {
  DCHECK(!path_.empty());
  sqlite3* db;
  if (sqlite3_open(WideToUTF8(path_).c_str(), &db) != SQLITE_OK) {
    NOTREACHED() << "Unable to open cookie DB.";
    return false;
  }

  if (!EnsureDatabaseVersion(db) || !InitTable(db)) {
    NOTREACHED() << "Unable to initialize cookie DB.";
    sqlite3_close(db);
    return false;
  }

  PrimeCache(db);

  
  SQLStatement smt;
  if (smt.prepare(db,
      "SELECT creation_utc, host_key, name, value, path, expires_utc, secure, "
      "httponly, last_access_utc FROM cookies") != SQLITE_OK) {
    NOTREACHED() << "select statement prep failed";
    sqlite3_close(db);
    return false;
  }

  while (smt.step() == SQLITE_ROW) {
    std::string key = smt.column_string(1);
    scoped_ptr<net::CookieMonster::CanonicalCookie> cc(
        new net::CookieMonster::CanonicalCookie(
            smt.column_string(2),                            
            smt.column_string(3),                            
            smt.column_string(4),                            
            smt.column_int(6) != 0,                          
            smt.column_int(7) != 0,                          
            Time::FromInternalValue(smt.column_int64(0)),    
            Time::FromInternalValue(smt.column_int64(8)),    
            true,                                            
            Time::FromInternalValue(smt.column_int64(5))));  
    
    if (!cc.get())
      break;

    DLOG_IF(WARNING,
            cc->CreationDate() > Time::Now()) << L"CreationDate too recent";
    cookies->push_back(
        net::CookieMonster::KeyedCanonicalCookie(smt.column_string(1),
                                                 cc.release()));
  }

  
  backend_ = new Backend(db, background_loop_);

  return true;
}

bool SQLitePersistentCookieStore::EnsureDatabaseVersion(sqlite3* db) {
  
  if (!meta_table_.Init(std::string(), kCurrentVersionNumber,
                        kCompatibleVersionNumber, db))
    return false;

  if (meta_table_.GetCompatibleVersionNumber() > kCurrentVersionNumber) {
    LOG(WARNING) << "Cookie database is too new.";
    return false;
  }

  int cur_version = meta_table_.GetVersionNumber();
  if (cur_version == 2) {
    SQLTransaction transaction(db);
    transaction.Begin();
    if ((sqlite3_exec(db,
                      "ALTER TABLE cookies ADD COLUMN last_access_utc "
                      "INTEGER DEFAULT 0", NULL, NULL, NULL) != SQLITE_OK) ||
        (sqlite3_exec(db,
                      "UPDATE cookies SET last_access_utc = creation_utc",
                      NULL, NULL, NULL) != SQLITE_OK)) {
      LOG(WARNING) << "Unable to update cookie database to version 3.";
      return false;
    }
    ++cur_version;
    meta_table_.SetVersionNumber(cur_version);
    meta_table_.SetCompatibleVersionNumber(
        std::min(cur_version, kCompatibleVersionNumber));
    transaction.Commit();
  }

  

  
  
  LOG_IF(WARNING, cur_version < kCurrentVersionNumber) <<
      "Cookie database version " << cur_version << " is too old to handle.";

  return true;
}

void SQLitePersistentCookieStore::AddCookie(
    const std::string& key,
    const net::CookieMonster::CanonicalCookie& cc) {
  if (backend_.get())
    backend_->AddCookie(key, cc);
}

void SQLitePersistentCookieStore::UpdateCookieAccessTime(
    const net::CookieMonster::CanonicalCookie& cc) {
  if (backend_.get())
    backend_->UpdateCookieAccessTime(cc);
}

void SQLitePersistentCookieStore::DeleteCookie(
    const net::CookieMonster::CanonicalCookie& cc) {
  if (backend_.get())
    backend_->DeleteCookie(cc);
}
