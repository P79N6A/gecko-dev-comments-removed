#define MOZ_NONHEAP_CLASS __attribute__((annotate("moz_nonheap_class")))
#define MOZ_STACK_CLASS __attribute__((annotate("moz_stack_class")))
#include <stddef.h>

struct MOZ_NONHEAP_CLASS NonHeap {
  int i;
  void *operator new(size_t x) throw() { return 0; }
  void *operator new(size_t blah, char *buffer) { return buffer; }
};

template <class T>
struct MOZ_NONHEAP_CLASS TemplateClass {
  T i;
};

void gobble(void *) { }

void misuseNonHeapClass(int len) {
  NonHeap valid;
  NonHeap alsoValid[2];
  static NonHeap validStatic;
  static NonHeap alsoValidStatic[2];

  gobble(&valid);
  gobble(&validStatic);
  gobble(&alsoValid[0]);

  gobble(new NonHeap); 
  gobble(new NonHeap[10]); 
  gobble(new TemplateClass<int>); 
  gobble(len <= 5 ? &valid : new NonHeap); 

  char buffer[sizeof(NonHeap)];
  gobble(new (buffer) NonHeap);
}

NonHeap validStatic;
struct RandomClass {
  NonHeap nonstaticMember; 
  static NonHeap staticMember;
};
struct MOZ_NONHEAP_CLASS RandomNonHeapClass {
  NonHeap nonstaticMember;
  static NonHeap staticMember;
};

struct BadInherit : NonHeap {}; 
struct MOZ_NONHEAP_CLASS GoodInherit : NonHeap {};

void useStuffWrongly() {
  gobble(new BadInherit); 
  gobble(new RandomClass); 
}


struct MOZ_STACK_CLASS StackClass {};
struct MOZ_NONHEAP_CLASS InferredStackClass : GoodInherit {
  NonHeap nonstaticMember;
  StackClass stackClass; 
};

InferredStackClass global; 
