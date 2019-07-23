




































var gTestfile = 'regress-321971.js';

var BUGNUMBER = 321971;
var summary = 'JSOP_FINDNAME replaces JSOP_BINDNAME';
var actual = 'no error';
var expect = 'no error';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var s='';
for (i=0; i < 1<<16; i++)
  s += 'x' + i + '=' + i + ';\n';

s += 'foo=' + i + ';\n';
eval(s);
foo;

reportCompare(expect, actual, summary);
