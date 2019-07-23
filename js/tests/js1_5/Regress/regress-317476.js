




































var bug = 317476;
var summary = 'The error thrown by JS_ReportError should be catchable';
var actual = 'no error';
var expect = 'no error';

printBugNumber (bug);
printStatus (summary);

if (typeof setTimeout != 'undefined')
{
  expect = 'error';
  try 
  { 
    setTimeout(2); 
  } 
  catch(ex) 
  {
    actual = 'error';
    printStatus(ex+'');
  }
}

  

reportCompare(expect, actual, summary);
