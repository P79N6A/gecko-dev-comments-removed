





var BUGNUMBER = 328897;
var summary = 'JS_ReportPendingException should';

var actual = 'No Error';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
if (typeof window == 'undefined')
{
  reportCompare(expect, actual, summary);
}
else
{
  expect = /TypeError: Not enough arguments to Window.showModalDialog./;

  window._onerror = window.onerror;
  window.onerror = (function (msg, page, line) { 
      actual = msg; 
      gDelayTestDriverEnd = false;
      jsTestDriverEnd();
      reportMatch(expect, actual, summary);
    });

  gDelayTestDriverEnd = true;

  
  window.showModalDialog();
  actual = 'No Error';
}

function onload() 
{
  if (actual == 'No Error')
  {
    gDelayTestDriverEnd = false;
    jsTestDriverEnd();
    reportCompare(expect, actual, summary);
  }
}
