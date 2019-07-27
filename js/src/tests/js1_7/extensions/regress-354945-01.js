





var BUGNUMBER = 354945;
var summary = 'Do not crash with new Iterator';
var expect = 'TypeError: trap __iterator__ for ({__iterator__:(function (){ })}) returned a primitive value';
var actual;



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try {
    var obj = {};
    obj.__iterator__ = function(){ };
    for(t in (new Iterator(obj))) { }
  } catch (ex) {
    actual = ex.toString();
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
