





var BUGNUMBER = 469761;
var summary = 'TM: Do not assert: STOBJ_GET_SLOT(callee_obj, JSSLOT_PRIVATE).isInt32()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  var o = { __proto__: function(){} };
  for (var j = 0; j < 3; ++j) { try { o.call(3); } catch (e) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
