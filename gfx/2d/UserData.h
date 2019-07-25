





































#include <stdlib.h>

namespace mozilla {
namespace gfx {

struct UserDataKey {
  int unused;
};


class UserData
{
  typedef void (*destroyFunc)(void *data);
public:
  UserData() : count(0), entries(NULL) {}

  
  void Add(UserDataKey *key, void *userData, destroyFunc destroy)
  {
    
    
    
    
    entries = static_cast<Entry*>(moz_xrealloc(entries, sizeof(Entry)*(count+1)));

    entries[count].key      = key;
    entries[count].userData = userData;
    entries[count].destroy  = destroy;

    count++;
  }

  

  
  void *Get(UserDataKey *key)
  {
    for (int i=0; i<count; i++) {
      if (key == entries[i].key) {
        return entries[i].userData;
      }
    }
    return NULL;
  }

  ~UserData()
  {
    for (int i=0; i<count; i++) {
      entries[i].destroy(entries[i].userData);
    }
    free(entries);
  }

private:
  struct Entry {
    const UserDataKey *key;
    void *userData;
    destroyFunc destroy;
  };

  int count;
  Entry *entries;

};

}
}


