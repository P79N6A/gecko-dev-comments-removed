




































var gTestfile = 'regress-352613-01.js';

var BUGNUMBER = 352613;
var summary = 'decompilation of |switch| |case| with computed value';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function () { switch(8) { case  1: a; case ('fafafa'.replace(/a/g, [1,2,3,4].map)): b; } }
  expect = 'function () { switch(8) { case  1: a; case "fafafa".replace(/a/g, [1,2,3,4].map): b; default:;} }';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = 'TypeError: "a" is not a function';
  try
  {
    f();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
