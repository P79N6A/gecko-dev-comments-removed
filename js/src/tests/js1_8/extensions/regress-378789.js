




































var gTestfile = 'regress-378789.js';

var BUGNUMBER = 378789;
var summary = 'js_PutEscapedString should handle nulls';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof dumpHeap == 'undefined')
  {
    print('dumpHeap not supported');
  }
  else
  {
    dumpHeap(null, [ "a\0b" ], null, 1);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
