




































var gTestfile = 'regress-352079.js';

var BUGNUMBER = 352079;
var summary = 'decompilation of various operators';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { f(let (y = 3) 4)++; }
  expect = 'function() { f(let (y = 3) 4)++; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { f(4)++; } 
  expect = 'function() { f(4)++; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { ++f(4); }
  expect = 'function() { ++f(4); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { delete(p(3)) } 
  expect = 'function() { p(3), true; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
