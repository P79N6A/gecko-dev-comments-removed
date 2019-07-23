




































var gTestfile = 'regress-352873-02.js';

var BUGNUMBER = 352873;
var summary = 'decompilation of nested |try...catch| with |with|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function(){ try { try { } finally { return; } } finally { with({}) { } }}
  expect = 'function(){ try { try { } finally { return; } } finally { with({}) { } }}';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = actual = 'No Crash';
  f();
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
