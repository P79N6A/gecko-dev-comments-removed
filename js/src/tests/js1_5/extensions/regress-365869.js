





var BUGNUMBER = 365869;
var summary = 'strict warning for object literal with duplicate propery names';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (!options().match(/strict/))
  {
    options('strict');
  }
  if (!options().match(/werror/))
  {
    options('werror');
  }

  try
  {
    expect = 'SyntaxError: property name a appears more than once in object literal';
    eval('({a:4, a:5})');
    
    actual = 'No warning';
  }
  catch(ex)
  {
    actual = ex + '';
  }
 
  reportCompare(expect, actual, summary);

  print('test crash from bug 371292 Comment 3');

  try
  {
    expect = 'SyntaxError: property name 1 appears more than once in object literal';
    eval('({1:1, 1:2})');
    
    actual = 'No warning';
  }
  catch(ex)
  {
    actual = ex + '';
  }
 
  reportCompare(expect, actual, summary);


  print('test crash from bug 371292 Comment 9');

  try
  {
    expect = "TypeError: can't redefine non-configurable property '5'";
    "012345".__defineSetter__(5, function(){});
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);


  exitFunc ('test');
}
