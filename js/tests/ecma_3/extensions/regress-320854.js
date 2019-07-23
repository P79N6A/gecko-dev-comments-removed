




































var bug = 320854;
var summary = 'o.hasOwnProperty("length") should not lie when o has function in proto chain';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var o = {__proto__:function(){}}; 

expect = false;
actual = o.hasOwnProperty('length')
  
reportCompare(expect, actual, summary);
