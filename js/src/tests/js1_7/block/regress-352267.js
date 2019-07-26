





var BUGNUMBER = 352267;
var summary = 'Do not assert with |if|, block, |let|';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  uneval(function() { if (y) { { let set = 4.; } } else if ([1,2,3]) { } });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
