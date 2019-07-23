




































var gTestfile = 'regress-355145.js';

var BUGNUMBER = 355145;
var summary = 'JS_GetMethodById() on XML Objects';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj = <x/>;
  expect = "foo";

  obj.function::__iterator__ = function() { yield expect; };
  for(var val in obj)
    actual = val;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
