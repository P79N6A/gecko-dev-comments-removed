




































#ifndef nsDOMMemoryReporter_h__
#define nsDOMMemoryReporter_h__

#include "nsIMemoryReporter.h"





namespace mozilla {
  namespace dom {
    namespace MemoryReporter {
      






      template <class TypeCurrent, class TypeParent>
      inline PRInt64 GetBasicSize(const TypeCurrent* const obj) {
        return obj->TypeParent::SizeOf() - sizeof(TypeParent)
                                         + sizeof(TypeCurrent);
      }
    }
  }
}




#define NS_DECL_DOM_MEMORY_REPORTER_SIZEOF  \
  virtual PRInt64 SizeOf() const;

#define NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(TypeCurrent, TypeParent) \
  virtual PRInt64 SizeOf() const {                                           \
    return mozilla::dom::MemoryReporter::GetBasicSize<TypeCurrent,           \
                                                      TypeParent>(this);     \
  }

class nsDOMMemoryReporter: public nsIMemoryReporter {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  static void Init();

private:
  
  nsDOMMemoryReporter();
};

#endif 

