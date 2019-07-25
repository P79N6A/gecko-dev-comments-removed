




#ifndef MOZILLA_GFX_USERDATA_H_
#define MOZILLA_GFX_USERDATA_H_

#include <stdlib.h>
#include "Types.h"
#include "mozilla/Assertions.h"

namespace mozilla {
namespace gfx {

struct UserDataKey {
  int unused;
};


class UserData
{
  typedef void (*destroyFunc)(void *data);
public:
  UserData() : count(0), entries(nullptr) {}

  
  void Add(UserDataKey *key, void *userData, destroyFunc destroy)
  {
    for (int i=0; i<count; i++) {
      if (key == entries[i].key) {
        if (entries[i].destroy) {
          entries[i].destroy(entries[i].userData);
        }
        entries[i].userData = userData;
        entries[i].destroy = destroy;
        return;
      }
    }

    
    
    
    
    entries = static_cast<Entry*>(realloc(entries, sizeof(Entry)*(count+1)));

    if (!entries) {
      MOZ_CRASH();
    }

    entries[count].key      = key;
    entries[count].userData = userData;
    entries[count].destroy  = destroy;

    count++;
  }

  
  void* Remove(UserDataKey *key)
  {
    for (int i=0; i<count; i++) {
      if (key == entries[i].key) {
        void *userData = entries[i].userData;
        
        --count;
        for (;i<count; i++) {
          entries[i] = entries[i+1];
        }
        return userData;
      }
    }
    return nullptr;
  }

  
  void *Get(UserDataKey *key)
  {
    for (int i=0; i<count; i++) {
      if (key == entries[i].key) {
        return entries[i].userData;
      }
    }
    return nullptr;
  }

  bool Has(UserDataKey *key)
  {
    for (int i=0; i<count; i++) {
      if (key == entries[i].key) {
        return true;
      }
    }
    return false;
  }

  void Destroy()
  {
    for (int i=0; i<count; i++) {
      if (entries[i].destroy) {
        entries[i].destroy(entries[i].userData);
      }
    }
    free(entries);
    entries = nullptr;
    count = 0;
  }

  ~UserData()
  {
    Destroy();
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

#endif 
