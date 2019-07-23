




































var gTestfile = 'regress-375801.js';

var BUGNUMBER = 375801;
var summary = 'uneval should use "(void 0)" instead of "undefined"';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '({a: (void 0)})'
  actual = uneval({a: undefined})
  compareSource(expect, actual, summary + ': uneval');

  expect = 'function() {({a: undefined});}';
  actual = (function() {({a: undefined});}).toString();
  compareSource(expect, actual, summary + ': toString');

  expect = '(function () {({a: undefined});})';
  actual = (function () {({a: undefined});}).toSource();
  compareSource(expect, actual, summary + ': toSource');

  exitFunc ('test');
}
