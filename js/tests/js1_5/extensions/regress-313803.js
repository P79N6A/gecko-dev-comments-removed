




































var gTestfile = 'regress-313803.js';

var BUGNUMBER = 313803;
var summary = 'uneval() on func with embedded objects with getter or setter';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var func = function ff() {
  obj = { get foo() { return "foo"; }};
  return 1;
};

actual = uneval(func);

expect = '(function ff() {obj = {get foo () {return "foo";}};return 1;})';

compareSource(expect, actual, summary);
