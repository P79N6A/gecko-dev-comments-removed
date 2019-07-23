




































var gTestfile = 'regress-354945-01.js';

var BUGNUMBER = 354945;
var summary = 'Do not crash with new Iterator';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj = {};
  obj.__iterator__ = function(){ };
  for(t in (new Iterator(obj))) { }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
