




































var gTestfile = 'simple-fib.js';

var BUGNUMBER     = 326466;  
var summary = "Simple Fibonacci iterator";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





function fib()
{
  var a = 0, b = 1;
  while (true)
  {
    yield a;
    var t = a;
    a = b;
    b += t;
  }
}

var failed = false;

try
{
  var g = fib();

  if (g.next() != 0)
    throw "F_0 = 0";
  if (g.next() != 1)
    throw "F_1 = 1";
  if (g.next() != 1)
    throw "F_2 = 1";
  if (g.next() != 2)
    throw "F_3 = 2";
}
catch (e)
{
  failed = e;
}



expect = false;
actual = failed;

reportCompare(expect, actual, summary);
