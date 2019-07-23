






var gTestfile = 'regress-469625.js';

var BUGNUMBER = 469625;
var summary = 'TM: Array prototype and expression closures';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'TypeError: [] is not a function';

  jit(true);

  Array.prototype.__proto__ = function () 3; 

  try
  {
    [].__proto__();
  }
  catch(ex)
  {
    print(actual = ex + '');
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
