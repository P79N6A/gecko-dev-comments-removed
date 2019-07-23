




































var gTestfile = 'regress-352797-02.js';

var BUGNUMBER = 352797;
var summary = 'Assertion: OBJ_GET_CLASS(cx, obj) == &js_BlockClass';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { let (x = eval.call(<x/>.(1), "")) {} })();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
