




































var gTestfile = 'regress-352604.js';

var BUGNUMBER = 352604;
var summary = 'Do not assert: !OBJ_GET_PROTO(cx, ctor)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() {}
  delete Function;
  var g = new Function('');

  expect = f.__proto__;
  actual = g.__proto__;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
