






































var gTestfile = 'regress-116228.js';

var BUGNUMBER = 116228;
var summary = 'Do not crash - JSOP_THIS should null obj register';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var obj = {};
obj.toString = function() {return this();}
  try
  {
    obj.toString();
  }
  catch(e)
  {
  }
reportCompare(expect, actual, summary);
