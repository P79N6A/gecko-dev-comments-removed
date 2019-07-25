






var BUGNUMBER = 470388;
var summary = 'TM: Do not assert: !(fp->flags & JSFRAME_POP_BLOCKS)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for each (let x in [function(){}, {}, {}, function(){}, function(){},
                      function(){}]) { ([<y/> for (y in x)]); }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
