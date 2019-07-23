




































var gTestfile = 'regress-367629.js';


var BUGNUMBER = 367629;
var summary = 'Decompilation of result with native function as getter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var a = {}; 
  a.h getter = encodeURI; 

  expect = '({get h () {[native code]})';
  actual = uneval(a);      

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
