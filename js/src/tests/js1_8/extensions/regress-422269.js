





































var BUGNUMBER = 422269;
var summary = 'Compile-time let block should not capture runtime references';
var actual = 'No leak';
var expect = 'No leak';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function f()
  {
    let m = {sin: Math.sin};
    (function() { m.sin(1); })();
    return m;
  }

  if (typeof countHeap == 'undefined')
  {
    expect = actual = 'Test skipped';
    print('Test skipped. Requires countHeap function.');
  }
  else
  {
    var x = f();
    gc();
    var n = countHeap();
    x = null;
    
    
    
    eval("");
    gc();

    var n2 = countHeap();
    if (n2 >= n)
      actual = "leak is detected, something roots the result of f";
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
