




































var gTestfile = 'regress-344139.js';

var BUGNUMBER     = "344139";
var summary = "Basic let functionality";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

var x = 7;

function f1()
{
  let x = 5;
  return x;
}

function f2()
{
  let x = 5;
  x = 3;
  return x;
}

function f3()
{
  let x = 5;
  x += x;
  return x;
}

function f4()
{
  var v = 5;
  let q = 17;

  
  for (let v = 0; v < 10; v++)
    q += v;

  if (q != 62)
    throw "f4(): wrong value for q\n" +
      "  expected: 62\n" +
      "  actual:   " + q;

  return v;
}

try
{
  if (f1() != 5 || x != 7)
    throw "f1() == 5";
  if (f2() != 3 || x != 7)
    throw "f2() == 3";
  if (f3() != 10 || x != 7)
    throw "f3() == 10";

  if (f4() != 5)
    throw "f4() == 5";

  var bad = true;
  try
  {
    eval("q++"); 
  }
  catch (e)
  {
    if (e instanceof ReferenceError)
      bad = false;
  }
  if (bad)
    throw "f4(): q escaping scope!";
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
