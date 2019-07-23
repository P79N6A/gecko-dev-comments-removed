




































var gTestfile = 'regress-344052.js';

var BUGNUMBER = 344052;
var summary = 'Function prototype - simple shared property';
var actual = '';
var expect = 'true';

Function.prototype.foo = true;
function y(){};


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  actual = String(y.foo);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
