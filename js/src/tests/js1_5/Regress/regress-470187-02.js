




































var gTestfile = 'regress-470187-02.js';

var BUGNUMBER = 470187;
var summary = 'Do not assert: ATOM_IS_STRING(atom)';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');

  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var j=0;j<3;++j) ({valueOf: function(){return 2}}) - [];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
