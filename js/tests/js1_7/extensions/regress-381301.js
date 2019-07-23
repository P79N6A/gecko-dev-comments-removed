




































var gTestfile = 'regress-381301.js';


var BUGNUMBER = 381301;
var summary = 'uneval of object with native-function getter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '( { get x print ( ) { [ native code ] } } )';
  actual =  uneval({x getter: print});

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
