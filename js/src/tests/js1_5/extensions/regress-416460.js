




































var gTestfile = 'regress-416460.js';

var BUGNUMBER = 416460;
var summary = 'Do not assert: SCOPE_GET_PROPERTY(OBJ_SCOPE(pobj), ATOM_TO_JSID(atom))';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  /a/.__proto__.__proto__ = { "2": 3 };
  var b = /b/;
  b["2"];
  b["2"];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
