




































var gTestfile = 'regress-470187-01.js';

var BUGNUMBER = 470187;
var summary = 'Do not assert: entry->kpc == (jsbytecode*) atoms[index]';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var j=0;j<3;++j) ({valueOf: function(){return 2}}) - /x/;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
