




































var gTestfile = 'regress-332472.js';

var BUGNUMBER = 332472;
var summary = 'new RegExp() ignores string boundaries when throwing exceptions';
var actual = '';
var expect = 'SyntaxError: invalid quantifier ?asdf';

printBugNumber(BUGNUMBER);
printStatus (summary);

var str1 = "?asdf\nAnd you really shouldn't see this!";
var str2 = str1.substr(0, 5);
try {
  new RegExp(str2);
}
catch(ex) {
  printStatus(ex);
  actual = ex + '';
}
 
reportCompare(expect, actual, summary);
