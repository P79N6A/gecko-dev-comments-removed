




#ifndef JSSTREAMWRITER_H
#define JSSTREAMWRITER_H

#include <ostream>
#include <stdlib.h>
#include "nsDeque.h"

class JSStreamWriter
{
public:
  explicit JSStreamWriter(std::ostream& aStream);
  ~JSStreamWriter();

  void BeginObject();
  void EndObject();
  void BeginArray();
  void EndArray();

  
  
  
  void BeginBareList();
  void EndBareList();

  
  
  void SpliceArrayElements(const char* aElements);

  void Name(const char *name);
  void Value(int value);
  void Value(unsigned value);
  void Value(double value);
  void Value(const char *value, size_t valueLength);
  void Value(const char *value);
  template <typename T>
  void NameValue(const char *aName, T aValue)
  {
    Name(aName);
    Value(aValue);
  }

private:
  std::ostream& mStream;
  bool mNeedsComma;
  bool mNeedsName;

  nsDeque mStack;

  
  JSStreamWriter(const JSStreamWriter&);
  JSStreamWriter& operator=(const JSStreamWriter&);

  void* operator new(size_t);
  void* operator new[](size_t);
  void operator delete(void*) {
    
    
    
    
    abort();
  }
  void operator delete[](void*);
};

#endif
