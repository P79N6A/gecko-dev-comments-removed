





var BUGNUMBER = 349023;
var summary = 'Bogus JSCLASS_IS_EXTENDED in the generator class';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var gen = (function() { yield 3; })();
  gen.foopy;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
