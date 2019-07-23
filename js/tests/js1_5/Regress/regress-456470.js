




































var gTestfile = 'regress-456470.js';

var BUGNUMBER = 456470;
var summary = 'TM: Make sure JSOP_DEFLOCALFUN pushes the right function object.';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  function x() {
    function a() {
      return true;
    }
    return a();
  }

  for (var i = 0; i < 10; ++i)
    x();

  jit(false);
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
