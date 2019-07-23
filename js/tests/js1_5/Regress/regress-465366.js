




































var gTestfile = 'regress-465366.js';

var BUGNUMBER = 465366;
var summary = 'TM: JIT: error with multiplicative loop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  function f()
  {
    var k = 1;
    for (var n = 0; n < 2; n++) {
      k = (k * 10);
    }
    return k;
  }
  f();
  print(f());

  jit(false);
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
