




































var gTestfile = 'regress-479353.js';

var BUGNUMBER = 479353;
var summary = 'Do not assert: (uint32)(index_) < atoms_->length';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (new Function("({}), arguments;"))();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
