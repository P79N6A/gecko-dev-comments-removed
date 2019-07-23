




































var gTestfile = 'regress-328897.js';

var BUGNUMBER = 328897;
var summary = 'JS_ReportPendingException should';
var actual = 'Error';
var expect = 'Error';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
if (typeof window != 'undefined')
{
  try
  {
    window.location="javascript:Components.classes";
    actual = 'No Error';
  }
  catch(ex)
  {
    printStatus(ex+'');
  }
}

reportCompare(expect, actual, summary);
