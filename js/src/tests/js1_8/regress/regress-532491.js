





var BUGNUMBER = 466128;
var summary = 'Do not assert: staticLevel == script->staticLevel, at ../jsobj.cpp';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(false);
  function f(foo) {
    if (a % 2 == 1) {
      try {
        eval(foo);
      } catch(e) {}
    }
  }
  a = 1;
  f("eval(\"x\")");
  f("x");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
