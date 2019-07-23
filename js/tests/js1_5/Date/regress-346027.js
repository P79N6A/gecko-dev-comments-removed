




































var bug = 346027;
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
  actual = isNaN(d.getFullYear());

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
