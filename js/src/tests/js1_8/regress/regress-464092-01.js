




































var gTestfile = 'regress-464092-01.js';

var BUGNUMBER = 464092;
var summary = 'Do not assert: OBJ_IS_CLONED_BLOCK(obj)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  let (a) 'b'.replace(/b/g, function() { c = this; }); c.d = 3; c.d;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
