




































var gTestfile = 'regress-379925.js';


var BUGNUMBER = 379925;
var summary = 'Do not Assert: pc[oplen] == JSOP_POP || pc[oplen] == JSOP_SETSP';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { for(var [x, x] = [,,] in []); };
  expect = 'function() { var [x, x] = [,,]; for( [x, x] in []) {} }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
