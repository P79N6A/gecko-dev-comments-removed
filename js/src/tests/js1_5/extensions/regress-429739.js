





































var gTestfile = 'regress-429739.js';

var BUGNUMBER = 429739;
var summary = 'Do not assert: OBJ_GET_CLASS(cx, obj)->flags & JSCLASS_HAS_PRIVATE';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'function anonymous(y) {}';

  try
  {
    var o = { __noSuchMethod__: Function }; 
    actual = (new o.y) + '';
  }
  catch(ex)
  {
    actual = ex + '';
  }

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
