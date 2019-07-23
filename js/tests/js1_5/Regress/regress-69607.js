













































var gTestfile = 'regress-69607.js';
var BUGNUMBER = 69607;
var summary = "Testing that we don't crash on trivial JavaScript";
var var1;
var var2;
var var3;

printBugNumber(BUGNUMBER);
printStatus (summary);





if(false)
{
  var1 = 0;
}
else
{
  var2 = 0;
}

if(false)
{
  var3 = 0;
}

reportCompare('No Crash', 'No Crash', '');

