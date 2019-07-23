




































var gTestfile = 'regress-346363.js';

var BUGNUMBER = 346363;
var summary = 'Date.prototype.setFullYear()';
var actual = '';
var expect = true;



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var d = new Date();
  d.setFullYear();
  d.setFullYear(2006);
  actual = d.getFullYear() == 2006;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
