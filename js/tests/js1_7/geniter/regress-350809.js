




































var bug = 350809;
var summary = 'Assertion if yield in xml filtering predicate';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
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
