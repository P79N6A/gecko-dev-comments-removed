




































var gTestfile = '15.7.4.7-2.js';

var BUGNUMBER = "411893";
var summary = "num.toPrecision(undefined) should equal num.toString()";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

try
{
  var prec = 3.3.toPrecision(undefined);
  var str  = 3.3.toString();
  if (prec !== str)
  {
    throw "not equal!  " +
          "3.3.toPrecision(undefined) === '" + prec + "', " +
          "3.3.toString() === '" + str + "'";
  }
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
