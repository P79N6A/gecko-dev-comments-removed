





































var BUGNUMBER = 394709;
var summary = 'Do not leak with object.watch and closure';
var actual = 'No Leak';
var expect = 'No Leak';

if (typeof countHeap == 'undefined')
{
  countHeap = function () { 
    print('This test requires countHeap which is not supported'); 
    return 0;
  };
}


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  
  eval();

  runtest();
  gc();
  var count1 = countHeap();
  runtest();
  gc();
  var count2 = countHeap();
  runtest();
  gc();
  var count3 = countHeap();
  
  if (count1 < count2 && count2 < count3)
    throw "A leaky watch point is detected";

  function runtest () {
    var obj = { b: 0 };
    obj.watch('b', watcher);

    function watcher(id, old, value) {
      ++obj.n;
      return value;
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
