#define MOZ_IMPLICIT __attribute__((annotate("moz_implicit")))

struct Bad {
  operator bool(); 
};
struct Good {
  explicit operator bool();
};
struct Okay {
  MOZ_IMPLICIT operator bool();
};
