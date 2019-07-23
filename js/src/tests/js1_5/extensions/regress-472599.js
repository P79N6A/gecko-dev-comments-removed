




































var gTestfile = 'regress-472599.js';

var BUGNUMBER = 472599;
var summary = 'Do not assert: JSVAL_IS_INT(STOBJ_GET_SLOT(callee_obj, JSSLOT_PRIVATE))';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  var a = (function(){}).prototype;
  a.__proto__ = a.toString;
  for (var i = 0; i < 4; ++i) { try{ a.call({}); } catch(e) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
