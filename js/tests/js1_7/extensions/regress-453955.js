




































var gTestfile = 'regress-453955.js';

var BUGNUMBER = 453955;
var summary = 'Do not assert: sprop->setter != js_watch_set || pobj != obj';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var z = 0; z < 2; ++z) 
  { 
    [].filter.watch("9", function(y) { yield y; });
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
