




































var gTestfile = 'regress-354945-02.js';

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
 
  expect = 'TypeError: obj.__iterator__ returned a primitive value';
  var obj = {};
  obj.__iterator__ = function(){ };
  try
  {
    for(t in (obj)) { }
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
