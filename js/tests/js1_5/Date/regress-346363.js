




































var bug = 346363;
var summary = 'Date.prototype.setFullYear()';
var actual = '';
var expect = true;



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var d = new Date();
  d.setFullYear();
  d.setFullYear(2006);
  actual = d.getFullYear() == 2006;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
