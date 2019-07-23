




































var gTestfile = 'regress-355339.js';

var BUGNUMBER = 355339;
var summary = 'Do not assert: sprop->setter != js_watch_set';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = actual = 'No Crash';
  o = {};
  o.watch("j", function(a,b,c) { print("*",a,b,c) });
  o.unwatch("j");
  o.watch("j", function(a,b,c) { print("*",a,b,c) });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
