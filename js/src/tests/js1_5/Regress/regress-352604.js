





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
 
  delete Function;
  var x = function () {};

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
