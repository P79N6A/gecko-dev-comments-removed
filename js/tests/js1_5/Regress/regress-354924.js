




































var bug = 354924;
var summary = 'Do not crash with export/import and setter';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  this.x setter= function(){}; 
  export *; 
  t = this; 
  new Function("import t.*; import t.*;")();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
