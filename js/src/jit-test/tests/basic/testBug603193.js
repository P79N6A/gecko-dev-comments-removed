function g(code) {
  f = Function(code);
  for (a in f()) {}
}


g()
g("(function(){})")
g()


g("yield")
