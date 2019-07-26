





var BUGNUMBER = 459628;
var summary = 'Do not assert: STOBJ_GET_SLOT(obj, map->freeslot).isUndefined()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

(function() { 
  for (var odjoff = 0; odjoff < 4; ++odjoff) { 
    new Date()[0] = 3; 
  } 
})();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
