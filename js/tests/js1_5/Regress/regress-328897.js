




































var bug = 328897;
var summary = 'JS_ReportPendingException should';
var actual = 'Error';
var expect = 'Error';

printBugNumber (bug);
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
