




































var bug = 352605;
var summary = 'Do not assert with |yield|, nested xml-filtering predicate';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  expect = 'InternalError: yield not yet supported from filtering predicate';
  try
  {  
    (function() { <y/>.(<x/>.(false), (yield 3)) })().next();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
