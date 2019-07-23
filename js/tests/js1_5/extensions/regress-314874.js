




































var gTestfile = 'regress-314874.js';

var BUGNUMBER = 314874;
var summary = 'Function.call/apply with non-primitive argument';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var thisArg = { valueOf: function() { return {a: 'a', b: 'b'}; } };

  var f = function () { return (this); };

  expect  = f.call(thisArg);

  thisArg.f = f;

  actual = thisArg.f();

  delete thisArg.f;

  expect = expect.toSource();
  actual = actual.toSource();
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
