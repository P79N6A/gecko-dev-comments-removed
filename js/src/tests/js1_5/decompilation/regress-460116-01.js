




































var gTestfile = 'regress-460116-01.js';

var BUGNUMBER = 460116;
var summary = 'Condition propagation folding constants';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = (function (){if(typeof(false||undef))throw "fail"});
  expect = 'function (){if(typeof(false||undef)) {throw "fail";}}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
