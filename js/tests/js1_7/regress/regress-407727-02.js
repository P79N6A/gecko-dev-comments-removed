




































var gTestfile = 'regress-407727-02.js';

var BUGNUMBER = 407727;
var summary = 'let Object block redeclaration';
var actual = '';
var expect = 1;

printBugNumber(BUGNUMBER);
printStatus (summary);

{ 
  let Object = 1;
  actual = Object;
  reportCompare(expect, actual, summary);
}
