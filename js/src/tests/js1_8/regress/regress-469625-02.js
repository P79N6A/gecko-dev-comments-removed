




































var gTestfile = 'regress-469625-02.js';

var BUGNUMBER = 469625;
var summary = 'group assignment with rhs containing holes';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'y';

  Array.prototype[1] = 'y';
  var [x, y, z] = ['x', , 'z'];

  actual = y;
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
