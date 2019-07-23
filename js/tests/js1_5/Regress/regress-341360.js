





































var bug = 341360;
var summary = 'clearInterval broken';
var actual = '';
var expect = 'Ok';

printBugNumber (bug);
printStatus (summary);

function xxx() 
{
  if(t != null) 
  {
    print('Clearing interval...');
    window.clearInterval(t);
    t = null;
    setTimeout('yyy()', 2000);
    
  }
  else {
    print('Clearing interval failed...');
    actual = "Broken";
    gDelayTestDriverEnd = false;
    reportCompare(expect, actual, summary);
    jsTestDriverEnd();
  }
}

function yyy()
{
  print('Checking result...');
  actual = 'Ok';
  gDelayTestDriverEnd = false;
  reportCompare(expect, actual, summary);
  jsTestDriverEnd();
}

if (typeof window == 'undefined')
{
  expect = actual = 'Not tested';
  reportCompare(expect, actual, summary);
}
else
{
  print('Start...');
  gDelayTestDriverEnd = true;
  var t = window.setInterval(xxx, 1000);
}

