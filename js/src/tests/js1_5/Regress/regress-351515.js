





var BUGNUMBER = 351515;
var summary = 'js17 features must be enabled by version request';
var actual = 'No Error';
var expect = 'No Error';



test();


yield = 1;
let   = 1;

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(yield, let) { return yield+let; }

  var yield = 1;
  var let = 1;

  function yield() {}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
