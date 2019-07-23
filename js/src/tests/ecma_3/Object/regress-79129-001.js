













































var gTestfile = 'regress-79129-001.js';
var BUGNUMBER = 79129;
var summary = "Regression test: we shouldn't crash on this code";


test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  tryThis();
  reportCompare('No Crash', 'No Crash', 'Should not crash');
  exitFunc ('test');
}


function tryThis()
{
  obj={};
  obj.a = obj.b = obj.c = 1;
  delete obj.a;
  delete obj.b;
  delete obj.c;
  obj.d = obj.e = 1;
  obj.a=1;
  obj.b=1;
  obj.c=1;
  obj.d=1;
  obj.e=1;
}
