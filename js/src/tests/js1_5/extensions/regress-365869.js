




































var gTestfile = 'regress-365869.js';

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
    expect = 'TypeError: redeclaration of property a';
    var o = {a:4, a:5};
    
    actual = 'No warning';
  }
  catch(ex)
  {
    actual = ex + '';
    print(ex);
  }
 
  reportCompare(expect, actual, summary);

  print('test crash from bug 371292 Comment 3');

  try
  {
    expect = 'TypeError: redeclaration of property 1';
    var o1 = {1:1, 1:2};
    
    actual = 'No warning';
  }
  catch(ex)
  {
    actual = ex + '';
    print(ex);
  }
 
  reportCompare(expect, actual, summary);


  print('test crash from bug 371292 Comment 9');

  try
  {
    expect = 'TypeError: redeclaration of const 5';
    "012345".__defineSetter__(5, function(){});
  }
  catch(ex)
  {
    actual = ex + '';
    print(ex);
  }

  reportCompare(expect, actual, summary);


  exitFunc ('test');
}
