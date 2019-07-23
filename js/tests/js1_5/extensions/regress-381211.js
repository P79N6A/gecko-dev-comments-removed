




































var gTestfile = 'regress-381211.js';


var BUGNUMBER = 381211;
var summary = 'uneval with getter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '( { get x() {} } )';
  actual = uneval({get x(){}});
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
