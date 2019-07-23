




































var gTestfile = '15.7.4.3-01.js';

var BUGNUMBER = "412068";
var summary = "num.toLocaleString incorrectly accesses its first argument " +
              "even when no first argument has been given";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

try
{
  if ("3" !== 3..toLocaleString())
    throw '"3" should equal 3..toLocaleString()';
  if ("9" !== 9..toLocaleString(8))
    throw 'Number.prototype.toLocaleString should ignore its first argument';
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);
