




































var bug = 371692;
var summary = 'Keep extra parentheses in conditional tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function (){ if((i=1)){ a=2; } };
  expect = 'function (){ if((i=1)){ a=2; } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
