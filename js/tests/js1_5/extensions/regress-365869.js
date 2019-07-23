




































var bug = 365869;
var summary = 'strict warning for object literal with duplicate propery names';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var jsOptions = new JavaScriptOptions();


  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);
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
  jsOptions.reset();
  
  reportCompare(expect, actual, summary);

  print('test crash from bug 371292');

  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);
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
  jsOptions.reset();
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
