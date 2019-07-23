




































var bug = 355341;
var summary = 'Do not crash with watch and setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  this.x setter= Function; this.watch('x', function () { }); x = 3;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
