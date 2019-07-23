




































var gTestfile = 'regress-472450-02.js';

var BUGNUMBER = 472450;
var summary = 'TM: Do not assert: StackBase(fp) + blockDepth == regs.sp';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for each (let x in [function(){}, {}, {}, function(){}, function(){}]) { ([<y/>
                                                          for (y in x)]); }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
