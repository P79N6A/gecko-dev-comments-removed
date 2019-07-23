




































var gTestfile = 'regress-452329.js';

var BUGNUMBER = 452329;
var summary = 'Do not assert: *data->pc == JSOP_CALL || *data->pc == JSOP_NEW';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  this.__defineGetter__("x", "".match); if (x) 3;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
