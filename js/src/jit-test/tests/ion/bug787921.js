
function TestCase(n, d, e, a) {
  this.bugnumber = typeof(BUGNUMER) != 'undefined' ? BUGNUMBER : '';
  this.type = (typeof window == 'undefined' ? 'shell' : 'browser');
  gTestcases[gTc++] = this;
  if (optionName) {}
  {} {} {} 
}
function f() {}
function g(n, h) {
    var t = g(TestCase.toSource());
}
g(80, f);
