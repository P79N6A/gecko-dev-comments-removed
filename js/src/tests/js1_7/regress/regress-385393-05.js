






var BUGNUMBER = 385393;
var summary = 'Regression test for bug 385393';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function c(gen)
  {
    Iterator;    

    "" + gen;

    for (var i in gen())
      ;
  }

  function gen()
  {
    ({}).hasOwnProperty();
    yield;
  }

  c(gen);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
