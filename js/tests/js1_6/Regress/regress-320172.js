




































var gTestfile = 'regress-320172.js';

var BUGNUMBER = 320172;
var summary = 'Regression from bug 285219';
var actual = 'No Crash';
var expect = 'No Crash';

enterFunc ('test');
printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  (function xxx(){ ["var x"].forEach(eval); })();
}
catch(ex)
{
}

printStatus('No Crash');
reportCompare(expect, actual, summary);
