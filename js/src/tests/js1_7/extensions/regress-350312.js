





var BUGNUMBER = 350312;
var summary = 'Accessing wrong stack slot with nested catch/finally';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var iter;
  function gen()
  {
    try {
      yield iter;
    } catch (e if e == null) {
      actual += 'CATCH,';
      print("CATCH");
    } finally {
      actual += 'FINALLY';
      print("FINALLY");
    }
  }

  expect = 'FINALLY';
  actual = '';
  (iter = gen()).next().close();
  reportCompare(expect, actual, summary);

  expect = 'FINALLY';
  actual = '';
  try
  {
    (iter = gen()).next().throw(1);
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  expect = 'CATCH,FINALLY';
  actual = '';
  try
  {
    (iter = gen()).next().throw(null);
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  expect = 'FINALLY';
  actual = '';
  var expectexcp = '[object StopIteration]';
  var actualexcp = '';
  try
  {
    (iter = gen()).next().next();     
  }
  catch(ex)
  {
    actualexcp = ex + '';
  }
  reportCompare(expect, actual, summary);
  reportCompare(expectexcp, actualexcp, summary);

  exitFunc ('test');
}
