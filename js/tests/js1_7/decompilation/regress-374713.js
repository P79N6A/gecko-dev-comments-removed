




































var gTestfile = 'regress-374713.js';

var BUGNUMBER = 374713;
var summary = 'Do not assert decompiling function() { try { } catch([]) { } }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function() { try { } catch([]) { } };
  expect = 'function() { try { } catch([]) { } }';
  actual = f + '';
 
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
