




































var gTestfile = 'regress-381963-02.js';

var BUGNUMBER = 381963;
var summary = 'Decompilation of genexp in |while|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function f() { while (false) { print(2); } }';

  function f() { while (0 && (b for (x in 3))) print(2) }

  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
