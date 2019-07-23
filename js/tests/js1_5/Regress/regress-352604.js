




































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
 
  delete Function; new Function("");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
