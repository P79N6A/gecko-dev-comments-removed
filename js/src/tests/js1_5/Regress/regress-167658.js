






































var BUGNUMBER = 167658;
var summary = 'Do not crash due to js_NewRegExp initialization';
var actual = 'No Crash';
var expect = 'No Crash';
printBugNumber(BUGNUMBER);
printStatus (summary);

var UBOUND=100;
for (var j=0; j<UBOUND; j++)
{
  'Apfelkiste, Apfelschale'.replace('Apfel', function()
				    {
				      for (var i = 0; i < arguments.length; i++)
					printStatus(i+': '+arguments[i]);
				      return 'Bananen';
				    });

  printStatus(j);
}
 
reportCompare(expect, actual, summary);

