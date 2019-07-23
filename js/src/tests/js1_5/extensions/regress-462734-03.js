




































var gTestfile = 'regress-462734-03.js';

var BUGNUMBER = 462734;
var summary = 'Do not assert: pobj_ == obj2';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var save__proto__ = __proto__;

try
{
  Function.prototype.prototype;
  __proto__ = Function();
  prototype = prototype;
}
catch(ex)
{
  print(ex + '');
}

__proto__ = save__proto__;

reportCompare(expect, actual, summary);
