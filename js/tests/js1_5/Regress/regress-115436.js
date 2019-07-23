





































var bug = 115436;
var summary = 'Do not crash javascript warning duplicate arguments';
var actual = 'No Crash';
var expect = 'No Crash';


printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);

function x(y,y) 
{
  return 3;
}

var z = x(4,5);

jsOptions.reset();

reportCompare(expect, actual, summary);
