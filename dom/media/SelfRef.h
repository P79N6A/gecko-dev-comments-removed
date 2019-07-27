





#ifndef SELF_REF_H
#define SELF_REF_H

#include "mozilla/Attributes.h"

namespace mozilla {

template<class T>
class SelfReference {
public:
  SelfReference() : mHeld(false) {}
  ~SelfReference()
  {
    NS_ASSERTION(!mHeld, "Forgot to drop the self reference?");
  }

  void Take(T* t)
  {
    if (!mHeld) {
      mHeld = true;
      t->AddRef();
    }
  }
  void Drop(T* t)
  {
    if (mHeld) {
      mHeld = false;
      t->Release();
    }
  }

  operator bool() const { return mHeld; }

  SelfReference(const SelfReference& aOther) MOZ_DELETE;
  void operator=(const SelfReference& aOther) MOZ_DELETE;
private:
  bool mHeld;
};
} 

#endif 
