




































var bug = 352415;
var summary = 'decompilation of labelled loop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function() { L: for (let x;;) continue L };
  actual = f + '';
  expect = 'function() { L: for (let x;;) {continue L;} }';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
