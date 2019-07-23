




































var gTestfile = 'regress-416737-01.js';

var BUGNUMBER = 416737;
var summary = 'Do not assert: *pc == JSOP_GETARG';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  (function() { (function([]){ function n(){} })(1) });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

