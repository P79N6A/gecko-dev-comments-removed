





#ifndef CHROME_COMMON_NET_COOKIE_MONSTER_SQLITE_H__
#define CHROME_COMMON_NET_COOKIE_MONSTER_SQLITE_H__

#include <string>
#include <vector>

#include "base/ref_counted.h"
#include "base/message_loop.h"
#include "chrome/browser/meta_table_helper.h"
#include "net/base/cookie_monster.h"

struct sqlite3;

class SQLitePersistentCookieStore
    : public net::CookieMonster::PersistentCookieStore {
 public:
  SQLitePersistentCookieStore(const std::wstring& path,
                              MessageLoop* background_loop);
  ~SQLitePersistentCookieStore();

  virtual bool Load(std::vector<net::CookieMonster::KeyedCanonicalCookie>*);

  virtual void AddCookie(const std::string&,
                         const net::CookieMonster::CanonicalCookie&);
  virtual void UpdateCookieAccessTime(
      const net::CookieMonster::CanonicalCookie&);
  virtual void DeleteCookie(const net::CookieMonster::CanonicalCookie&);

 private:
  class Backend;

  
  bool EnsureDatabaseVersion(sqlite3* db);

  std::wstring path_;
  scoped_refptr<Backend> backend_;

  
  MessageLoop* background_loop_;

  MetaTableHelper meta_table_;

  DISALLOW_EVIL_CONSTRUCTORS(SQLitePersistentCookieStore);
};

#endif  
