






var BUGNUMBER = 394709;
var summary = 'Do not leak with object.watch and closure';
var actual = 'No Leak';
var expect = 'No Leak';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  assertEq(finalizeCount(), 0, "invalid initial state");

  runtest();
  gc();
  assertEq(finalizeCount(), 1, "leaked");

  runtest();
  gc();
  assertEq(finalizeCount(), 2, "leaked");

  runtest();
  gc();
  assertEq(finalizeCount(), 3, "leaked");


  function runtest () {
    var obj = { b: makeFinalizeObserver() };
    obj.watch('b', watcher);

    function watcher(id, old, value) {
      ++obj.n;
      return value;
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
