




































var gTestfile = 'regress-427196-03.js';

var BUGNUMBER = 427196;
var summary = 'Do not assert: OBJ_SCOPE(pobj)->object == pobj';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var c = {__proto__: []};
  var a = {__proto__: {__proto__: {}}};
  c.hasOwnProperty;
  c.__proto__ = a;
  c.hasOwnProperty;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
