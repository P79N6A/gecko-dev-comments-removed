




































var bug = 320172;
var summary = 'Regression from bug 285219';
var actual = 'No Crash';
var expect = 'No Crash';

enterFunc ('test');
printBugNumber (bug);
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
