




































var bug = 338001;
var summary = 'integer overflow in jsfun.c:Function';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

expectExitCode(0);
expectExitCode(5);

var fe="f";

for (i=0; i<25; i++) 
  fe += fe;

var fu=new Function(
  fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, 
  fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, 
  fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, 
  fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
  "done"
  );
  
print('Done');

reportCompare(expect, actual, summary);
