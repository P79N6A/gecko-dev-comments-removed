




































var gTestfile = 'regress-347593.js';

var BUGNUMBER = 347593;
var summary = 'For-each loop with destructuring assignment';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '23';
  actual = '';
  for (let [, { a: b }] in [{ a: 2 }, { a: 3 }])
  {
    actual += b;
  }
  reportCompare(expect, actual, summary);

  actual = '';
  for each (let { a: b } in [{ a: 2 }, { a: 3 }])
  {
    actual += b;
  }
  reportCompare(expect, actual, summary);

  expect = 'TypeError';
  actual = '';
  try
  {
    for each (let [, { a: b }] in [{ a: 2 }, { a: 3 }])
    {
      actual += b;
    }
  }
  catch(ex)
  {
    print(ex+'');
    actual = ex.name;
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
