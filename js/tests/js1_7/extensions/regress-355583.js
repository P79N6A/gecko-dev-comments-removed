




































var bug = 355583;
var summary = 'block object access to arbitrary stack slots';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  expect = 'No Crash';
  actual = 'No Crash';
  try
  {
  (function() {
    let b = function(){}.__parent__;
    print(b[1] = throwError);
  })();
  }
  catch(ex)
  {

  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
