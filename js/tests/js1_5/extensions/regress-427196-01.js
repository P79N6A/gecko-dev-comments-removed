




































var gTestfile = 'regress-427196-01.js';

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

  function boom()
  {
    var b = {};
    Array.__proto__ = b;
    b.__proto__ = {};
    var c = {};
    c.__proto__ = [];
    try { c.__defineSetter__("t", undefined); } catch(e) { }
    c.__proto__ = Array;
    try { c.__defineSetter__("v", undefined); } catch(e) { }
  }

  boom();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
