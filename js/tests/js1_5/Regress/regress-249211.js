





































var bug = 249211;
var summary = 'support export and import for 4xp';
var actual = '';
var expect = 'no error';

printBugNumber (bug);
printStatus (summary);
  
try
{
  var o = {}; 
  var f = function(){}; 
  export *; 
  import o.*;
  actual = 'no error';
}
catch(e)
{
  actual = 'error';
}

reportCompare(expect, actual, summary);

