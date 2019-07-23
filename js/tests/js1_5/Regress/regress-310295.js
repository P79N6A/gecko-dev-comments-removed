




































var bug = 310295;
var summary = 'Do not crash on JS_ValueToString';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var tmp = 23948730458647527874392837439299837412374859487593;

tmp = new Number(tmp);
tmp = tmp.valueOf()
tmp = String(tmp);
  
reportCompare(expect, actual, summary);
