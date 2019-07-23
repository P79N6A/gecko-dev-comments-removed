




































var gTestfile = 'regress-317533.js';

var BUGNUMBER = 317533;
var summary = 'improve function does not always return a value warnings';
var actual = '';
var expect = 'TypeError: anonymous function does not always return a value';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var code;

if (!options().match(/strict/))
{
  options('strict');
}
if (!options().match(/werror/))
{
  options('werror');
}

try
{
  actual = '';
  code = "(function(x){ if(x) return x; })";
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
  code = "(function(x){ if(x) return x; ;})";
  print(code);
  eval(code);
}
catch(ex)
{
  actual = ex + '';
  print(ex);
}

reportCompare(expect, actual, summary);
