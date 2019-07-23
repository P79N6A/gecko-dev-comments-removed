





































var gTestfile = 'regress-304376.js';

var BUGNUMBER = 304376;
var summary = 'String.prototype should be readonly and permanent';
var actual = '';
var expect = '';
printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'TypeError';

var saveString = String;
 
String = Array;

try
{
  
  "".join();
  String = saveString;
  actual = 'No Error';
}
catch(ex)
{
  String = saveString;
  actual = ex.name;
  printStatus(ex + '');
}

reportCompare(expect, actual, summary);
