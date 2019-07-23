




































var gTestfile = 'regress-350793-02.js';

var BUGNUMBER = 350793;
var summary = 'for-in loops must be yieldable';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var gen = function() { for(let y in [5,6,7,8]) yield ({}); };
  for (let it in gen())
    ;
  gc();

  reportCompare(expect, actual, summary);

  expect = 'function() { for(let y in [5,6,7,8]) { yield {}; }}';
  actual = gen + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
