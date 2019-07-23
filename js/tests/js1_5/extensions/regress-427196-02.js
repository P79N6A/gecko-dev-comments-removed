




































var gTestfile = 'regress-427196-02.js';

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
    var c = {__proto__: []};
    var a = {__proto__: {__proto__: {}}};
    c.hasOwnProperty("t");
    c.__proto__ = a;
    c.hasOwnProperty("v");
  }

  boom();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
