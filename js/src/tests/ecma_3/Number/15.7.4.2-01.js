




































var gTestfile = '15.7.4.2-01.js';

var BUGNUMBER = "411889";
var summary = "num.toString(), num.toString(10), and num.toString(undefined)" +
              " should all be equivalent";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

try
{
  var noargs = 3.3.toString();
  var tenarg = 3.3.toString(10);
  var undefarg = 3.3.toString(undefined);

  if (noargs !== tenarg)
    throw "() !== (10): " + noargs + " !== " + tenarg;
  if (tenarg !== undefarg)
    throw "(10) !== (undefined): " + tenarg + " !== " + undefarg;
}
catch (e)
{
  failed = e;
}

expect = false;
actual = failed;

reportCompare(expect, actual, summary);

expect = 1;
actual = 3.3.toString.length;
reportCompare(expect, actual, '3.3.toString.length should be 1');
