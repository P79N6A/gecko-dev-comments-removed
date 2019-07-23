




































var bug = 317533;
var summary = 'improve function does not always return a value warnings';
var actual = '';
var expect = 'TypeError: anonymous function does not always return a value';

printBugNumber (bug);
printStatus (summary);
  
var code;

debugger;

options('strict');
options('werror');

try
{
  actual = ''; 
  code = "function(x){ if(x) return x; }";
  print(code);
  eval(code);
}
catch(ex)
{
  actual = ex + '';
  print(ex);
}

reportCompare(expect, actual, summary);

try
{
  actual = '';
  code = "function(x){ if(x) return x; ;}";
  print(code);
  eval(code);
}
catch(ex)
{
  actual = ex + '';
  print(ex);
}

reportCompare(expect, actual, summary);
