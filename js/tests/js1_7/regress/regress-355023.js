




































var bug = 355023;
var summary = 'destructuring assignment optimization';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  Array.prototype[0] = 1024;

  expect = (function(){ var a=[],[x]=a; return x; })();
  actual = (function(){ var [x]=[]; return x; })();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
