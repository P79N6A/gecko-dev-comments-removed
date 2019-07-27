#define MOZ_GLOBAL_CLASS __attribute__((annotate("moz_global_class")))
#include <stddef.h>

struct MOZ_GLOBAL_CLASS Global {
  int i;
  void *operator new(size_t x) throw() { return 0; }
  void *operator new(size_t blah, char *buffer) { return buffer; }
};

template <class T>
struct MOZ_GLOBAL_CLASS TemplateClass {
  T i;
};

void gobble(void *) { }

void misuseGlobalClass(int len) {
  Global notValid; 
  Global alsoNotValid[2]; 
  static Global valid; 
  static Global alsoValid[2]; 

  gobble(&valid);
  gobble(&notValid);
  gobble(&alsoValid[0]);

  gobble(new Global); 
  gobble(new Global[10]); 
  gobble(new TemplateClass<int>); 
  gobble(len <= 5 ? &valid : new Global); 

  char buffer[sizeof(Global)];
  gobble(new (buffer) Global); 
}

Global valid;
struct RandomClass {
  Global nonstaticMember; 
  static Global staticMember;
};
struct MOZ_GLOBAL_CLASS RandomGlobalClass {
  Global nonstaticMember;
  static Global staticMember;
};

struct BadInherit : Global {}; 
struct MOZ_GLOBAL_CLASS GoodInherit : Global {};

void misuseGlobalClassEvenMore(int len) {
  BadInherit moreInvalid; 
  RandomClass evenMoreInvalid; 
}
