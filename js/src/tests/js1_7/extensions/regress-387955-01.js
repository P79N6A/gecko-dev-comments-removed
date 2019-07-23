




































var gTestfile = 'regress-387955-01.js';


var BUGNUMBER = 387955;
var summary = 'Do not Crash [@ JS_CallTracer]';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var fal = false;

  function gen() { 
    let x;
    function y(){} 
    this.__defineGetter__('', function(){}); 
    if (fal)
      yield;
  }

  for (var i in gen()) { }

  gc();
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
