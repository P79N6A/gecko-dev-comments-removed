




































var bug = 344120;
var summary = 'function to source with numeric labels';
var actual = '';
var expect = 'function () {\n    x = {1:1};\n}';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  actual = ''+function (){x={1:1}}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
