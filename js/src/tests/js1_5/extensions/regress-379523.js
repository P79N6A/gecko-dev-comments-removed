




































var gTestfile = 'regress-379523.js';

var BUGNUMBER = 379523;
var summary = 'Decompilation of sharp declaration';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function () { return #1=[a]; });
  
  expect = '(function () { return #1=[a]; })';
  actual = f.toSource();

  compareSource(expect, actual, summary + ': 1');

  f = (function () { return #1={a:b}; });

  expect = '(function () { return #1={a:b}; })';
  actual = f.toSource();

  compareSource(expect, actual, summary + ': 1');

  exitFunc ('test');
}
