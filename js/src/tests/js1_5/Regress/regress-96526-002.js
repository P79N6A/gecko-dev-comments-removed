














































var gTestfile = 'regress-96526-002.js';
printBugNumber(96526);
printStatus("Just seeing that we don't crash when compiling this script -");





a="[\"b\"]";
s="g";
for(i=0;i<20000;i++)
  s += a;
try {eval(s);}
catch (e) {};

reportCompare('No Crash', 'No Crash', '');
