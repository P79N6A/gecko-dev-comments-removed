




































var gTestfile = 'regress-353249.js';

var BUGNUMBER = 353249;
var summary = 'regression test for bug 353249';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function () { let (x) <x/>.(1) < let (z) eval('3');
	     for (x in this) {} });

  expect = 'function () { (let (x) <x/>.((1)) < (let (z) eval("3"))); ' +
    'for (x in this) {} }';
  actual = f + '';
  compareSource(expect, actual, summary);

  
  f();
  exitFunc ('test');
}
