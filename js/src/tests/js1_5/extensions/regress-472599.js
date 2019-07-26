





var BUGNUMBER = 472599;
var summary = 'Do not assert: STOBJ_GET_SLOT(callee_obj, JSSLOT_PRIVATE).isInt32()';
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
