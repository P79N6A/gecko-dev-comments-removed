



#ifndef NESTED_CLASS_INLINE_CTOR_H_
#define NESTED_CLASS_INLINE_CTOR_H_

#include <string>
#include <vector>



class Foo {
  class Bar {
    Bar() {}
    ~Bar() {}

    std::vector<std::string> a;
  };
};

#endif  
