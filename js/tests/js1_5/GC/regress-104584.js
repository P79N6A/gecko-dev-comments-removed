

















































var gTestfile = 'regress-104584.js';
var BUGNUMBER = 104584;
var summary = "Testing that we don't crash on this code -";

printBugNumber(BUGNUMBER);
printStatus (summary);

F();
G();

reportCompare('No Crash', 'No Crash', '');

function F(obj)
{
  if(!obj)
    obj = {};
  obj.toString();
  gc();
  obj.toString();
}


function G(obj)
{
  if(!obj)
    obj = {};
  print(obj);
  gc();
  print(obj);
}
