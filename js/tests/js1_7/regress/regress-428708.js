




































var gTestfile = 'regress-428708.js';

var BUGNUMBER = 428708;
var summary = 'Do not assert: OBJ_BLOCK_COUNT(cx, obj) == 1';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  let ([] = <x/>.(1)) { let a; let b; }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
