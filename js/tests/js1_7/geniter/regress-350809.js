




































var gTestfile = 'regress-350809.js';

var BUGNUMBER = 350809;
var summary = 'Do not assertion: if yield in xml filtering predicate';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    eval('(function(){ <x/>.(yield 4) })().next();');
  }
  catch(ex)
  {
    actual = expect =
      'InternalError: yield not yet supported from filtering predicate';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
