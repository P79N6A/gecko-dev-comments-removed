#define MOZ_STACK_CLASS __attribute__((annotate("moz_stack_class")))
#include <stddef.h>

struct MOZ_STACK_CLASS Stack {
  int i;
  void *operator new(size_t x) { return 0; }
  void *operator new(size_t blah, char *buffer) { return buffer; }
};

template <class T>
struct MOZ_STACK_CLASS TemplateClass {
  T i;
};

void gobble(void *) { }

void misuseStackClass(int len) {
  Stack valid;
  Stack alsoValid[2];
  static Stack notValid; 
  static Stack alsoNotValid[2]; 

  gobble(&valid);
  gobble(&notValid);
  gobble(&alsoValid[0]);

  gobble(new Stack); 
  gobble(new Stack[10]); 
  gobble(new TemplateClass<int>); 
  gobble(len <= 5 ? &valid : new Stack); 

  char buffer[sizeof(Stack)];
  gobble(new (buffer) Stack);
}

Stack notValid; 
struct RandomClass {
  Stack nonstaticMember; 
  static Stack staticMember; 
};
struct MOZ_STACK_CLASS RandomStackClass {
  Stack nonstaticMember;
  static Stack staticMember; 
};

struct BadInherit : Stack {}; 
struct MOZ_STACK_CLASS GoodInherit : Stack {};
