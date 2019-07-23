




































var gTestfile = 'regress-350621.js';

var BUGNUMBER = 350621;
var summary = 'for-in loops over generator objects';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var LOOPS = 500;

  function gen1() {
    for (var a = 1; a <= LOOPS; ++a)
      yield;
  }

  function gen2() {
    for (var b in gen1())
      yield;
  }

  function test_it(RUNS) {
    for (var c = 1; c <= RUNS; ++c) {
      var count = 0;
      for (var d in gen2()) {
        
        
        Object("x");
        ++count;
      }
      if (count != LOOPS) {
        print("Test run " + c + ": test failed, count = " + count +
	      ", should be " + LOOPS);
        var failed = true;
      }
    }
    actual = !failed;
    if (!failed)
    {
      print("Test passed.");
    }
  }

  expect = true;
  test_it(20);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
