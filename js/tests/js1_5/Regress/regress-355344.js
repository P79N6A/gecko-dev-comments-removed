




































var bug = 355344;
var summary = 'Exceptions thrown by watch point';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var o = {};

  expect = 'setter: yikes';

  o.watch('x', function(){throw 'yikes'}); 
  try
  {
    o.x = 3; 
  }
  catch(ex)
  {
    actual = "setter: " + ex;
  }

  try 
  { 
    eval("") ;
  }
  catch(e) 
  { 
    actual = "eval: " + e;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
