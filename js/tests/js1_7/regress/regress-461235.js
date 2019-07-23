




































var gTestfile = 'regress-461235.js';

var BUGNUMBER = 461235;
var summary = 'Do not assert: pos == GET_UINT16(pc)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  print(function() { [1 for (b in [])]; var c; });

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
