































namespace google_breakpad {

class C {
 public:
  C() : member_(1) {}
  virtual ~C() {}

  void set_member(int value) { member_ = value; }
  int member() const { return member_; }

  void f() { member_ = g(); }
  virtual int g() { return 2; }
  static char* h(const C &that) { return 0; }

 private:
  int member_;
};

static int i() {
  return 3;
}

}  

int main(int argc, char **argv) {
  google_breakpad::C object;
  object.set_member(google_breakpad::i());
  object.f();
  int value = object.g();
  char *nothing = object.h(object);

  return 0;
}
