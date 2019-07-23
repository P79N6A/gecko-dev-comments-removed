




































var gTestfile = 'regress-373828.js';

var BUGNUMBER = 373828;
var summary = 'Do not assert: op == JSOP_LEAVEBLOCKEXPR ? ' + 
  'fp->spbase + OBJ_BLOCK_DEPTH(cx, obj) == sp - 1 : fp->spbase + OBJ_BLOCK_DEPTH(cx, obj) == sp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    for(let y in [5,6]) let([] = [1]) (function(){ }); d;
  }
  catch(ex)
  {
    print(ex + '');
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
